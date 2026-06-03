#pragma once
#include "opencv2/core/mat.hpp"
#include <span>
#include <vector>

// These headers are automatically found via target_include_directories
#include "face_detection_model.h"
#include "face_recognition_model.h"

struct Config {
  // Compile-time byte buffers pointing directly inside your binary data segment
  static constexpr std::span<const unsigned char> face_detection_model{
      face_detection_data, face_detection_size};

  static constexpr std::span<const unsigned char> face_recognition_model{
      face_recognition_data, face_recognition_size};

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
