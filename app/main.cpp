#include "opencv2/core/mat.hpp"
#include <CLI/CLI.hpp>
#include <cstdlib>
#include <format>
#include <iostream>
#include <memory>
#include <opencv2/core/types.hpp>
#include <opencv2/dnn/dnn.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect/face.hpp>
#include <print>
#include <ranges>
#include <string>
#include <vector>

// Returns a cv::Mat on success, or std::nullopt if the camera fails
std::optional<cv::Mat> capture_from_webcam(int camera_index = 0) {
  // Force the use of Video4Linux2 API backend on Linux systems
  cv::VideoCapture cap(camera_index + cv::CAP_V4L2);

  if (!cap.isOpened()) {
    std::cerr << "Error: Cannot open camera at index " << camera_index << "\n";
    return std::nullopt;
  }

  // Skip a few initial frames to let the laptop camera auto-expose/warm up
  cv::Mat frame;
  for (int i = 0; i < 5; ++i) {
    cap >> frame;
  }

  if (frame.empty()) {
    std::cerr << "Error: Captured an empty frame from camera.\n";
    return std::nullopt;
  }

  // Explicitly release the camera hardware resource so other apps can use it
  cap.release();

  return frame;
}

struct Config {
  // 1. Strict, compile-time member constants (shared across all instances)
  static constexpr std::string_view face_detection_model =
      "models/face_detection_yunet_2023mar.onnx";
  static constexpr std::string_view face_recognition_model =
      "models/face_recognition_sface_2021dec.onnx";

  std::string reference_picture_path = "";
  bool debug_mode = false;
  float confidence_threshold = 0.6f;
  float non_max_suppression = 0.3f;
  float max_detections = 5000;
};

struct FacesData {
  std::vector<int> matched_face;
  cv::Mat input_image;
  cv::Mat faces;
};

class FaceRecognitionEngine {
private:
  std::shared_ptr<cv::FaceDetectorYN> face_detector_;
  std::shared_ptr<cv::FaceRecognizerSF> face_recognizer_;
  cv::Mat reference_image_;
  cv::Mat reference_faces_;
  std::vector<cv::Mat> reference_face_signatures_;

  cv::Mat detect_faces(const cv::Mat &image) {
    cv::Mat faces;
    face_detector_->setInputSize(image.size());
    face_detector_->detect(image, faces);
    return faces;
  }

  cv::Mat get_facial_features(cv::Mat faces1, int facenum, cv::Mat image1) {
    cv::Mat aligned_face1;
    face_recognizer_->alignCrop(image1, faces1.row(facenum), aligned_face1);
    cv::Mat feature1;
    face_recognizer_->feature(aligned_face1, feature1);
    return feature1.clone();
  }

  std::optional<std::vector<cv::Mat>>
  get_all_reference_facial_signatures(const Config &config) {
    reference_image_ = cv::imread(config.reference_picture_path).clone();
    reference_faces_ = detect_faces(reference_image_);

    if (reference_faces_.empty()) {
      return std::nullopt;
    }
    reference_face_signatures_.reserve(reference_faces_.rows);

    for (int i : std::views::iota(0, reference_faces_.rows)) {
      // Get face feature
      cv::Mat face_feature =
          get_facial_features(reference_faces_, i, reference_image_);
      // Save face feature
      reference_face_signatures_.push_back(face_feature);
    }
    return reference_face_signatures_;
  }

