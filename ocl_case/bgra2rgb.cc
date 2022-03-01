#include "ocl/ocl_functions.h"
#include <string.h>
#include <math.h>
#include <time.h>
#include <malloc.h>

#include <g2d.h>
#include <opencv2/opencv.hpp>


int main(int argc, char** argv)
{
    uint64_t sigStart, sigEnd;
    float msVal;

    /*GPU init*/
    struct imx_gpu IMX_GPU;
    memset(&IMX_GPU, 0, sizeof(imx_gpu));
    cl_init(&IMX_GPU);

    cv::Mat bgra_image, show_image;
    cv::Mat raw_image = cv::imread("1080p.jpg");
    cv::cvtColor(raw_image,bgra_image, cv::COLOR_BGR2BGRA);

    int width  = bgra_image.cols;
    int height = bgra_image.rows;

    size_t in_length = width * height * 4;
    size_t out_length = width * height * 3;

    /* G2D memory init */
    struct g2d_buf *in_buffer, *out_buffer;
    in_buffer = g2d_alloc(in_length, 1);
    out_buffer = g2d_alloc(out_length, 1);
    memcpy(in_buffer->buf_vaddr, bgra_image.data, in_length);
    memset(out_buffer->buf_vaddr, 0, out_length);


    sigStart = get_perf_count();
    bgra2rgb_ocl(&IMX_GPU, in_buffer, width, height, out_buffer);
    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000000;
    printf("Average %.2fms \n", msVal);
    

    cv::Mat dst_image; 
    dst_image.create (height, width, CV_8UC3);
    dst_image.data = (uchar *) ((unsigned long) out_buffer->buf_vaddr);
    cv::cvtColor(dst_image, show_image, cv::COLOR_RGB2BGR);
    cv::imwrite("rgb.jpg", show_image);

    cl_release(&IMX_GPU);
    g2d_free(in_buffer);
    g2d_free(out_buffer);
}