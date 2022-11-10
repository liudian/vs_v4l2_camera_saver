//
// Created by fengchong on 10/14/22.
//
#include <iostream>
#include "v4l2camera.h"
#include "jpegencodercuda.h"
#include "opencv2/opencv.hpp"
#include <stdio.h>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
namespace bpo = boost::program_options;

int main(int argc, char** argv) {
    std::string video_device;                                                                                           //设备名称 /dev/video0
    int jpeg_quality;                                                                                                   //图片质量 默认100
    int video_duration;                                                                                                 //视频拆分时长 默认5分钟300秒
    video_duration = 300;
    FILE *out_file = nullptr;
    std::string out_file_path;


    //分析参数
    bpo::options_description opts("all options");
    bpo::variables_map vm;
    opts.add_options()
            ("help","produce help message")
            ("camera,c",bpo::value<std::string>(&video_device)->default_value("/dev/video0"),"save camera name ex:/dev/video0")
            ("jpeg_quality",bpo::value<int>(&jpeg_quality)->default_value(100),"jpeg quality")
            ("out_path", bpo::value<std::string>(&out_file_path)->default_value("./"), "save avi file path")
            ("video_duration",bpo::value<int>(&video_duration)->default_value(300),"video duration");
    try{
        bpo::store(parse_command_line(argc,argv,opts),vm);
    }
    catch(...){
        std::cout << "some args is not defined" << std::endl;
        return 0;
    }
    bpo::notify(vm);
    if(vm.count("help")){
        std::cout << opts << std::endl;
        return 1;
    }


    std::cout << "start save camera " << video_device << " data" << std::endl;
    vs::V4L2Camera camera(video_device);
    vs::JPEGEncoderCUDA jpeg_encoder(camera.frame_width(),camera.frame_height(),jpeg_quality);
    cv::Mat rgb(cv::Size(camera.frame_width(),camera.frame_height()),CV_8UC3);

    boost::posix_time::ptime before,now;
    std::vector<std::string> split_string;
    boost::split(split_string,video_device,boost::is_any_of("/"),boost::token_compress_on);
    boost::posix_time::time_duration td(0,0,video_duration+1,0);
    before = boost::posix_time::microsec_clock::local_time()-td;
    std::string out_file_path_name = out_file_path + split_string.at(split_string.size()-1)+"_"+to_iso_string(before);
    std::cout << "save path:" << out_file_path_name << std::endl;
//    out_file = fopen(out_file_path_name.c_str(), "wa+");
    for(;;){

        camera.read_frame_callback([&](void *p, std::uint32_t len, const v4l2_buffer &buf) {
            now = boost::posix_time::microsec_clock::local_time();
//            std::cout << "total seconds:" << (now-before).total_seconds() << " video_duration:" << video_duration << std::endl;

            if((now-before).total_seconds() >= video_duration){
                before = now;
                if(!access(out_file_path_name.c_str(),0))
                    fclose(out_file);
                out_file_path_name = out_file_path+split_string.at(split_string.size()-1)+"_"+to_iso_string(now);
                out_file = fopen(out_file_path_name.c_str(),"wa+");
            }

//            fwrite((char*)camera.buffers()[buf.index].start,1,buf.length,file);                                       //原始数据写入
            cv::Mat yuyv(cv::Size(camera.frame_width(),camera.frame_height()),CV_8UC2,p);
            cv::cvtColor(yuyv,rgb,cv::COLOR_YUV2RGB_YUYV);
            std::vector<uint8_t> vector = jpeg_encoder.encode(rgb.data);
            fwrite(vector.data(),sizeof(decltype(vector)::value_type),vector.size(),out_file);
        });
    }

    fclose(out_file);
    return 0;
}
