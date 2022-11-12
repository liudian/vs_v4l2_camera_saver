cmake_minimum_required(VERSION 3.23)
project(vs_v4l2_camera_saver)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OpenCV REQUIRED opencv4)
pkg_check_modules(x265 REQUIRED x265)
find_package(exiv2 REQUIRED)
find_package(CUDAToolkit REQUIRED)
find_package(Boost REQUIRED COMPONENTS system program_options date_time filesystem)

add_executable(cameraSaver main.cpp
        v4l2camera.h v4l2camera.cpp
        jpegencodercuda.h jpegencodercuda.cpp
        )

add_executable(videoSplit main_video_split.cpp)

target_include_directories(cameraSaver
        PUBLIC ${OpenCV_INCLUDE_DIRS}
        )

target_include_directories(videoSplit
        PUBLIC ${OpenCV_INCLUDE_DIRS}
        )

target_link_libraries(cameraSaver
        PUBLIC ${OpenCV_LIBRARIES}
        CUDA::cudart CUDA::nvjpeg
        Boost::program_options Boost::date_time
        exiv2lib
        )

target_link_libraries(videoSplit
        ${OpenCV_LIBRARIES}
        Boost::program_options Boost::filesystem
        exiv2lib
        )