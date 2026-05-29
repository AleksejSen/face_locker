set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Build GoogleTest from your local submodule directory
add_subdirectory("${CMAKE_SOURCE_DIR}/extern_libs/googletest" "${CMAKE_BINARY_DIR}/googletest_build")

# Declare the test binary executable
add_executable(run_engine_tests test_face_engine.cpp)

# Point to internal headers so it finds face_engine.h and data_types.h
target_include_directories(run_engine_tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/src/include"
)

# Link your existing library targets along with the local submodule targets
target_link_libraries(run_engine_tests PRIVATE
    FaceEngine
    data_types
    gtest
    gtest_main
)

# Register the executable directly into CTest
add_test(NAME FaceEngineTests COMMAND run_engine_tests)

set_tests_properties(FaceEngineTests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
