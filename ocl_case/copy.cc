#include "ocl/ocl_cv.h"
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
    cv::cvtColor(raw_image, raw_image, cv::COLOR_BGR2BGRA);
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
    copy_viv(&IMX_GPU, (void*)(unsigned long)(unsigned int)in_buffer->buf_paddr, 
                    (void*)(unsigned long)(unsigned int)out_buffer->buf_paddr, length, true);
    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000000;
    printf("Average %.2fms \n", msVal);
    

    cv::Mat dst_image; 
    dst_image.create (height, width, CV_8UC4);
    dst_image.data = (uchar *) ((unsigned long) out_buffer->buf_vaddr);
    cv::cvtColor(dst_image, show_image, cv::COLOR_BGRA2BGR);
    cv::imwrite("copy.jpg", show_image);
}