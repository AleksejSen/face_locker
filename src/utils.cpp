#include "utils.h"
#include "opencv2/core/mat.hpp"
#include <csignal>
#include <iostream>
#include <opencv2/core/types.hpp>
#include <opencv2/dnn/dnn.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect/face.hpp>

namespace Utils {

std::atomic<bool> service_running{true};
std::condition_variable cv_sleep;
std::mutex cv_mtx;

bool interruptible_sleep(std::chrono::milliseconds duration) {
  std::unique_lock<std::mutex> lock(cv_mtx);
  return !cv_sleep.wait_for(lock, duration,
                            [] { return !service_running.load(); });
}

void signal_handler(int signal) {
  if (signal == SIGTERM || signal == SIGINT) {
    std::cout << "\n[Face Locker] Shutdown signal received. Stopping ..."
              << std::endl;
    service_running = false;
    cv_sleep.notify_all(); // Instantly wake up the main thread from any active
                           // sleep
  }
}

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

bool is_screen_already_locked() {
  std::string cmd =
      "if pgrep -x \"swaylock\" >/dev/null || pgrep -x \"i3lock\" >/dev/null "
      "|| pgrep -x \"hyprlock\" >/dev/null; then echo \"yes\"; "
      "elif command -v gnome-screensaver-command >/dev/null && "
      "gnome-screensaver-command -q 2>&1 | grep -q \"is active\"; then echo "
      "\"yes\"; "
      "elif command -v qdbus >/dev/null && qdbus org.freedesktop.ScreenSaver "
      "/ScreenSaver org.freedesktop.ScreenSaver.GetActive 2>/dev/null | grep "
      "-q \"true\"; then echo \"yes\"; "
      "else "
      "  session_id=$(loginctl session-status | head -n 1 | awk '{print $1}'); "
      "  if [ -z \"$session_id\" ]; then session_id=\"self\"; fi; "
      "  loginctl show-session \"$session_id\" -p LockedHint | awk -F= '{print "
      "$2}'; "
      "fi";

  std::array<char, 128> buffer;
  std::string result;

  std::unique_ptr<FILE, PipeDeleter> pipe(popen(cmd.c_str(), "r"));
  if (!pipe)
    return false;

  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }

  return (result.find("yes") != std::string::npos ||
          result.find("true") != std::string::npos);
}

void trigger_screen_lock() {
  std::cout << "[Face Locker] Intruder or absence detected! Locking screen..."
            << std::endl;

  std::string lock_cmd =
      "export DISPLAY=${DISPLAY:-:0}; "
      "if [ -z \"$WAYLAND_DISPLAY\" ] && [ -e \"$XDG_RUNTIME_DIR/wayland-0\" "
      "]; then export WAYLAND_DISPLAY=wayland-0; fi; "
      "if command -v swaylock >/dev/null; then swaylock -f -c 000000; "
      "elif command -v i3lock >/dev/null; then i3lock; "
      "elif command -v xdg-screensaver >/dev/null; then xdg-screensaver lock; "
      "elif [ \"$XDG_CURRENT_DESKTOP\" = \"GNOME\" ]; then "
      "gnome-screensaver-command -l; "
      "elif [ \"$XDG_CURRENT_DESKTOP\" = \"KDE\" ]; then qdbus "
      "org.freedesktop.ScreenSaver /ScreenSaver Lock; "
      "else loginctl lock-session; "
      "fi";

  int ret = std::system(lock_cmd.c_str());
  (void)ret;
}

// Helper to strip quotes and whitespace from config values
std::string clean_config_val(const std::string &val) {
  std::string cleaned = val;
  cleaned.erase(0, cleaned.find_first_not_of(" \t\"'"));
  cleaned.erase(cleaned.find_last_not_of(" \t\"'") + 1);
  return cleaned;
}
} // namespace Utils