  // Face Visualization
  void
  draw_face_annotations(cv::Mat &canvas, const cv::Mat &faces,
                        const std::vector<int> *matched_indices = nullptr) {
    const int thickness = 2;

    for (int i = 0; i < faces.rows; i++) {
      // Default colors and text
      auto box_color = cv::Scalar(0, 0, 255); // Red
      std::string text = "Face " + std::to_string(i);

      // If matched_indices is provided, determine color/text dynamically
      if (matched_indices != nullptr) {
        if (i < matched_indices->size() && (*matched_indices)[i] != -1) {
          box_color = cv::Scalar(0, 255, 0); // Green for matches
          text = "Matched Ref #" + std::to_string((*matched_indices)[i]);
        } else {
          text = "Unknown Face";
        }
      } else {
        // No match vector passed means we are visualizing raw reference
        // baselines
        box_color = cv::Scalar(255, 0, 0); // Cyan/Blue for references
        text = "Ref Target #" + std::to_string(i);
      }

      // Draw bounding box
      cv::Rect face_rect(faces.at<float>(i, 0), faces.at<float>(i, 1),
                         faces.at<float>(i, 2), faces.at<float>(i, 3));
      cv::rectangle(canvas, face_rect, box_color, 10);

      // Draw landmarks
      circle(
          canvas,
          cv::Point2i(int(faces.at<float>(i, 4)), int(faces.at<float>(i, 5))),
          2, cv::Scalar(255, 0, 0), thickness);
      circle(
          canvas,
          cv::Point2i(int(faces.at<float>(i, 6)), int(faces.at<float>(i, 7))),
          2, cv::Scalar(0, 0, 255), thickness);
      circle(
          canvas,
          cv::Point2i(int(faces.at<float>(i, 8)), int(faces.at<float>(i, 9))),
          2, cv::Scalar(0, 255, 0), thickness);
      circle(
          canvas,
          cv::Point2i(int(faces.at<float>(i, 10)), int(faces.at<float>(i, 11))),
          2, cv::Scalar(255, 0, 255), thickness);
      circle(
          canvas,
          cv::Point2i(int(faces.at<float>(i, 12)), int(faces.at<float>(i, 13))),
          2, cv::Scalar(0, 255, 255), thickness);

      // Add text label cleanly above the rectangle
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

public:
  FaceRecognitionEngine(const Config &config) {

    face_detector_ = cv::FaceDetectorYN::create(
        config.face_detection_model.data(), "", cv::Size(640, 480),
        config.confidence_threshold, config.non_max_suppression,
        config.max_detections);

    face_recognizer_ =
        cv::FaceRecognizerSF::create(config.face_recognition_model.data(), "");

    get_all_reference_facial_signatures(config);
  }

  FacesData get_faces_from_input(cv::Mat input_data, const Config &config) {
    FacesData result;
    result.input_image = input_data.clone();
    // Detect Faces in Input Picture
    result.faces = detect_faces(result.input_image);
    result.matched_face.assign(result.faces.rows, false);
    bool any_match_found = false;
    // Go thought all detected faces
    for (int index : std::views::iota(0, result.faces.rows)) {
      // Normalize input face data
      cv::Mat input_data_face_feature =
          get_facial_features(result.faces, index, input_data);
      bool is_match = false;
      // Compare face to reference picture faces
      for (const auto &ref_face_sig : reference_face_signatures_) {
        double score =
            face_recognizer_->match(ref_face_sig, input_data_face_feature);
        if (score >= config.confidence_threshold) {
          result.matched_face[index] = true;
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

  void visualize_references() {
    if (reference_faces_.empty() || reference_image_.empty()) {
      std::println("==== WARNING: Reference assets are empty! ====");
      return;
    }

    // Call the shared private drawing logic
    draw_face_annotations(reference_image_, reference_faces_);

    // Show in window
    cv::namedWindow("Reference Pic", cv::WINDOW_NORMAL);
    cv::imshow("Reference Pic", reference_image_);
  }

  void visualize(FacesData &face_data) {
    draw_face_annotations(face_data.input_image, face_data.faces,
                          &face_data.matched_face);

    cv::namedWindow("Camera Pic", cv::WINDOW_NORMAL);
    cv::imshow("Camera Pic", face_data.input_image);
  }
};

int main(int argc, char **argv) {

  Config config;

  CLI::App app{"Face Locker"};
  app.add_option("-r,--reference_picture", config.reference_picture_path,
                 "Reference Picture")
      ->required();

  app.add_flag("-d,--debug", config.debug_mode, "Debug Mode")
      ->default_val(false);

  CLI11_PARSE(app, argc, argv);

  double cosine_similar_thresh = 0.363;
  double l2norm_similar_thresh = 1.128;

  FaceRecognitionEngine face_recognizer(config);

  auto cam_img_raw = capture_from_webcam(0);

  auto face_data =
      face_recognizer.get_faces_from_input(cam_img_raw.value(), config);

  if (config.debug_mode) {
    face_recognizer.visualize_references();
    face_recognizer.visualize(face_data);
  }

  cv::waitKey(0);
}
