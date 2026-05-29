#include "data_types.h"
#include "face_engine.h"
#include "opencv2/core/mat.hpp"
#include "utils.h"
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
