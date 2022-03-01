#ifndef _OCL_FUNCTIONS_H_
#define _OCL_FUNCTIONS_H_

#include "imx_device.h"
#include <g2d.h>

#ifdef __cplusplus
extern "C" {
#endif

void copy_ocl(struct imx_gpu* GPU, uint8_t* src, int src_w, int src_h, int channels, uint8_t* dst);
void zerocpy_ocl(struct imx_gpu* GPU, uint8_t* src, int width, int height, int channels, uint8_t* dst);
void zerocpy_ocl_g2d(struct imx_gpu* GPU, struct g2d_buf* src, int width, int height, int channels, struct g2d_buf* dst);
void zerocpy_ocl_last(struct imx_gpu* GPU, struct g2d_buf* src, int width, int height, int channels, struct g2d_buf* dst);
void bgra2rgb_ocl(struct imx_gpu* GPU, struct g2d_buf* src, int width, int height, struct g2d_buf* dst);

// void bgra2rgb_ocl_zerocpy(struct imx_gpu* GPU, uint8_t* src, int width, int height, uint8_t* dst);
// void copy_phy_ocl(struct imx_gpu* GPU, struct g2d_buf* src, int width, int height, int channels, struct g2d_buf* dst);

void vector_ocl(struct imx_gpu* GPU, struct g2d_buf* src, int width, int height, struct g2d_buf* dst);

#ifdef __cplusplus
}
#endif

#endif