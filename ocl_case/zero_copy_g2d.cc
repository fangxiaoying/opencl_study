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
    int width  = raw_image.cols;
    int height = raw_image.rows;
    int channels = raw_image.channels();
    size_t length = width * height * channels;

    uint64_t sigStart, sigEnd;
    float msVal;

    in_buffer = g2d_alloc(length, 1);
    out_buffer = g2d_alloc(length, 1);
    memcpy(in_buffer->buf_vaddr, raw_image.data, length);
    memset(out_buffer->buf_vaddr, 0, length);

    sigStart = get_perf_count();
    zerocpy_ocl_g2d(&IMX_GPU, in_buffer, width, 
                            height, channels, out_buffer);
    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000000;
    printf("Average %.2fms \n", msVal);
    

    cv::Mat dst_image; 
    dst_image.create (height, width, CV_8UC3);
    dst_image.data = (uchar *) ((unsigned long) out_buffer->buf_vaddr);
    cv::imwrite("cl_image_g2d.jpg", dst_image);

    cl_release(&IMX_GPU);
    g2d_free(in_buffer);
    g2d_free(out_buffer);
}