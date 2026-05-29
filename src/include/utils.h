#pragma once
#include "opencv2/core/mat.hpp"
#include <optional>

std::optional<cv::Mat> capture_from_webcam(int camera_index = 0);
