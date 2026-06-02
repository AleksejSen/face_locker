#include "face_engine.h"
#include <gtest/gtest.h>
#include <opencv2/core.hpp>

class FaceEngineCTest : public ::testing::Test {
protected:
  Config config;

  void SetUp() override {
    // Fix: Route paths relative to the build directory execution path
    config.reference_picture_path = "../non_existent_file.jpg";

    // Ensure you use absolute paths or step out to root for the models
    // Assuming your 'models/' folder is in your repository root directory:
    // config.face_detection_model =
    // "../models/face_detection_yunet_2023mar.onnx";
    // config.face_recognition_model =
    // "../models/face_recognition_sface_2021dec.onnx";

    config.confidence_threshold = 0.6f;
  }
};

// Test 1: Verify the constructor doesn't crash when files are missing
TEST_F(FaceEngineCTest, HandlesMissingFilesGracefully) {
  EXPECT_NO_THROW({ FaceRecognitionEngine engine(config); });
}

// Test 2: Verify the engine can safely process a blank matrix frame
TEST_F(FaceEngineCTest, HandlesBlankMatrices) {
  FaceRecognitionEngine engine(config);
  cv::Mat empty_frame; // 0x0 empty matrix

  FacesData result = engine.recognize_faces(empty_frame, config);

  // Verify properties scale down gracefully to zero safely
  EXPECT_EQ(result.faces.rows, 0);
  EXPECT_TRUE(result.matched_face.empty());
  EXPECT_TRUE(result.input_image.empty());
}
