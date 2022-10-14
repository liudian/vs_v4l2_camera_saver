//
// Created by fengchong on 10/14/22.
//

#include "v4l2camera.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <cerrno>
#include <stdexcept>
#include <system_error>
#include <iostream>


namespace vs {
    V4L2Camera::Fd::Fd(const std::string &path) : fd_(open(path.c_str(), O_RDWR)) {
        if(-1 == fd_) {
            throw std::system_error(errno, std::generic_category(), std::string("open ") + path);
        }
    }

    V4L2Camera::Fd::~Fd() {
        if(-1 == close(fd_)) {
            perror("close");
            exit(EXIT_FAILURE);
        }
    }

    int V4L2Camera::Fd::get() const {
        return fd_;
    }

    V4L2Camera::V4L2Camera(const std::string &dev_name) : fd_(dev_name), dev_name_(dev_name) {

        init_device();
        init_mmap();
        start_capture();
    }

    void V4L2Camera::init_device() {
        v4l2_capability cap{};
        if (-1 == ioctl(fd_.get(), VIDIOC_QUERYCAP, &cap)) {
            throw std::system_error(errno, std::generic_category(),
                                    std::string("VIDIOC_QUERYCAP ") + dev_name_);
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
            throw std::runtime_error(dev_name_ + " is no video capture device");
        }
        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            throw std::runtime_error(dev_name_ + " does not support streaming i/o");
        }
        v4l2_format format{};
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd_.get(), VIDIOC_G_FMT, &format) == -1) {
            throw std::system_error(errno, std::generic_category(),
                                    std::string("VIDIOC_G_FMT ") + dev_name_);
        }
        if (V4L2_PIX_FMT_YUYV != format.fmt.pix.pixelformat) {
            throw std::runtime_error(dev_name_ + " pixel format is not V4L2_PIX_FMT_YUYV");
        }
        frame_width_ = format.fmt.pix.width;
        frame_height_ = format.fmt.pix.height;
        std::cout << "frame size :" << frame_width_ << " " << frame_height_ << std::endl;

        if (ioctl(fd_.get(), VIDIOC_S_FMT, &format) == -1) {
            throw std::system_error(errno, std::generic_category(),
                                    std::string("VIDIOC_S_FMT ") + dev_name_);
        }

    }

    void V4L2Camera::init_mmap() {
        v4l2_requestbuffers req{};

        req.count = buffers_.size();
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (-1 == ioctl(fd_.get(), VIDIOC_REQBUFS, &req)) {
            throw std::system_error(errno, std::generic_category(),
                                    std::string("VIDIOC_REQBUFS ") + dev_name_);
        }

        if (req.count < buffers_.size()) {
            throw std::runtime_error(dev_name_ + " insufficient buffer memory");
        }


        for (int n_buffers = 0; n_buffers < req.count; ++n_buffers) {
            v4l2_buffer buf{};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = n_buffers;

            if (-1 == ioctl(fd_.get(), VIDIOC_QUERYBUF, &buf)) {
                throw std::system_error(errno, std::generic_category(),
                                        std::string("VIDIOC_QUERYBUF ") + dev_name_);
            }

            buffers_[n_buffers].length = buf.length;
            buffers_[n_buffers].start =
                    mmap(NULL /* start anywhere */,
                         buf.length,
                         PROT_READ | PROT_WRITE /* required */,
                         MAP_SHARED /* recommended */,
                         fd_.get(), buf.m.offset);

            if (MAP_FAILED == buffers_[n_buffers].start) {

                throw std::system_error(errno, std::generic_category(),
                                        std::string("mmap ") + dev_name_);
            }
        }

    }

    void V4L2Camera::start_capture() {
        for (int i = 0; i < buffers_.size(); ++i) {
            v4l2_buffer buf{};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            if (-1 == ioctl(fd_.get(), VIDIOC_QBUF, &buf)) {
                throw std::system_error(errno, std::generic_category(),
                                        std::string("VIDIOC_QBUF ") + dev_name_);
            }
        }
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == ioctl(fd_.get(), VIDIOC_STREAMON, &type)) {
            throw std::system_error(errno, std::generic_category(),
                                    std::string("VIDIOC_STREAMON ") + dev_name_);
        }

    }

    V4L2Camera::~V4L2Camera() {
        v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == ioctl(fd_.get(), VIDIOC_STREAMOFF, &type)) {
            auto errmsg = std::string("VIDIOC_STREAMOFF ") + dev_name_;
            perror(errmsg.c_str());
            exit(EXIT_FAILURE);
        }
        for (const auto & buffer : buffers_)
            if (-1 == munmap(buffer.start, buffer.length)) {
                auto errmsg = std::string("munmap ") + dev_name_;
                perror(errmsg.c_str());
                exit(EXIT_FAILURE);
            }

    }

    int V4L2Camera::YUV422ToYUV420(unsigned char *yuv422, unsigned char *yuv420, int width, int height) {
        int y_size = width * height;
        int i,j,k=0;

        //Y
        for(i=0;i<y_size;i++){
            yuv420[i] = yuv422[i*2];
        }
        //U
        for(int i=0;i<height;i++){
            if((i%2)!=0) continue;
            for(j=0;j<(width/2);j++){
                if((4*j+1)>(2*width)) break;
                yuv420[y_size + k*2*width/4+j] = yuv422[i*2*width+4*j+1];
            }
            k++;
        }

        //V
        k=0;
        for(i=0;i<height;i++){
            if((i%2)==0) continue;
            for(j=0;j<(width/2);j++){
                if((4*j+3)>(2*width)) break;
                yuv420[y_size + y_size/4+k*2*width/4+j] = yuv422[i*2*width+4*j+3];
            }
            k++;
        }
        return 0;
    }

    void V4L2Camera::read_frame_callback(const std::function<void(void *, std::uint32_t, const v4l2_buffer &)>& callback) {
        v4l2_buffer buf{};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (-1 == ioctl(fd_.get(), VIDIOC_DQBUF, &buf)) {
            throw std::system_error(errno, std::generic_category(),
                                    std::string("VIDIOC_DQBUF ") + dev_name_);
        }

        callback(buffers_[buf.index].start, buffers_[buf.index].length, buf);

        if (-1 == ioctl(fd_.get(), VIDIOC_QBUF, &buf)) {

            throw std::system_error(errno, std::generic_category(),
                                    std::string("VIDIOC_QBUF ") + dev_name_);
        }

    }
} // vs