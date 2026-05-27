#include "opencv2/core/mat.hpp"
#include <CLI/CLI.hpp>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <opencv2/core/types.hpp>
#include <opencv2/dnn/dnn.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect/face.hpp>
#include <print>
#include <string>
#include <tuple>
#include <unordered_set>

constexpr auto FD_MODEL_PATH = "models/face_detection_yunet_2023mar.onnx";

const std::string FR_MODEL_PATH = "models/face_recognition_sface_2021dec.onnx";

const float THRESHOLD = 0.9f;
// Used for bounding box suppression
const float NMS_THRESHOLD = 0.3f;
// Keep this many bounding boxes
const float TOP_K = 5000;

// Convenience macro for fairly decent timing
#define TIME_MEASURE_START(name)                                               \
  auto name##_start = std::chrono::high_resolution_clock::now();
#define TIME_MEASURE_END(name)                                                 \
  auto name##_end = std::chrono::high_resolution_clock::now();                 \
  auto name##_duration =                                                       \
      std::chrono::duration_cast<std::chrono::milliseconds>(name##_end -       \
                                                            name##_start);     \
  std::print("  🕐 {} took {} ms\n", #name, name##_duration.count());

enum class Mode { Normal, Demo, Search };

std::tuple<Mode, std::string, std::string> parse_args(int argc, char **argv) {
  Mode mode = Mode::Normal;
  int base = 0;
  if (static_cast<std::string>(argv[1]) == "-d") {
    std::cout << "demo mode\n";
    mode = Mode::Demo;
    base += 1;
  }

  if (static_cast<std::string>(argv[1]) == "-s") {
    std::cout << "search mode\n";
    mode = Mode::Search;
    base += 1;
  }

  if (argc != (base + 3)) {
    std::cerr << "Usage: " << argv[0] << " <image1> <image2>" << std::endl;
    exit(1);
  }

  return {mode, argv[base + 1], argv[base + 2]};
}

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
    auto box_color = cv::Scalar(0, 255, 0);
    if (matches.find(i) != matches.end()) {
      box_color = cv::Scalar(0, 0, 255);
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
    std::string text = "Face " + std::to_string(i);
    int font_face = cv::FONT_HERSHEY_SIMPLEX;
    double font_scale = 0.5;
    int thickness = 1;
    int baseline = 0;
    cv::Size text_size =
        cv::getTextSize(text, font_face, font_scale, thickness, &baseline);
    cv::Point text_org(face_rect.x,
                       face_rect.y + face_rect.height + text_size.height + 5);
    cv::putText(input, text, text_org, font_face, font_scale,
                cv::Scalar(0, 255, 0), thickness);
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

// DNN tutorial: https://docs.opencv.org/4.x/d0/dd4/tutorial_dnn_face.html
int main(int argc, char **argv) {

  std::string ref_picture_path;
  bool debug;

  CLI::App app{"Face Locker"};
  app.add_option("-r,--reference_picture", ref_picture_path,
                 "Reference Picture")
      ->required();

  app.add_flag("-d,--debug", debug, "Debug Mode")->default_val(false);

  CLI11_PARSE(app, argc, argv);

  double cosine_similar_thresh = 0.363;
  double l2norm_similar_thresh = 1.128;

  // Get refrence picture
  cv::Mat image1 = cv::imread(ref_picture_path);
  cv::namedWindow("Reference Pic", cv::WINDOW_NORMAL);

  // Get PC Cam image
  auto cam_img_raw = capture_from_webcam(0);

  if (!cam_img_raw.has_value()) {
    return EXIT_FAILURE;
  }
  cv::Mat cam_img = cam_img_raw.value();
  cv::namedWindow("Camera Pic", cv::WINDOW_NORMAL);

  // TODO:
  // 1. Add face face recognition
  // 2. Compare faces

  if (debug) {
    cv::imshow("Reference Pic", image1);
    cv::imshow("Camera Pic", cam_img);
  }

  cv::waitKey(0);

  // OLD CODE

  // auto [mode, img1_name, img2_name] = parse_args(argc, argv);

  // std::print("Comparing images: {} and {}\n", img1_name, img2_name);

  // Read the images
  // cv::Mat image1 = cv::imread(ref_picture_path);
  // cv::Mat image2 = cv::imread(img2_name);

  // if (mode == Mode::Demo) {
  //   cv::namedWindow("Image 1", cv::WINDOW_NORMAL);
  //   cv::namedWindow("Image 2", cv::WINDOW_NORMAL);
  //
  //   imshow("Image 1", image1);
  //   imshow("Image 2", image2);
  //   cv::waitKey(0);
  // }
  //
  // auto face_detector = cv::FaceDetectorYN::create(
  //     FD_MODEL_PATH, "", cv::Size(640, 480), THRESHOLD, NMS_THRESHOLD,
  //     TOP_K);
  //
  // TIME_MEASURE_START(face_detect_image1)
  // auto faces1 = detect_faces(face_detector, image1);
  // TIME_MEASURE_END(face_detect_image1)
  // if (faces1.empty()) {
  //   std::print("No faces found in image 1. Aborting.\n");
  //   return EXIT_FAILURE;
  // }
  // std::print("Found {} faces in image 1\n", faces1.rows);
  //
  // if (mode == Mode::Search) {
  //   // extract folder from image1,
  //   std::print("Search mode\n");
  //   std::filesystem::path reference_pic =
  //       std::filesystem::path(static_cast<std::string>(img1_name));
  //   std::filesystem::path paretn_dir = reference_pic.parent_path();
  //   std::print("Reference Image: {}, Parent dir: {}\n",
  //   reference_pic.string(),
  //              paretn_dir.string());
  //   std::unordered_set<std::filesystem::path> simmilar_images;
  //
  //   // Iterate thought all the images in parent dir
  //   for (const auto &entry : std::filesystem::directory_iterator(paretn_dir))
  //   {
  //     if (entry != reference_pic) {
  //       cv::Mat image2 = cv::imread(entry.path().string());
  //       TIME_MEASURE_START(face_detect_image2)
  //       auto faces2 = detect_faces(face_detector, image2);
  //       TIME_MEASURE_END(face_detect_image2)
  //
  //       if (faces2.empty()) {
  //         std::print("No faces found in image 2. Aborting.\n");
  //         return EXIT_FAILURE;
  //       }
  //       std::print("Found {} faces in image 2\n", faces2.rows);
  //
  //       auto face_recognizer = cv::FaceRecognizerSF::create(FR_MODEL_PATH,
  //       "");
  //
  //       std::unordered_set<int> match_set1;
  //       std::unordered_set<int> match_set2;
  //
  //       for (auto i = 0; i < faces1.rows; ++i) {
  //         TIME_MEASURE_START(face_recog_image1)
  //         const auto feature1 =
  //             get_facial_features(faces1, i, image1, face_recognizer);
  //         TIME_MEASURE_END(face_recog_image1)
  //
  //         for (auto j = 0; j < faces2.rows; ++j) {
  //           TIME_MEASURE_START(face_recog_image2)
  //           const auto feature2 =
  //               get_facial_features(faces2, j, image2, face_recognizer);
  //           TIME_MEASURE_END(face_recog_image2)
  //
  //           // Run feature extraction with given aligned_face
  //           TIME_MEASURE_START(face_recog_match)
  //           double cos_score = face_recognizer->match(
  //               feature1, feature2,
  //               cv::FaceRecognizerSF::DisType::FR_COSINE);
  //           double L2_score = face_recognizer->match(
  //               feature1, feature2,
  //               cv::FaceRecognizerSF::DisType::FR_NORM_L2);
  //           TIME_MEASURE_END(face_recog_match)
  //
  //           bool is_match = cos_score >= cosine_similar_thresh &&
  //                           L2_score <= l2norm_similar_thresh;
  //
  //           if (is_match) {
  //             match_set1.insert(i);
  //             match_set2.insert(j);
  //
  //             simmilar_images.insert(entry);
  //           }
  //         }
  //       }
  //     }
  //   }
  //   std::print("Rerence Image:{}\n", reference_pic.string());
  //   std::print("Found Images with same faces:{}\n", simmilar_images.size());
  //   for (const auto &picture : simmilar_images) {
  //     std::print("{}", picture.string());
  //   }
  //   std::print("\n");
  //   return EXIT_SUCCESS;
  // }
  //
  // TIME_MEASURE_START(face_detect_image2)
  // auto faces2 = detect_faces(face_detector, image2);
  // TIME_MEASURE_END(face_detect_image2)
  // if (faces2.empty()) {
  //   std::print("No faces found in image 2. Aborting.\n");
  //   return EXIT_FAILURE;
  // }
  // std::print("Found {} faces in image 2\n", faces2.rows);
  //
  // auto face_recognizer = cv::FaceRecognizerSF::create(FR_MODEL_PATH, "");
  //
  // std::unordered_set<int> match_set1;
  // std::unordered_set<int> match_set2;
  //
  // for (auto i = 0; i < faces1.rows; ++i) {
  //   TIME_MEASURE_START(face_recog_image1)
  //   const auto feature1 =
  //       get_facial_features(faces1, i, image1, face_recognizer);
  //   TIME_MEASURE_END(face_recog_image1)
  //
  //   std::print("Looking for face {}-{}:{}x{}x{} in {}\n", img1_name, i,
  //              faces1.at<float>(i, 0), faces1.at<float>(i, 1),
  //              faces1.at<float>(i, 2), img2_name);
  //
  //   for (auto j = 0; j < faces2.rows; ++j) {
  //     std::print("  Comparing with {}-{}:{}x{}x{} in {}\n", img2_name, j,
  //                faces2.at<float>(j, 0), faces2.at<float>(j, 1),
  //                faces2.at<float>(j, 2), img1_name);
  //
  //     TIME_MEASURE_START(face_recog_image2)
  //     const auto feature2 =
  //         get_facial_features(faces2, j, image2, face_recognizer);
  //     TIME_MEASURE_END(face_recog_image2)
  //
  //     // Run feature extraction with given aligned_face
  //     TIME_MEASURE_START(face_recog_match)
  //     double cos_score = face_recognizer->match(
  //         feature1, feature2, cv::FaceRecognizerSF::DisType::FR_COSINE);
  //     double L2_score = face_recognizer->match(
  //         feature1, feature2, cv::FaceRecognizerSF::DisType::FR_NORM_L2);
  //     TIME_MEASURE_END(face_recog_match)
  //
  //     bool is_match = cos_score >= cosine_similar_thresh &&
  //                     L2_score <= l2norm_similar_thresh;
  //     if (is_match) {
  //       match_set1.insert(i);
  //       match_set2.insert(j);
  //     }
  //     std::print("    Cosine score: {} (threshold {}). Similar: {}\n",
  //                cos_score, cosine_similar_thresh,
  //                cos_score >= cosine_similar_thresh);
  //     std::print("    L2 score: {} (threshold {}). Similar: {}\n", L2_score,
  //                l2norm_similar_thresh, L2_score <= l2norm_similar_thresh);
  //   }
  // }
  //
  // if (mode == Mode::Demo) {
  //   auto modified_image1 = image1.clone();
  //   visualize(modified_image1, faces1, match_set1);
  //   auto modified_image2 = image2.clone();
  //   visualize(modified_image2, faces2, match_set2);
  //
  //   imshow("Image 1", modified_image1);
  //   imshow("Image 2", modified_image2);
  //   cv::waitKey(0);
  // }
  //
  // if (match_set1.empty()) {
  //   return EXIT_FAILURE;
  // } else {
  //   return EXIT_SUCCESS;
  // }
}
