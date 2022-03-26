#include <string.h>
#include <math.h>
#include <time.h>
#include <malloc.h>

#include <g2d.h>
#include <opencv2/opencv.hpp>

extern "C" {
#include "gst/imx_2d_device.h"
}
GST_DEBUG_CATEGORY_EXTERN (imx2ddevice_debug);
#define GST_CAT_DEFAULT imx2ddevice_debug


int main(int argc, char** argv)
{
    int ret;
    struct g2d_buf *src_buffer, *dst_buffer;
    PhyMemBlock src_mem = {0}, dst_mem = {0};
    Imx2DFrame src = {0}, dst = {0};

    gst_init(&argc, &argv);

    Imx2DDevice* g2d_device = imx_2d_device_create(IMX_2D_DEVICE_G2D);

    cv::Mat raw_image = cv::imread("1080p.jpg");
    if (raw_image.empty()) {
        printf("read image is empty! \n");
        return -1;
    }
    cv::cvtColor(raw_image, raw_image, cv::COLOR_BGR2BGRA);
    int width  = raw_image.cols;
    int height = raw_image.rows;
    int channels = raw_image.channels();
    size_t length = width * height * channels;
    size_t resize_len = 1280 * 720 * 4;

    src_buffer = g2d_alloc(length, 1);
    dst_buffer = g2d_alloc(resize_len, 1);
    memcpy(src_buffer->buf_vaddr, raw_image.data, length);
    memset(dst_buffer->buf_vaddr, 0, resize_len);
    
    src.info.fmt = GST_VIDEO_FORMAT_RGBA;
    src.info.w = width;
    src.info.h = height;
    src.info.stride = (width + 15) & (~0xf);
    // // src.info.tile_type = IMX_2D_TILE_AMHPION;

    src.fd[0] = src.fd[1] = src.fd[2] = src.fd[3] = -1;
    src_mem.vaddr = (guint8*)src_buffer->buf_vaddr;
    src_mem.paddr = (guint8*)src_buffer->buf_paddr;
    src_mem.size = src_buffer->buf_size;
    src.mem = &src_mem;
    src.crop.x = 0;
    src.crop.y = 0;
    src.crop.w = 1920;
    src.crop.h = 1080;
    // src.rotate = IMX_2D_ROTATION_0;
    src.alpha = 0xFF;

    dst.info.fmt = GST_VIDEO_FORMAT_RGBA;
    dst.info.w = 1280;
    dst.info.h = 720;
    dst.info.stride = (1280 + 15) & (~0xf);
    
    // src.info.tile_type = 
    dst.fd[0] = dst.fd[1] = dst.fd[2] = dst.fd[3] = -1;
    dst_mem.vaddr = (guint8*)dst_buffer->buf_vaddr;
    dst_mem.paddr = (guint8*)dst_buffer->buf_paddr;
    dst_mem.size = dst_buffer->buf_size;
    dst.mem = &dst_mem;
    dst.crop.x = 0;
    dst.crop.y = 0;
    dst.crop.w = 1280;
    dst.crop.h = 720;
    // dst.rotate = IMX_2D_ROTATION_180;
    dst.alpha = 0xFF;


    ret = g2d_device->open(g2d_device);
    ret = g2d_device->config_input(g2d_device, &src.info);
    ret |= g2d_device->config_output(g2d_device, &dst.info);
    ret |= g2d_device->set_rotate(g2d_device, IMX_2D_ROTATION_180);

    ret = g2d_device->blend(g2d_device,&dst, &src);
    ret = g2d_device->close(g2d_device);

    cv::Mat dst_image, show_image;
    dst_image.create (720, 1280, CV_8UC4);
    dst_image.data = (uchar *) ((unsigned long)dst_mem.vaddr);
    cv::cvtColor(dst_image, show_image, cv::COLOR_BGRA2BGR);
    cv::imwrite("g2d_image.jpg", show_image);

    g2d_free(src_buffer);
    g2d_free(dst_buffer);
    
}