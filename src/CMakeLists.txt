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
