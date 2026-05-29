# ==============================================================================
# 1. Header-Only Data Types Library
# ==============================================================================
add_library(data_types INTERFACE)

target_include_directories(data_types INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(data_types INTERFACE opencv_core)

# ==============================================================================
# 2. Static Utilities Library
# ==============================================================================
add_library(Utils STATIC utils.cpp)

# Ensure Utils can find its own local headers
target_include_directories(Utils PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# FIX: Grant the Utils target access to the raw building OpenCV headers
target_include_directories(Utils PUBLIC
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/include"
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/core/include"
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/imgproc/include"
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/highgui/include"
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/videoio/include"
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/imgcodecs/include"
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/dnn/include"
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/objdetect/include"
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/video/include"
    "${CMAKE_BINARY_DIR}" # Crucial for generated opencv2/opencv_modules.hpp
)

# Force the build system to generate OpenCV headers before building utils.cpp
add_dependencies(Utils
    opencv_core
    opencv_videoio
    opencv_objdetect
    opencv_highgui
    opencv_imgproc
)

# Link dependencies to the target
target_link_libraries(Utils PUBLIC
    data_types
    opencv_core
    opencv_videoio
    opencv_objdetect
    opencv_highgui
    opencv_imgproc
)
