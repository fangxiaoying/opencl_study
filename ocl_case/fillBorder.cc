#include "ocl/ocl_cv.h"
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
    ocl_device IMX_GPU;
    ocl_function fill;
    memset(&IMX_GPU, 0, sizeof(imx_gpu));
    cl_init(&IMX_GPU);

    cv::Mat bgra_image, show_image;
    cv::Mat raw_image = cv::imread("1080p.jpg");
    cv::cvtColor(raw_image,bgra_image, cv::COLOR_BGR2BGRA);

    int width  = bgra_image.cols;
    int height = bgra_image.rows;



    int pad_right  = 0;
    int pad_bottom = 1920 - 1080;

    size_t in_length = width * height * 4;
    

    /* G2D memory init */
    struct g2d_buf *in_buffer, *out_buffer;
    in_buffer = g2d_alloc(in_length, 1);
    memcpy(in_buffer->buf_vaddr, bgra_image.data, in_length);
    // memset(out_buffer->buf_vaddr, 0, out_length);


    padding_init(&IMX_GPU, &fill, in_buffer, &out_buffer, width, height,
                                                  &pad_right, &pad_bottom);

    sigStart = get_perf_count();
    padding_run(&fill, width, height, false);
    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000000;
    printf("Average %.2fms \n", msVal);
    

    cv::Mat dst_image; 
    dst_image.create (height + pad_bottom, width + pad_right, CV_8UC4);
    dst_image.data = (uchar *) ((unsigned long) out_buffer->buf_vaddr);
    cv::cvtColor(dst_image, show_image, cv::COLOR_BGRA2BGR);
    cv::imwrite("rgb.jpg", show_image);

    padding_release(&fill, out_buffer);
    cl_release(&IMX_GPU);
    g2d_free(in_buffer);
}