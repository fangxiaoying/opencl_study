#include <string.h>
#include <math.h>
#include <time.h>
#include <malloc.h>

#include <g2d.h>
#include <opencv2/opencv.hpp>
#include "gst/imx_2d_device.h"


int main(int argc, char** argv)
{
    // struct imx_gpu IMX_GPU;
    // memset(&IMX_GPU, 0, sizeof(imx_gpu));
    // cl_init(&IMX_GPU);

    cv::Mat raw_image = cv::imread("1080p.jpg");
    if (raw_image.empty()) {
        printf("read image is empty! \n");
        return -1;
    }
    cv::cvtColor(raw_image, raw_image, cv::COLOR_BGR2BGRA);

    // int ret = 0;
    // void *g2d_handle_ = NULL;
    // // setup g2d
    // ret = g2d_open(&g2d_handle_);
    // if (ret != 0 || g2d_handle_ == NULL)
    // {
    //   GST_ERROR ("g2d_open failed");
    //   return ERROR;
    // }

    // // setup src g2d surface
    // struct g2d_surface src;
    // ret = setup_g2d_surface(
    //     vinfo->finfo->format,
    //     vinfo->width,
    //     vinfo->height,
    //     src_frame->mem->paddr,
    //     src_frame->rotate,
    //     &src);
    // if (ret != 0) {
    //     GST_ERROR("setup_surface failed");
    // return ret;
    // }







}