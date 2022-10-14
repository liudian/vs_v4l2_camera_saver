//
// Created by liudian on 6/21/22.
//

#ifndef VS_V4L2_CAMERA_JPEGENCODERCUDA_H
#define VS_V4L2_CAMERA_JPEGENCODERCUDA_H
#include <cstdint>
#include <cuda_runtime.h>
#include <nvjpeg.h>
#include <stdexcept>
#include <vector>
namespace vs {

    class JPEGEncoderCUDA {
    private:
        std::uint32_t width_, height_;
        nvjpegHandle_t nv_handle_;
        nvjpegEncoderState_t nv_enc_state_;
        nvjpegEncoderParams_t nv_enc_params_;
        void *device_buffer_ = nullptr;
        size_t buffer_length_;
        nvjpegImage_t nv_image_{};
    public:
        JPEGEncoderCUDA(std::uint32_t width, std::uint32_t height, std::uint8_t jpeg_quality);
        std::vector<std::uint8_t> encode(void *buffer);
        ~JPEGEncoderCUDA();

    };

} // vs

#endif //VS_V4L2_CAMERA_JPEGENCODERCUDA_H
