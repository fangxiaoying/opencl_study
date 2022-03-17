#include "ocl_cv.h"
#include <CL/cl_ext_viv.h>

#define benchmark

#define phy_mem_flag CL_MEM_USE_UNCACHED_HOST_MEMORY_VIV | CL_MEM_USE_HOST_PHYSICAL_ADDR_VIV


void copy_viv(struct imx_gpu* GPU, void* src, void* dst, u_int size, bool is_phy_addr)
{
    cl_event prof_event;
    cl_kernel kernel;
    cl_mem src_mem, dst_mem;
    
    size_t local_size[2];
    size_t global_size[2];

    if( is_phy_addr != true) {
        printf("input addr not physical addrees! \n");
        return;
    }

    uint64_t sigStart, sigEnd;
    float msVal;

    kernel = clCreateKernel(GPU->program, "copy", &status);
    CHECK_ERROR(status);

    src_mem = clCreateBuffer(GPU->context, phy_mem_flag | CL_MEM_READ_ONLY, 
                                             sizeof(u_char) * size, src, &status);

    dst_mem = clCreateBuffer(GPU->context,  phy_mem_flag | CL_MEM_WRITE_ONLY, 
                                             sizeof(u_char) * size, dst, &status);
    CHECK_ERROR(status);

    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &src_mem);
    status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &dst_mem);
    CHECK_ERROR(status);

    /*8MP work-gropu size == 8 */
    local_size[0] = 16;
    local_size[1] = 1;
    global_size[0] = (size + local_size[0] - 1) / local_size[0];  // rounded up
    global_size[1] = 1;

#ifdef benchmark
    sigStart = get_perf_count();
#endif

    status = clEnqueueNDRangeKernel(GPU->queue, kernel, 2, NULL, 
                                                        global_size, local_size, 0, NULL, &prof_event);
    // clFlush(GPU->queue);
    clWaitForEvents(1, &prof_event);

#ifdef benchmark
    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000000;
    printf("time 1: %.2fms \n", msVal);
#endif

    clReleaseMemObject(src_mem);
    clReleaseMemObject(dst_mem);
    clReleaseKernel(kernel);   
}

/* The function only support symmetrical padding.
 * left: letf = right, how many pixels need filled
 * top : top = bottom, how many pixels need filled
 */

void padding_viv(struct imx_gpu* GPU, void* src, void* dst, int left, int top, bool is_phy_addr)
{
    /* will implement if necessary */
}


/* default image format */
void resize_viv(struct imx_gpu* GPU, void* src_ptr, void* dst_ptr, int src_w, int src_h,
                                    int dst_w, int dst_h, bool is_phy_addr)
{
    cl_event prof_event;
    cl_kernel kernel;
    cl_mem src_image, dst_image;
    cl_image_format image_format;
    cl_image_desc src_desc, dst_desc;
    size_t global_size[2];

#if 1
    image_format.image_channel_order     = CL_RGBA;
    image_format.image_channel_data_type = CL_UNSIGNED_INT8;
#else
    /* SAMPLER NOT SUPPORT RGB24 */
    image_format.image_channel_order     = CL_RGB;
    image_format.image_channel_data_type = CL_UNORM_SHORT_565;
#endif

    memset(&src_desc, 0, sizeof(src_desc));
    src_desc.image_type   = CL_MEM_OBJECT_IMAGE2D_ARRAY;
    src_desc.image_width  = src_w;
    src_desc.image_height = src_h;

    memset(&dst_desc, 0, sizeof(dst_desc));
    dst_desc.image_type   = CL_MEM_OBJECT_IMAGE2D;
    dst_desc.image_width  = dst_w;
    dst_desc.image_height = dst_h;

    kernel = clCreateKernel(GPU->program, "resize", &status);
    CHECK_ERROR(status);


    src_image = clCreateImage(GPU->context, phy_mem_flag | CL_MEM_READ_ONLY,
                                        &image_format, &src_desc, src_ptr, &status);
    CHECK_ERROR(status);

    src_image = clCreateImage(GPU->context, phy_mem_flag | CL_MEM_WRITE_ONLY,
                                        &image_format, &dst_desc, dst_ptr, &status);

    CHECK_ERROR(status);

    cl_float widthNormalizationFactor  = 1.0f / dst_w;
    cl_float heightNormalizationFactor = 1.0f / dst_h;

    status  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &src_image);
    status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &dst_image);
    status |= clSetKernelArg(kernel, 2, sizeof(cl_float), &widthNormalizationFactor);
    status |= clSetKernelArg(kernel, 3, sizeof(cl_float), &heightNormalizationFactor);
    CHECK_ERROR(status);

    global_size[0] = dst_w; 
    global_size[1] = dst_h;

    status = clEnqueueNDRangeKernel(GPU->queue, kernel, 2, NULL, global_size,
                                                                         NULL, 0, NULL, NULL);

    clReleaseMemObject(src_image);
    clReleaseMemObject(dst_image);
    clReleaseKernel(kernel);   
}
