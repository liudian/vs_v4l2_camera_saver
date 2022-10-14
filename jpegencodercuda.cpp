//
// Created by liudian on 6/21/22.
//

#include "jpegencodercuda.h"
#include <string>

namespace vs {
    JPEGEncoderCUDA::JPEGEncoderCUDA(std::uint32_t width, std::uint32_t height, std::uint8_t jpeg_quality) : width_(
            width), height_(height) {
        nvjpegStatus_t ns;
        if ((ns = nvjpegCreateSimple(&nv_handle_)) != NVJPEG_STATUS_SUCCESS) {
            throw std::runtime_error("nvjpegCreateSimple error: " + std::to_string(ns));
        }
        if ((ns = nvjpegEncoderStateCreate(nv_handle_, &nv_enc_state_, nullptr)) != NVJPEG_STATUS_SUCCESS) {
            nvjpegDestroy(nv_handle_);
            throw std::runtime_error("nvjpegEncoderStateCreate error: " + std::to_string(ns));
        }
        if ((ns = nvjpegEncoderParamsCreate(nv_handle_, &nv_enc_params_, nullptr)) != NVJPEG_STATUS_SUCCESS) {
            nvjpegEncoderStateDestroy(nv_enc_state_);
            nvjpegDestroy(nv_handle_);
            throw std::runtime_error("nvjpegEncoderParamsCreate error: " + std::to_string(ns));
        }
        if ((ns = nvjpegEncoderParamsSetSamplingFactors(nv_enc_params_, NVJPEG_CSS_420, nullptr)) !=
            NVJPEG_STATUS_SUCCESS) {
            nvjpegEncoderParamsDestroy(nv_enc_params_);
            nvjpegEncoderStateDestroy(nv_enc_state_);
            nvjpegDestroy(nv_handle_);
            throw std::runtime_error("nvjpegEncoderParamsSetSamplingFactors error: " + std::to_string(ns));
        }
        if ((ns = nvjpegEncoderParamsSetQuality(nv_enc_params_, jpeg_quality, nullptr)) !=
            NVJPEG_STATUS_SUCCESS) {
            nvjpegEncoderParamsDestroy(nv_enc_params_);
            nvjpegEncoderStateDestroy(nv_enc_state_);
            nvjpegDestroy(nv_handle_);
            throw std::runtime_error("nvjpegEncoderParamsSetQuality error: " + std::to_string(ns));
        }
        cudaError_t ce;
        buffer_length_ = width * 3 * height;
        if ((ce = cudaMalloc(&device_buffer_, buffer_length_)) != cudaSuccess) {
            nvjpegEncoderParamsDestroy(nv_enc_params_);
            nvjpegEncoderStateDestroy(nv_enc_state_);
            nvjpegDestroy(nv_handle_);
            throw std::runtime_error("cudaMalloc error: " + std::to_string(ce));
        }
        nv_image_.channel[0] = static_cast<unsigned char *>(device_buffer_);
        nv_image_.pitch[0] = 3 * width;


    }

    JPEGEncoderCUDA::~JPEGEncoderCUDA() {
        nvjpegEncoderParamsDestroy(nv_enc_params_);
        nvjpegEncoderStateDestroy(nv_enc_state_);
        nvjpegDestroy(nv_handle_);

    }

    std::vector<std::uint8_t> JPEGEncoderCUDA::encode(void *buffer) {
        std::vector<std::uint8_t> ret;
        cudaError_t ce = cudaMemcpy(device_buffer_, buffer, buffer_length_, cudaMemcpyHostToDevice);
        if (ce != cudaSuccess) {
            throw std::runtime_error("cudaMemcpy error: " + std::to_string(ce));
        }
        nvjpegStatus_t ns;
        ns = nvjpegEncodeImage(nv_handle_, nv_enc_state_, nv_enc_params_, &nv_image_,
                               NVJPEG_INPUT_RGBI, width_, height_, nullptr);
        if(ns != NVJPEG_STATUS_SUCCESS) {
            throw std::runtime_error("nvjpegEncodeImage error: " + std::to_string(ns));

        }
        size_t length = 0;
        ns = nvjpegEncodeRetrieveBitstream(nv_handle_, nv_enc_state_, nullptr, &length, nullptr);
        if(ns != NVJPEG_STATUS_SUCCESS) {
            throw std::runtime_error("nvjpegEncodeRetrieveBitstream error: " + std::to_string(ns));

        }
        ret.resize(length);
        ns = nvjpegEncodeRetrieveBitstream(nv_handle_, nv_enc_state_, ret.data(), &length, nullptr);
        ns = nvjpegEncodeRetrieveBitstream(nv_handle_, nv_enc_state_, nullptr, &length, nullptr);
        if(ns != NVJPEG_STATUS_SUCCESS) {
            throw std::runtime_error("nvjpegEncodeRetrieveBitstream error: " + std::to_string(ns));

        }
        return ret;
    }
} // vs