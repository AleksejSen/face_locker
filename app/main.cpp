#include "opencv2/core/mat.hpp"
#include <CLI/CLI.hpp>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
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
#include <tuple>
#include <unordered_set>

cv::Mat detect_faces(const std::shared_ptr<cv::FaceDetectorYN> face_detector,
                     const cv::Mat &image) {

  cv::Mat faces;
  face_detector->setInputSize(image.size());
  face_detector->detect(image, faces);
  return faces;
}

cv::Mat get_facial_features(cv::Mat faces1, int facenum, cv::Mat image1,
                            std::shared_ptr<cv::FaceRecognizerSF> recognizer) {
  cv::Mat aligned_face1;
  recognizer->alignCrop(image1, faces1.row(facenum), aligned_face1);
  cv::Mat feature1;
  recognizer->feature(aligned_face1, feature1);
  return feature1.clone();
}

static void visualize(cv::Mat &input, cv::Mat &faces,
                      std::unordered_set<int> matches = {}) {

  const int thickness = 2;

  for (int i = 0; i < faces.rows; i++) {
    auto box_color = cv::Scalar(0, 0, 255);
    if (matches.find(i) != matches.end()) {
      box_color = cv::Scalar(0, 255, 0);
    }
    // Draw bounding box
    cv::Rect face_rect(faces.at<float>(i, 0), faces.at<float>(i, 1),
                       faces.at<float>(i, 2), faces.at<float>(i, 3));

    cv::rectangle(input, face_rect, box_color, 10); // Draw landmarks
    circle(input,
           cv::Point2i(int(faces.at<float>(i, 4)), int(faces.at<float>(i, 5))),
           2, cv::Scalar(255, 0, 0), thickness);
    circle(input,
           cv::Point2i(int(faces.at<float>(i, 6)), int(faces.at<float>(i, 7))),
           2, cv::Scalar(0, 0, 255), thickness);
    circle(input,
           cv::Point2i(int(faces.at<float>(i, 8)), int(faces.at<float>(i, 9))),
           2, cv::Scalar(0, 255, 0), thickness);
    circle(
        input,
        cv::Point2i(int(faces.at<float>(i, 10)), int(faces.at<float>(i, 11))),
        2, cv::Scalar(255, 0, 255), thickness);
    circle(
        input,
        cv::Point2i(int(faces.at<float>(i, 12)), int(faces.at<float>(i, 13))),
        2, cv::Scalar(0, 255, 255), thickness);

    // Add text under the rectangle
    // std::string text = "Face " + std::to_string(i);
    // int font_face = cv::FONT_HERSHEY_SIMPLEX;
    // double font_scale = 0.5;
    // int thickness = 1;
    // int baseline = 0;
    // cv::Size text_size =
    //     cv::getTextSize(text, font_face, font_scale, thickness, &baseline);
    // cv::Point text_org(face_rect.x,
    //                    face_rect.y + face_rect.height + text_size.height +
    //                    5);
    // cv::putText(input, text, text_org, font_face, font_scale,
    //             cv::Scalar(0, 255, 0), thickness);
  }
}

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

class FaceEngine {
private:
  std::shared_ptr<cv::FaceDetectorYN> face_detector_;
  std::shared_ptr<cv::FaceRecognizerSF> face_recognizer_;

public:
  // TODO: Continue on Face engine
  FaceEngine(const Config &config)
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

  auto face_detector = cv::FaceDetectorYN::create(
      config.face_detection_model.data(), "", cv::Size(640, 480),
      config.confidence_threshold, config.non_max_suppression,
      config.max_detections);

  auto face_recognizer =
      cv::FaceRecognizerSF::create(config.face_recognition_model.data(), "");

  // Get reference picture
  std::println("=== Ref pic path:{}", config.reference_picture_path);
  cv::Mat reference_face_pic = cv::imread(config.reference_picture_path);

  // Detect Ref Face
  auto reference_faces = detect_faces(face_detector, reference_face_pic);
  // Structure to hold detected faces features
  // Faces need to be 'normalized' so thay can be easily compared depending on
  // their angle, position from camera etc
  std::vector<cv::Mat> reference_face_signatures;
  if (!reference_faces.empty()) {
    reference_face_signatures.reserve(reference_faces.rows);

    for (int i : std::views::iota(0, reference_faces.rows)) {
      // Get face feature
      cv::Mat face_feature = get_facial_features(
          reference_faces, i, reference_face_pic, face_recognizer);
      // Save face feature
      reference_face_signatures.push_back(face_feature);
    }
  } else {
    std::println("==== No Reference Face Found. Aborting. ====");
    return EXIT_FAILURE;
  }

  std::println("==== Reference Face Found:{} | Extracted {} ====",
               reference_faces.rows, reference_face_signatures.size());

  auto cam_img_raw = capture_from_webcam(0);

  if (!cam_img_raw.has_value()) {
    return EXIT_FAILURE;
  }
  cv::Mat cam_img = cam_img_raw.value();

  cv::Mat cam_faces = detect_faces(face_detector, cam_img);
  std::unordered_set<int> matched_face_indices;
  for (int i : std::views::iota(0, cam_faces.rows)) {
    cv::Mat cam_face_feature =
        get_facial_features(cam_faces, i, cam_img, face_recognizer);
    bool is_match = false;
    for (const auto &ref_face_sig : reference_face_signatures) {
      // Calculate similarity
      double score = face_recognizer->match(ref_face_sig, cam_face_feature);
      std::println("==== Match index:{} ====", score);
      if (score >= cosine_similar_thresh) {
        std::println("==== Face matches !====");
        is_match = true;
        break;
      }
    }
    if (is_match) {
      matched_face_indices.insert(i);
    }

    if (config.debug_mode) {
      visualize(cam_img, cam_faces, matched_face_indices);
    }
  }

  if (config.debug_mode) {
    cv::namedWindow("Reference Pic", cv::WINDOW_NORMAL);
    cv::namedWindow("Camera Pic", cv::WINDOW_NORMAL);
    cv::imshow("Reference Pic", reference_face_pic);
    cv::imshow("Camera Pic", cam_img);
  }

  cv::waitKey(0);
}
