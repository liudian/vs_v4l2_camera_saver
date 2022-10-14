//
// Created by fengchong on 10/14/22.
//

#ifndef VS_V4L2_CAMERA_V4L2CAMERA_H
#define VS_V4L2_CAMERA_V4L2CAMERA_H
#include <string>
#include <array>
#include <functional>
#include <linux/videodev2.h>
#include <stdio.h>

namespace vs {

    class V4L2Camera {
    private:
        class Fd {
        private:
            int fd_;
        public:
            explicit Fd(const std::string & path);
            ~Fd();
            int get() const;
            Fd(const Fd &) = delete;
            Fd & operator=(const Fd &) = delete;
        };
        struct Buffer {
            void * start;
            size_t length;
        };
    private:
        Fd fd_;
        std::string dev_name_;
        std::array<Buffer, 3> buffers_{};
        std::uint32_t frame_width_{}, frame_height_{};
        void init_device();
        void init_mmap();
        void start_capture();
        int YUV422ToYUV420(unsigned char *yuv422,unsigned char *yuv420 ,int width,int height);

    public:
        explicit V4L2Camera(const std::string & dev_name);
        ~V4L2Camera();
        void read_frame_callback(const std::function<void(void*, std::uint32_t, const v4l2_buffer &)>& callback);
        std::array<Buffer,3> buffers() const {
            return buffers_;
        }
        std::uint32_t frame_width() const {
            return frame_width_;
        }
        std::uint32_t frame_height() const {
            return frame_height_;
        }

    };

} // vs

#endif //VS_V4L2_CAMERA_V4L2CAMERA_H
