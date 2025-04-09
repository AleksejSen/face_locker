#include <cstdlib>
#include <format>
#include <opencv2/core/types.hpp>
#include <opencv2/dnn/dnn.hpp>
#include <opencv2/objdetect/face.hpp>
#include <opencv2/opencv.hpp>
#include <print>
#include <string>
#include <unordered_set>
#include <utility>

constexpr auto FD_MODEL_PATH = "models/face_detection_yunet_2023mar.onnx";

const std::string FR_MODEL_PATH = "models/face_recognition_sface_2021dec.onnx";

const float THRESHOLD = 0.9f;
// Used for bounding box suppression
const float NMS_THRESHOLD = 0.3f;
// Keep this many bounding boxes
const float TOP_K = 5000;

std::pair<std::string, std::string> parse_args(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <image1> <image2>" << std::endl;
    exit(1);
  }

  return {argv[1], argv[2]};
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
    rectangle(input,
              cv::Rect2i(int(faces.at<float>(i, 0)), int(faces.at<float>(i, 1)),
                         int(faces.at<float>(i, 2)),
                         int(faces.at<float>(i, 3))),
              box_color, thickness);
    // Draw landmarks
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

    putText(input, std::format("{}", i),
            cv::Point2i(int(faces.at<float>(i, 4)), int(faces.at<float>(i, 5))),
            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
  }
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

  cv::namedWindow("Image 1", cv::WINDOW_NORMAL);
  cv::namedWindow("Image 2", cv::WINDOW_NORMAL);

  imshow("Image 1", image1);
  imshow("Image 2", image2);
  cv::waitKey(0);

  auto face_detector = cv::FaceDetectorYN::create(
      FD_MODEL_PATH, "", cv::Size(640, 480), THRESHOLD, NMS_THRESHOLD, TOP_K);

  auto faces1 = detect_faces(face_detector, image1);
  if (faces1.empty()) {
    std::print("No faces found in image 1. Aborting.\n");
    return EXIT_FAILURE;
  }
  std::print("Found {} faces in image 1\n", faces1.rows);

  auto faces2 = detect_faces(face_detector, image2);
  if (faces2.empty()) {
    std::print("No faces found in image 2. Aborting.\n");
    return EXIT_FAILURE;
  }
  std::print("Found {} faces in image 2\n", faces2.rows);

  auto face_recognizer = cv::FaceRecognizerSF::create(FR_MODEL_PATH, "");

  std::unordered_set<int> match_set1;
  std::unordered_set<int> match_set2;

  for (auto i = 0; i < faces1.rows; ++i) {
    const auto feature1 =
        get_facial_features(faces1, i, image1, face_recognizer);

    std::print("Looking for face {}-{}:{}x{}x{} in {}\n", img1_name, i,
               faces1.at<float>(i, 0), faces1.at<float>(i, 1),
               faces1.at<float>(i, 2), img2_name);

    for (auto j = 0; j < faces2.rows; ++j) {
      std::print("  Comparing with {}-{}:{}x{}x{} in {}\n", img2_name, j,
                 faces2.at<float>(j, 0), faces2.at<float>(j, 1),
                 faces2.at<float>(j, 2), img1_name);

      const auto feature2 =
          get_facial_features(faces2, j, image2, face_recognizer);

      // Run feature extraction with given aligned_face
      double cos_score = face_recognizer->match(
          feature1, feature2, cv::FaceRecognizerSF::DisType::FR_COSINE);
      double L2_score = face_recognizer->match(
          feature1, feature2, cv::FaceRecognizerSF::DisType::FR_NORM_L2);

      bool is_match = cos_score >= cosine_similar_thresh &&
                      L2_score <= l2norm_similar_thresh;
      if (is_match) {
        match_set1.insert(i);
        match_set2.insert(j);
      }
      std::print("    Cosine score: {} (threshold {}). Similar: {}\n",
                 cos_score, cosine_similar_thresh,
                 cos_score >= cosine_similar_thresh);
      std::print("    L2 score: {} (threshold {}). Similar: {}\n", L2_score,
                 l2norm_similar_thresh, L2_score <= l2norm_similar_thresh);
    }
  }

  auto modified_image1 = image1.clone();
  visualize(modified_image1, faces1, match_set1);
  auto modified_image2 = image2.clone();
  visualize(modified_image2, faces2, match_set2);

  imshow("Image 1", modified_image1);
  imshow("Image 2", modified_image2);
  cv::waitKey(0);

  return EXIT_SUCCESS;
}
