#include "ocl/ocl_functions.h"
#include <string.h>
#include <math.h>
#include <time.h>
#include <malloc.h>

#include <opencv2/opencv.hpp>

/*
clEnqueueMapBuffer and  64 byte memory alignment
to implement OCL zero copy
*/

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

    uint8_t* src_data = (uint8_t *)memalign(64, length * sizeof(uint8_t));
    uint8_t* dst_data = (uint8_t *)memalign(64, length * sizeof(uint8_t));
    memcpy(src_data, raw_image.data, length);

    sigStart = get_perf_count();
    zerocpy_ocl(&IMX_GPU, src_data, width, height, channels, dst_data);
    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000000;
    printf("Average %.2fms \n", msVal);
    

    cv::Mat dst_image(height, width, CV_8UC3, dst_data);
    cv::imwrite("cl_image.jpg", dst_image);

    cl_release(&IMX_GPU);
    free(src_data);
    src_data = NULL;
    free(dst_data);
    dst_data = NULL;
}