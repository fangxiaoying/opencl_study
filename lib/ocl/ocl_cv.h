#ifndef _OCL_CV_H_
#define _OCL_CV_H_

#include "imx_device.h"
#include <stdbool.h>
#include <g2d.h>

#ifdef __cplusplus
extern "C" {
#endif

void copy_viv(struct imx_gpu* GPU, void* src, void* dst, u_int size, bool is_phy_addr);
void padding_viv(struct imx_gpu* GPU, void* src, void* dst, int left, int top, bool is_phy_addr);
void resize_viv(struct imx_gpu* GPU, void* src_ptr, void* dst_ptr, int src_w, int src_h,
                                                        int dst_w, int dst_h, bool is_phy_addr);

void padding_init(ocl_device* GPU, ocl_function* fill, struct g2d_buf *src, 
        struct g2d_buf **dst, int src_w, int src_h, int *fill_right, int *fill_bottom);
void padding_run(ocl_function* fill, int src_w, int src_h, bool flush);
void padding_release(ocl_function* fill, struct g2d_buf *dst);

#ifdef __cplusplus
}
#endif

#endif