# ==============================================================================
# 0. Compile-Time Resource Embedding (Baking Models Into Headers)
# ==============================================================================
set(EMBEDDED_HEADERS_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_includes")
file(MAKE_DIRECTORY "${EMBEDDED_HEADERS_DIR}")

# Function to transform a binary file into a C++ hex header array
function(embed_model_file INPUT_PATH OUTPUT_HEADER_NAME VARIABLE_PREFIX)
  if(NOT EXISTS "${INPUT_PATH}")
    message(FATAL_ERROR "Model file not found: ${INPUT_PATH}")
  endif()

  # Read binary file as a hex string
  file(READ "${INPUT_PATH}" HEX_CONTENT HEX)

  # Format hex string into C++ array comma-separated elements
  string(REGEX REPLACE "(..)" "0x\\1, " C_ARRAY "${HEX_CONTENT}")

  # Calculate total size in bytes (2 hex characters = 1 byte)
  string(LENGTH "${HEX_CONTENT}" HEX_LENGTH)
  math(EXPR ARRAY_LENGTH "${HEX_LENGTH} / 2")

  # Generate the header file content dynamically
  set(HEADER_CONTENT "/* Generated via CMake - Do Not Edit */
#pragma once
#include <cstddef>

inline constexpr unsigned char ${VARIABLE_PREFIX}_data[] = { ${C_ARRAY} };
inline constexpr std::size_t ${VARIABLE_PREFIX}_size = ${ARRAY_LENGTH};
")

  # Write code directly to the build directory
  file(GENERATE
        OUTPUT "${EMBEDDED_HEADERS_DIR}/${OUTPUT_HEADER_NAME}"
        CONTENT "${HEADER_CONTENT}"
    )
endfunction()

# Trigger generation using your exact model relative paths
embed_model_file("${CMAKE_SOURCE_DIR}/models/face_detection_yunet_2023mar.onnx" "face_detection_model.h" "face_detection")
embed_model_file("${CMAKE_SOURCE_DIR}/models/face_recognition_sface_2021dec.onnx" "face_recognition_model.h" "face_recognition")

# ==============================================================================
# 1. Header-Only Data Types Library
# ==============================================================================
add_library(data_types INTERFACE)

target_include_directories(data_types INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${EMBEDDED_HEADERS_DIR}> # Gives data_types access to generated headers
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(data_types INTERFACE opencv_core)

# ==============================================================================
# Shared Include Configuration for OpenCV Header Trees
# ==============================================================================
set(OPENCV_INTERNAL_INCLUDES
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/include"
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/core/include"
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/imgproc/include"
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/highgui/include"
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/videoio/include"
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/imgcodecs/include"
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/dnn/include"
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/objdetect/include"
    "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/video/include"
    "${CMAKE_BINARY_DIR}"
)

# ==============================================================================
# 2. Static Utilities Library
# ==============================================================================
add_library(Utils STATIC utils.cpp)

target_include_directories(Utils PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(Utils PUBLIC ${OPENCV_INTERNAL_INCLUDES})

add_dependencies(Utils opencv_core opencv_videoio)
target_link_libraries(Utils PUBLIC data_types opencv_core opencv_videoio)

# ==============================================================================
# 3. Static Face Recognition Engine Library
# ==============================================================================
add_library(FaceEngine STATIC face_engine.cpp)

# Allow face_engine.cpp to easily find face_engine.h inside src/include/
target_include_directories(FaceEngine PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_include_directories(FaceEngine PUBLIC ${OPENCV_INTERNAL_INCLUDES})

# Enforce build system synchronization blocks
add_dependencies(FaceEngine
    opencv_core
    opencv_imgproc
    opencv_highgui
    opencv_dnn
    opencv_objdetect
)

# Link targeted OpenCV binaries to compile your static engine library
target_link_libraries(FaceEngine PUBLIC
    data_types
    opencv_core
    opencv_imgproc
    opencv_highgui
    opencv_dnn
    opencv_objdetect
)
