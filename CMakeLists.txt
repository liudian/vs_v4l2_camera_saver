cmake_minimum_required(VERSION 3.23)
project(vs_v4l2_camera_saver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OpenCV REQUIRED opencv4)
pkg_check_modules(x265 REQUIRED x265)
find_package(CUDAToolkit REQUIRED)
find_package(Boost REQUIRED COMPONENTS system program_options date_time)

add_executable(cameraSaver main.cpp
        v4l2camera.h v4l2camera.cpp
        jpegencodercuda.h jpegencodercuda.cpp
        )

target_include_directories(cameraSaver
        PUBLIC ${OpenCV_INCLUDE_DIRS}
        )

target_link_libraries(cameraSaver
        PUBLIC ${OpenCV_LIBRARIES}
        CUDA::cudart CUDA::nvjpeg
        Boost::program_options Boost::date_time
        )