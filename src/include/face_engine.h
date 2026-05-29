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

  /**
   * @brief Detects and recognizes faces in an input image against pre-loaded
   * references.
   *
   * This function executes the main face recognition pipeline:
   * 1. Clones the input image to avoid modifying original frame buffers.
   * 2. Detects bounding boxes and facial landmark coordinates using YuNet.
   * 3. Extracts distinct facial feature embeddings for each discovered face via
   * SFace.
   * 4. Evaluates extracted features against the engine's stored reference
   * signatures.
   *
   * @param input_data The source frame matrix (typically from a webcam capture
   * or disk).
   * @param config The runtime configuration containing thresholds and model
   * specs.
   * @return A populated FacesData structure containing the cloned image, face
   * coordinates, and a mapping array of match results.
   */
  FacesData recognize_faces(cv::Mat input_data, const Config &config);

  void visualize_references();
  void visualize(FacesData &face_data);
};
