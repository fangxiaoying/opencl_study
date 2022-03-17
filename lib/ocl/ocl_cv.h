#ifndef _OCL_CV_H_
#define _OCL_CV_H_

#include "imx_device.h"
#include <stdbool.h> 

#ifdef __cplusplus
extern "C" {
#endif

void copy_viv(struct imx_gpu* GPU, void* src, void* dst, u_int size, bool is_phy_addr);
void padding_viv(struct imx_gpu* GPU, void* src, void* dst, int left, int top, bool is_phy_addr);
void resize_viv(struct imx_gpu* GPU, void* src_ptr, void* dst_ptr, int src_w, int src_h,
                                                        int dst_w, int dst_h, bool is_phy_addr);


#ifdef __cplusplus
}
#endif

#endif