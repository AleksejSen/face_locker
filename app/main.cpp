#include "data_types.h"
#include "face_engine.h"
#include "utils.h"
#include <CLI/CLI.hpp>
#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

int main(int argc, char **argv) {
  Config config;
  namespace fs = std::filesystem;

  // 1. Resolve configuration path targeting ~/.config/face-locker/config.env
  std::string home_dir = std::getenv("HOME") ? std::getenv("HOME") : "";
  fs::path config_dir = fs::path(home_dir) / ".config" / "face-locker";
  fs::path config_file = config_dir / "config.env";

  // 2. Automatically generate path structures if missing on startup
  if (!fs::exists(config_file)) {
    try {
      fs::create_directories(config_dir);
      std::ofstream out(config_file);
      if (out.is_open()) {
        out << "# Face Locker Configuration File\n";
        out << "REFERENCE_PICTURE=\"\"\n";
        out << "INTERVAL=10\n";
        out << "DEBUG_MODE=false\n";
        out.close();
        std::cout << "[Face Locker] Created default config at: "
                  << config_file.string() << std::endl;
      }
    } catch (const fs::filesystem_error &e) {
      std::cerr
          << "[Face Locker] Filesystem error while generating default config: "
          << e.what() << std::endl;
    }
  } else {
    // 3. File exists: Read variables into the memory config struct
    std::ifstream in(config_file);
    std::string line;
    while (std::getline(in, line)) {
      if (line.empty() || line[0] == '#')
        continue;
      std::size_t delim = line.find('=');
      if (delim == std::string::npos)
        continue;

      std::string key = line.substr(0, delim);
      std::string val = Utils::clean_config_val(line.substr(delim + 1));

      if (key == "REFERENCE_PICTURE")
        config.reference_picture_path = val;
      else if (key == "INTERVAL")
        config.interval = std::stoi(val);
      else if (key == "DEBUG_MODE")
        config.debug_mode = (val == "true" || val == "1");
    }
  }

  // 4. Setup CLI Options (Notice ->required() is removed to allow file
  // configuration)
  CLI::App app{"Face Locker"};

  app.add_option("-r,--reference_picture", config.reference_picture_path,
                 "Reference Picture Path");

  // Keep defaults dynamic to show what values were extracted from file
  app.add_option("-i,--interval", config.interval, "Face Check Interval")
      ->default_val(config.interval);

  app.add_flag("-d,--debug", config.debug_mode, "Debug Mode");

  CLI11_PARSE(app, argc, argv);

  // 5. Post-Parse Validation: Validate that we have a picture path from
  // somewhere
  if (config.reference_picture_path.empty()) {
    std::cerr
        << "Error: No reference picture specified!\n"
        << "Please pass -r/--reference_picture or set REFERENCE_PICTURE in "
        << config_file.string() << std::endl;
    return EXIT_FAILURE;
  }

  // Ensure the configured file actually exists before starting webcam engine
  if (!fs::exists(config.reference_picture_path)) {
    std::cerr << "Error: Reference picture path does not exist: "
              << config.reference_picture_path << std::endl;
    return EXIT_FAILURE;
  }

  // Rest of your initialization and interruptible execution loop continues
  // here...

  std::signal(SIGINT, Utils::signal_handler);
  std::signal(SIGTERM, Utils::signal_handler);

  FaceRecognitionEngine face_recognizer(config);
  std::cout << "[Face Locker] Safe background daemon initialized." << std::endl;

  while (Utils::service_running) {
    if (Utils::is_screen_already_locked()) {
      std::cout << "[Face Locker] Screen Already Locked ..." << std::endl;
      // Explicitly updated to milliseconds to match your helper's parameter
      if (!Utils::interruptible_sleep(std::chrono::milliseconds(2000)))
        break;
      continue;
    }

    auto cam_img_raw = Utils::capture_from_webcam(0);
    if (!cam_img_raw.has_value()) {
      std::cerr << "[Face Locker] Camera capture dropped. Retrying..."
                << std::endl;
      // Explicitly updated to milliseconds to match your helper's parameter
      if (!Utils::interruptible_sleep(std::chrono::milliseconds(3000)))
        break;
      continue;
    }

    auto recognition_results =
        face_recognizer.recognize_faces(cam_img_raw.value(), config);

    if (config.debug_mode) {
      face_recognizer.visualize_references();
      face_recognizer.visualize(recognition_results);

      // Initial responsive check for instant keypress actions
      if (cv::waitKey(1) >= 0) {
        break;
      }
    }

    bool access_granted = std::ranges::any_of(
        recognition_results.matched_face, [](int idx) { return idx != -1; });

    if (!access_granted) {
      Utils::trigger_screen_lock();
      if (!Utils::interruptible_sleep(std::chrono::milliseconds(2000)))
        break;
      continue; // Skip the interval delay since we just locked the screen
    }

    auto total_sleep_ms = config.interval * 1000;
    auto elapsed_ms = 0;
    bool quit_requested = false;

    while (elapsed_ms < total_sleep_ms && Utils::service_running) {
      if (config.debug_mode) {
        // Pump events to make the window render and stay responsive
        if (cv::waitKey(100) >= 0) {
          quit_requested = true;
          break;
        }
      } else {
        if (!Utils::interruptible_sleep(std::chrono::milliseconds(100))) {
          break;
        }
      }
      elapsed_ms += 100;
    }

    if (quit_requested || !Utils::service_running) {
      break;
    }
  }

  std::cout << "[Face Locker] Clean shutdown completed." << std::endl;
  return EXIT_SUCCESS;
}
