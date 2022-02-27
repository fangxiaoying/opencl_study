#include "ocl/ocl_functions.h"
#include <string.h>
#include <math.h>

#include <opencv2/opencv.hpp>


int main(int argc, char** argv)
{
    struct imx_gpu IMX_GPU;
    memset(&IMX_GPU, 0, sizeof(imx_gpu));
    cl_init(&IMX_GPU);

    cv::Mat raw_image = cv::imread("1080p.jpg");
    if (raw_image.empty()) {
        printf("read image is empty! \n");
        return -1;
    } 

    int width  = raw_image.cols;
    int height = raw_image.rows;
    int channels = raw_image.channels();
    size_t length = width * height * channels;

    uint64_t sigStart, sigEnd;
    float msVal;

    uint8_t* dst_data = (uint8_t*)calloc(width * height * channels, sizeof(uint8_t));

    sigStart = get_perf_count();
    copy_ocl(&IMX_GPU,raw_image.data, width, height, channels, dst_data);
    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000;
    printf("Total : %.2fms \n", msVal);


    cv::Mat dst_image(height, width, CV_8UC3, dst_data);
    // cv::cvtColor(dst_image, show_image, cv::COLOR_BGRA2BGR);
    cv::imwrite("cl_image.jpg", dst_image);

    cl_release(&IMX_GPU);
    free(dst_data);
    dst_data = NULL;

}