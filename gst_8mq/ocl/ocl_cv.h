#ifndef _OCL_CV_H_
#define _OCL_CV_H_

#include "imx_device.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void BGRA2RGB_init(ocl_function* csc);
void BGRA2RGB_run(ocl_function* csc, void * host_src, void* host_dst, int width, int height, bool is_phy_addr);
void BGRA2RGB_release(ocl_function* csc);

#ifdef __cplusplus
}
#endif

#endif