#pragma once

#include "opencv2/core/mat.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>

namespace Utils {

// Declare global synchronization variables as extern (shared variables)
extern std::atomic<bool> service_running;
extern std::condition_variable cv_sleep;
extern std::mutex cv_mtx;

// Declare your helper functions
bool interruptible_sleep(std::chrono::milliseconds duration);
void signal_handler(int signal);

struct PipeDeleter {
  void operator()(FILE *pipe) const {
    if (pipe)
      pclose(pipe);
  }
};

std::optional<cv::Mat> capture_from_webcam(int camera_index = 0);

bool is_screen_already_locked();

void trigger_screen_lock();

std::string clean_config_val(const std::string &val);
} // namespace Utils
