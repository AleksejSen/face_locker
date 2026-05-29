#include "utils.h"
#include "opencv2/core/mat.hpp"
#include <iostream>
#include <opencv2/core/types.hpp>
#include <opencv2/dnn/dnn.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect/face.hpp>

// Returns a cv::Mat on success, or std::nullopt if the camera fails
std::optional<cv::Mat> capture_from_webcam(int camera_index) {
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
