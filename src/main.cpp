#include <cstdlib>
#include <opencv2/dnn/dnn.hpp>
#include <opencv2/objdetect/face.hpp>
#include <opencv2/opencv.hpp>
#include <print>
#include <string>
#include <utility>

std::pair<std::string, std::string> parse_args(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <image1> <image2>" << std::endl;
    exit(1);
  }

  return {argv[1], argv[2]};
}

cv::Mat detect_faces(const cv::Mat &image) {

  float threshold = 0.9f;
  // Used for bounding box suppression
  float nms_threshold = 0.3f;
  // Keep this many bounding boxes
  float top_k = 5000;

  constexpr auto fd_modelPath = "models/face_detection_yunet_2023mar.onnx";

  // TODO: Do I need one detector per image?
  auto detector_1 = cv::FaceDetectorYN::create(fd_modelPath, "", image.size(),
                                               threshold, nms_threshold, top_k);

  cv::Mat faces;
  detector_1->detect(image, faces);
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

// DNN tutorial: https://docs.opencv.org/4.x/d0/dd4/tutorial_dnn_face.html
int main(int argc, char **argv) {

  double cosine_similar_thresh = 0.363;
  double l2norm_similar_thresh = 1.128;

  auto [img1_name, img2_name] = parse_args(argc, argv);

  std::print("Comparing images: {} and {}\n", img1_name, img2_name);

  // Read the images
  cv::Mat image1 = cv::imread(img1_name);
  cv::Mat image2 = cv::imread(img2_name);

  auto faces1 = detect_faces(image1);
  if (faces1.empty()) {
    std::print("No faces found in image 1. Aborting.\n");
    return EXIT_FAILURE;
  }
  std::print("Found {} faces in image 1\n", faces1.rows);
  for (int i = 0; i < faces1.rows; ++i) {
  }

  auto faces2 = detect_faces(image2);
  if (faces2.empty()) {
    std::print("No faces found in image 2. Aborting.\n");
    return EXIT_FAILURE;
  }
  std::print("Found {} faces in image 2\n", faces2.rows);
  for (int i = 0; i < faces1.rows; ++i) {
    std::print("Face {}: {} x {} x {}\n", i, faces1.at<float>(i, 0),
               faces1.at<float>(i, 1), faces1.at<float>(i, 2));
  }

  auto recognizer = cv::FaceRecognizerSF::create(
      "models/face_recognition_sface_2021dec.onnx", "");

  for (auto i = 0; i < faces1.rows; ++i) {
    const auto feature1 = get_facial_features(faces1, i, image1, recognizer);

    std::print("Looking for face {}-{}:{}x{}x{} in {}\n", img1_name, i,
               faces1.at<float>(i, 0), faces1.at<float>(i, 1),
               faces1.at<float>(i, 2), img2_name);

    for (auto j = 0; j < faces2.rows; ++j) {
      std::print("  Comparing with {}-{}:{}x{}x{} in {}\n", img2_name, j,
                 faces2.at<float>(j, 0), faces2.at<float>(j, 1),
                 faces2.at<float>(j, 2), img1_name);

      const auto feature2 = get_facial_features(faces2, j, image2, recognizer);

      // Run feature extraction with given aligned_face
      double cos_score = recognizer->match(
          feature1, feature2, cv::FaceRecognizerSF::DisType::FR_COSINE);
      double L2_score = recognizer->match(
          feature1, feature2, cv::FaceRecognizerSF::DisType::FR_NORM_L2);

      std::print("    Cosine score: {} (threshold {}). Similar: {}\n",
                 cos_score, cosine_similar_thresh,
                 cos_score >= cosine_similar_thresh);
      std::print("    L2 score: {} (threshold {}). Similar: {}\n", L2_score,
                 l2norm_similar_thresh, L2_score <= l2norm_similar_thresh);
    }
  }

  return EXIT_SUCCESS;
}
