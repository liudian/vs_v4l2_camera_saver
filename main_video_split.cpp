//
// Created by fengchong 11/12/22.
//
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <exiv2/exiv2.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace bpo = boost::program_options;

class JpegSplitMark
{
public:
    uint16_t FFD8;
    uint16_t FFE0;
};

void renameAllImage(boost::filesystem::path b_out_path){
    boost::filesystem::directory_iterator endIter;
    for(boost::filesystem::directory_iterator iter(b_out_path);iter!=endIter;iter++){
        if(!boost::filesystem::is_directory(*iter)){
            Exiv2::Image::UniquePtr  image = Exiv2::ImageFactory::open(iter->path().string());
            assert(image.get()!=0);
            image->readMetadata();
            Exiv2::ExifData &exifData = image->exifData();
            Exiv2::Exifdatum& tag = exifData["Exif.Photo.DateTimeOriginal"];
            std::string timestamp = tag.toString();
            std::string bname = basename(iter->path());

            if(boost::algorithm::contains(bname,timestamp))
                continue;

            std::string rname = basename(iter->path())+"_"+ timestamp + iter->path().extension().string();
            std::string rpath = iter->path().parent_path().string()+"/"+rname;
            boost::filesystem::rename(iter->path().string(),rpath);

            std::cout << "rname path:" << rpath << std::endl;

//            std::cout << "path:" << iter->path().string() << " timestamp:" << timestamp
//                        << " file name:" << iter->path().filename() << " parent_path:" << iter->path().parent_path()
//                        << " extension:" << iter->path().extension() << " absolute:" << absolute(iter->path())
//                        << " basename:" << basename(iter->path())<< std::endl;

        }
    }
}

int splitVideo(std::string video_path_name,std::string out_path){
    JpegSplitMark jsm;
    std::ifstream inFile(video_path_name,std::ios::in|std::ios::binary);
    if(!inFile){
        std::cerr << "read video file error:" << video_path_name << std::endl;
        return 0;
    }

    //创建输出路径
    boost::system::error_code error;
    if(!boost::filesystem::is_regular_file(out_path,error))
        boost::filesystem::create_directory(out_path,error);

    int i=1;
    std::ofstream jpegFile;
    while(!inFile.eof()){
        if(inFile.read(reinterpret_cast<char*>(&jsm),sizeof(JpegSplitMark))) {
            if (jsm.FFD8 == 55551 && jsm.FFE0 == 57599){
                if(jpegFile.is_open()) jpegFile.close();
                jpegFile.open(out_path+"/"+"img"+std::to_string(i)+".jpeg",std::ios::out|std::ios::app);
                i++;
                std::cout << "read jpeg split mark:" << i << std::endl;
            }
            inFile.seekg(-4, std::ios::cur);
            uint8_t b = 0;
            inFile.read((char *)&b,sizeof(b));
            jpegFile << b;
        }
    }
    inFile.close();
    renameAllImage(boost::filesystem::path(out_path));
    return 1;
}

int main(int argc,char **argv){
    std::string video_path_name,out_path;

    bpo::options_description opts("all options");
    bpo::variables_map vm;
    opts.add_options()
            ("help","produce help message")
            ("video",bpo::value<std::string>(&video_path_name),"video file path")
            ("out_path",bpo::value<std::string>(&out_path)->default_value("./output/"),"save image file path");
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

    if(splitVideo(video_path_name,out_path)==0)
        return 0;

    return 1;
}














