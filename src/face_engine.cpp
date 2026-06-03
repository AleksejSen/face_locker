#include "face_engine.h"

FaceRecognitionEngine::FaceRecognitionEngine(const Config &config) {
  // 1. Create temporary vectors from the span data
  std::vector<uchar> det_buffer(config.face_detection_model.begin(),
                                config.face_detection_model.end());

  std::vector<uchar> rec_buffer(config.face_recognition_model.begin(),
                                config.face_recognition_model.end());

  // Empty buffer needed for the network topology argument
  std::vector<uchar> empty_config_buffer;

  // 2. Initialize YuNet from memory buffer
  face_detector_ = cv::FaceDetectorYN::create(
      "onnx",                                 // Framework type
      det_buffer,                             // Model weights vector
      empty_config_buffer,                    // Empty architecture config
      cv::Size(640, 480),                     // Default input image size
      config.confidence_threshold,            // Score threshold
      config.non_max_suppression,             // NMS threshold
      static_cast<int>(config.max_detections) // Top K detections
  );

  // 3. Initialize SFace from memory buffer
  face_recognizer_ =
      cv::FaceRecognizerSF::create("onnx",             // Framework type
                                   rec_buffer,         // Model weights vector
                                   empty_config_buffer // Empty config vector
      );

  get_all_reference_facial_signatures(config);
}

cv::Mat FaceRecognitionEngine::detect_faces(const cv::Mat &image) {
  cv::Mat faces;
  face_detector_->setInputSize(image.size());
  face_detector_->detect(image, faces);
  return faces;
}

cv::Mat FaceRecognitionEngine::get_facial_features(cv::Mat faces1, int facenum,
                                                   cv::Mat image1) {
  cv::Mat aligned_face1;
  face_recognizer_->alignCrop(image1, faces1.row(facenum), aligned_face1);
  cv::Mat feature1;
  face_recognizer_->feature(aligned_face1, feature1);
  return feature1.clone();
}

std::optional<std::vector<cv::Mat>>
FaceRecognitionEngine::get_all_reference_facial_signatures(
    const Config &config) {
  reference_image_ = cv::imread(config.reference_picture_path).clone();
  reference_faces_ = detect_faces(reference_image_);

  if (reference_faces_.empty()) {
    return std::nullopt;
  }
  reference_face_signatures_.reserve(reference_faces_.rows);

  for (int i : std::views::iota(0, reference_faces_.rows)) {
    cv::Mat face_feature =
        get_facial_features(reference_faces_, i, reference_image_);
    reference_face_signatures_.push_back(face_feature);
  }
  return reference_face_signatures_;
}

void FaceRecognitionEngine::draw_face_annotations(
    cv::Mat &canvas, const cv::Mat &faces,
    const std::vector<int> *matched_indices) {
  const int thickness = 2;

  for (int i = 0; i < faces.rows; i++) {
    auto box_color = cv::Scalar(0, 0, 255); // Red
    std::string text = "Face " + std::to_string(i);

    if (matched_indices != nullptr) {
      if (i < matched_indices->size() && (*matched_indices)[i] != -1) {
        box_color = cv::Scalar(0, 255, 0); // Green
        text = "Matched Ref #" + std::to_string((*matched_indices)[i]);
      } else {
        text = "Unknown Face";
      }
    } else {
      box_color = cv::Scalar(255, 0, 0); // Cyan
      text = "Ref Target #" + std::to_string(i);
    }

    cv::Rect face_rect(faces.at<float>(i, 0), faces.at<float>(i, 1),
                       faces.at<float>(i, 2), faces.at<float>(i, 3));
    cv::rectangle(canvas, face_rect, box_color, 10);

    cv::circle(
        canvas,
        cv::Point2i(int(faces.at<float>(i, 4)), int(faces.at<float>(i, 5))), 2,
        cv::Scalar(255, 0, 0), thickness);
    cv::circle(
        canvas,
        cv::Point2i(int(faces.at<float>(i, 6)), int(faces.at<float>(i, 7))), 2,
        cv::Scalar(0, 0, 255), thickness);
    cv::circle(
        canvas,
        cv::Point2i(int(faces.at<float>(i, 8)), int(faces.at<float>(i, 9))), 2,
        cv::Scalar(0, 255, 0), thickness);
    cv::circle(
        canvas,
        cv::Point2i(int(faces.at<float>(i, 10)), int(faces.at<float>(i, 11))),
        2, cv::Scalar(255, 0, 255), thickness);
    cv::circle(
        canvas,
        cv::Point2i(int(faces.at<float>(i, 12)), int(faces.at<float>(i, 13))),
        2, cv::Scalar(0, 255, 255), thickness);

    int font_face = cv::FONT_HERSHEY_SIMPLEX;
    double font_scale = 0.6;
    int text_thickness = 2;
    cv::Point text_org(face_rect.x, face_rect.y - 10);
    if (text_org.y < 0)
      text_org.y = face_rect.y + 20;

    cv::putText(canvas, text, text_org, font_face, font_scale, box_color,
                text_thickness);
  }
}

FacesData FaceRecognitionEngine::recognize_faces(cv::Mat input_data,
                                                 const Config &config) {
  FacesData result;
  result.input_image = input_data.clone();
  result.faces = detect_faces(result.input_image);
  result.matched_face.assign(result.faces.rows,
                             -1); // Adjusted type matching to vector<int>
  bool any_match_found = false;

  for (int index : std::views::iota(0, result.faces.rows)) {
    cv::Mat input_data_face_feature =
        get_facial_features(result.faces, index, input_data);
    for (size_t ref_idx = 0; ref_idx < reference_face_signatures_.size();
         ++ref_idx) {
      double score = face_recognizer_->match(
          reference_face_signatures_[ref_idx], input_data_face_feature);
      if (score >= config.confidence_threshold) {
        result.matched_face[index] = ref_idx;
        any_match_found = true;
        std::println("==== MATCHING FACE FOUND ====");
        break;
      }
    }
  }

  if (!any_match_found) {
    std::println("==== NO MATCHING FACE ====");
  }
  return result;
}

void FaceRecognitionEngine::visualize_references() {
  if (reference_faces_.empty() || reference_image_.empty()) {
    std::println("==== WARNING: Reference assets are empty! ====");
    return;
  }

  cv::Mat display_img = reference_image_.clone();
  draw_face_annotations(display_img, reference_faces_);

  cv::namedWindow("Reference Pic", cv::WINDOW_NORMAL);
  cv::imshow("Reference Pic", display_img);
}

void FaceRecognitionEngine::visualize(FacesData &face_data) {
  if (face_data.input_image.empty())
    return;

  cv::Mat display_img = face_data.input_image.clone();
  draw_face_annotations(display_img, face_data.faces, &face_data.matched_face);

  cv::namedWindow("Camera Pic", cv::WINDOW_NORMAL);
  cv::imshow("Camera Pic", display_img);
}
