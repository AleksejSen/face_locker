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

int main(int argc, char **argv) {

  auto [img1_name, img2_name] = parse_args(argc, argv);

  std::print("Comparing images: {} and {}\n", img1_name, img2_name);

  // Read the images
  cv::Mat image1 = cv::imread(img1_name);
  cv::Mat image2 = cv::imread(img2_name);

  return 0;
}
