#pragma once

#include "data_types.h"
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <optional>
#include <print>
#include <ranges>
#include <vector>

class FaceRecognitionEngine {
private:
  std::shared_ptr<cv::FaceDetectorYN> face_detector_;
  std::shared_ptr<cv::FaceRecognizerSF> face_recognizer_;
  cv::Mat reference_image_;
  cv::Mat reference_faces_;
  std::vector<cv::Mat> reference_face_signatures_;

  cv::Mat detect_faces(const cv::Mat &image);
  cv::Mat get_facial_features(cv::Mat faces1, int facenum, cv::Mat image1);
  std::optional<std::vector<cv::Mat>>
  get_all_reference_facial_signatures(const Config &config);
  void draw_face_annotations(cv::Mat &canvas, const cv::Mat &faces,
                             const std::vector<int> *matched_indices = nullptr);

public:
  FaceRecognitionEngine(const Config &config);
  FacesData get_faces_from_input(cv::Mat input_data, const Config &config);
  void visualize_references();
  void visualize(FacesData &face_data);
};
