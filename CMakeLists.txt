cmake_minimum_required(VERSION 3.15)
project(FaceLocker VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set OpenCV configuration options
set(BUILD_opencv_dnn ON CACHE BOOL "Enable DNN" FORCE)
set(WITH_EIGEN OFF CACHE BOOL "Disable Eigen" FORCE)
set(WITH_FFMPEG OFF CACHE BOOL "Disable FFMPEG" FORCE)
set(WITH_GTK ON CACHE BOOL "Enable GTK" FORCE)
set(WITH_WIN32UI OFF CACHE BOOL "Disable GUI" FORCE)
set(WITH_QT OFF CACHE BOOL "Disable Qt" FORCE)
set(WITH_QUIRC OFF CACHE BOOL "Disable Quirc" FORCE)
set(WITH_V4L OFF CACHE BOOL "Disable Video4Linux" FORCE)
set(WITH_WAYLAND OFF CACHE BOOL "Disable Wayland" FORCE)
set(WITH_WEBP OFF CACHE BOOL "Disable WebP" FORCE)
set(BUILD_TESTS OFF CACHE INTERNAL "")
set(BUILD_PERF_TESTS OFF CACHE INTERNAL "")
set(BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(BUILD_opencv_apps OFF CACHE INTERNAL "")
set(BUILD_DOCS OFF CACHE INTERNAL "")
set(WITH_IPP OFF CACHE INTERNAL "")
# Add OpenCV
add_subdirectory(extern_libs/opencv)
include_directories(extern_libs/opencv/include)

# Add CLI11 arg parser
add_subdirectory(extern_libs/cli11)

# Enable CTest infrastructure
enable_testing()

# add_subdirectory(src)
add_subdirectory(app)


