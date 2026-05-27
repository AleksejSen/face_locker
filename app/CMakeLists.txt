add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        opencv_core
        opencv_imgproc
        opencv_highgui
        opencv_videoio
        opencv_imgcodecs
        opencv_dnn
        opencv_objdetect
        opencv_video
        CLI11::CLI11
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"

        # Public and Module headers from source
        "${CMAKE_SOURCE_DIR}/extern_libs/opencv/include"
        "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/core/include"
        "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/imgproc/include"
        "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/highgui/include"
        "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/videoio/include"
        "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/imgcodecs/include"
        "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/dnn/include"
        "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/objdetect/include"
        "${CMAKE_SOURCE_DIR}/extern_libs/opencv/modules/video/include"

        # FIX: Points to 'build/', allowing the compiler to find 'opencv2/opencv_modules.hpp'
        "${CMAKE_BINARY_DIR}"

        # CLI11 headers
        "${CMAKE_SOURCE_DIR}/extern_libs/cli11/include"
)
