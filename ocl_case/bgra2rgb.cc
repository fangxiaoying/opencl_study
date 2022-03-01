#include "ocl/ocl_functions.h"
#include <string.h>
#include <math.h>
#include <time.h>
#include <malloc.h>

#include <g2d.h>
#include <opencv2/opencv.hpp>


int main(int argc, char** argv)
{
    struct imx_gpu IMX_GPU;
    memset(&IMX_GPU, 0, sizeof(imx_gpu));
    cl_init(&IMX_GPU);

    struct g2d_buf *in_buffer, *out_buffer;

    cv::Mat bgra_image, show_image;
    cv::Mat raw_image = cv::imread("1080p.jpg");
    cv::cvtColor(raw_image,bgra_image, cv::COLOR_BGR2BGRA);

    int width  = bgra_image.cols;
    int height = bgra_image.rows;

    size_t in_length = width * height * 4;
    size_t out_length = width * height * 3;

    uint64_t sigStart, sigEnd;
    float msVal;


    in_buffer = g2d_alloc(in_length, 1);
    out_buffer = g2d_alloc(out_length, 1);

    memcpy(in_buffer->buf_vaddr, bgra_image.data, in_length);

    // uint8_t* src_data = (uint8_t *)memalign(64, in_length * sizeof(uint8_t));
    // uint8_t* dst_data = (uint8_t *)memalign(64, out_length * sizeof(uint8_t));
    // memcpy(src_data, bgra_image.data, in_length);
    // bgra2rgb_tmp(&IMX_GPU, src_data, width, height, dst_data);




    sigStart = get_perf_count();
    bgra2rgb_ocl(&IMX_GPU, in_buffer, width, height, out_buffer);
    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000000;
    printf("Average %.2fms \n", msVal);
    

    cv::Mat dst_image; 
    dst_image.create (height, width, CV_8UC3);
    dst_image.data = (uchar *) ((unsigned long) out_buffer->buf_vaddr);

    // cv::Mat dst_image(height, width, CV_8UC3, dst_data);

    cv::cvtColor(dst_image, show_image, cv::COLOR_RGB2BGR);
    cv::imwrite("rgb.jpg", show_image);
}