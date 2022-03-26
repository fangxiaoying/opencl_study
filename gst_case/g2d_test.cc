#include <string.h>
#include <math.h>
#include <time.h>
#include <malloc.h>

#include <g2d.h>
#include <opencv2/opencv.hpp>

extern "C" {
#include "gst/imx_2d_device.h"
}
// GST_DEBUG_CATEGORY_EXTERN (imx2ddevice_debug);
// #define GST_CAT_DEFAULT imx2ddevice_debug


int main(int argc, char** argv)
{
    int ret;
    struct g2d_buf *src_buffer, *dst_buffer;
    PhyMemBlock src_mem = {0}, dst_mem = {0};

    Imx2DDevice* g2d_device = imx_2d_device_create(IMX_2D_DEVICE_G2D);

    cv::Mat raw_image = cv::imread("1080p.jpg");
    if (raw_image.empty()) {
        printf("read image is empty! \n");
        return -1;
    }
    cv::cvtColor(raw_image, raw_image, cv::COLOR_BGR2BGRA);
    int width  = 1920;
    int height = 1080
    
    ;
    int channels = raw_image.channels();
    size_t length = width * height * channels;

    src_buffer = g2d_alloc(length, 1);
    dst_buffer = g2d_alloc(length, 1);
    memcpy(src_buffer->buf_vaddr, raw_image.data, length);
    memset(dst_buffer->buf_vaddr, 0, length);
    
    src_mem.paddr = (guint8*)src_buffer->buf_paddr;
    // src_mem.vaddr = (guint8*)src_buffer->buf_vaddr;
    src_mem.size = src_buffer->buf_size;
    dst_mem.paddr = (guint8*)dst_buffer->buf_paddr;
    dst_mem.vaddr = (guint8*)dst_buffer->buf_vaddr;
    dst_mem.size = dst_buffer->buf_size;

    ret = g2d_device->open(g2d_device);
    ret = g2d_device->frame_copy(g2d_device, &src_mem, &dst_mem);
    ret = g2d_device->close(g2d_device);

    cv::Mat dst_image, show_image;
    dst_image.create (height, width, CV_8UC4);
    dst_image.data = (uchar *) ((unsigned long)dst_mem.vaddr);
    cv::cvtColor(dst_image, show_image, cv::COLOR_BGRA2BGR);
    cv::imwrite("g2d_image.jpg", show_image);

    g2d_free(src_buffer);
    g2d_free(dst_buffer);

}