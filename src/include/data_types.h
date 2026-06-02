#pragma once

#include "opencv2/core/mat.hpp"
#include <vector>

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
  int interval = 10;
};

struct FacesData {
  std::vector<int> matched_face;
  cv::Mat input_image;
  cv::Mat faces;
};
