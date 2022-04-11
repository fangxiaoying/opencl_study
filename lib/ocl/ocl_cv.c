#include "ocl_cv.h"
#include <CL/cl_ext_viv.h>
#include <string.h>

#define benchmark

#define phy_mem_flag CL_MEM_USE_UNCACHED_HOST_MEMORY_VIV | CL_MEM_USE_HOST_PHYSICAL_ADDR_VIV


void copy_viv(struct imx_gpu* GPU, void* src, void* dst, u_int size, bool is_phy_addr)
{
    cl_event prof_event;
    cl_kernel kernel;
    cl_mem src_mem, dst_mem;
    
    // size_t local_size[2];
    // size_t global_size[2];
    size_t local_size;
    size_t global_size;



    if( is_phy_addr != true) {
        printf("input addr not physical addrees! \n");
        return;
    }

    uint64_t sigStart, sigEnd;
    float msVal;

#ifdef benchmark
    sigStart = get_perf_count();
#endif

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
    // local_size[0] = 16;
    // local_size[1] = 1;
    // global_size[0] = (size + local_size[0] - 1) / local_size[0];  // rounded up
    // global_size[1] = 1;

    local_size = 16;
    global_size = (size + local_size - 1) / local_size;  // rounded up


    // status = clEnqueueNDRangeKernel(GPU->queue, kernel, 2, NULL, 
    //                                                     global_size, local_size, 0, NULL, &prof_event);

    status = clEnqueueNDRangeKernel(GPU->queue, kernel, 1, NULL, 
                                                        &global_size, &local_size, 0, NULL, &prof_event);

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
    src_desc.image_type   = CL_MEM_OBJECT_IMAGE2D;
    src_desc.image_width  = src_w;
    src_desc.image_height = src_h;
    src_desc.image_depth  = 1;
    src_desc.image_array_size = 1;
    src_desc.image_row_pitch = src_w * 4; //RGBA
    src_desc.image_slice_pitch = src_w * src_h * 4;
    src_desc.num_mip_levels = 0;
    src_desc.num_samples = 0;
    src_desc.buffer = NULL;


    memset(&dst_desc, 0, sizeof(dst_desc));
    dst_desc.image_type   = CL_MEM_OBJECT_IMAGE2D;
    dst_desc.image_width  = dst_w;
    dst_desc.image_height = dst_h;

    kernel = clCreateKernel(GPU->program, "resize", &status);
    CHECK_ERROR(status);


    src_image = clCreateImage(GPU->context, CL_MEM_USE_HOST_PHYSICAL_ADDR_VIV| CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
                                        &image_format, &src_desc, src_ptr, &status);
    CHECK_ERROR(status);

    dst_image = clCreateImage(GPU->context, phy_mem_flag | CL_MEM_WRITE_ONLY,
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

/*  padding image simple implement, only support fill black pixels on image right
 *  and bottom and only support RGBA/BGRA/BGRX/RGBX format
 */

void padding_init(ocl_device* GPU, ocl_function* fill, struct g2d_buf *src, 
        struct g2d_buf **dst, int src_w, int src_h, int *fill_right, int *fill_bottom)
{
    fill->GPU = GPU;
    fill->mems = (cl_mem*)malloc(sizeof(cl_mem) * 2);

    // output size align 8x8 block
    size_t src_size = src_w * src_h * 4;
    *fill_right = ((src_w + *fill_right + 7) >> 3 << 3) - src_w;
    *fill_bottom = ((src_h + *fill_bottom + 7) >> 3 << 3) - src_h;
    size_t dst_size =  (src_w + *fill_right) * (src_h + *fill_bottom) * 4;

    

    *dst = g2d_alloc(dst_size, 1);
    void* src_ptr =  (void*)(unsigned long)(unsigned int)src->buf_paddr;
    void* dst_ptr = (void*)(unsigned long)(unsigned int)(*dst)->buf_paddr;
    memset((u_char *)((unsigned long)(*dst)->buf_vaddr), 0, dst_size);

    

    fill->kernel = clCreateKernel(GPU->program, "padding", &status);
    CHECK_ERROR(status);

    fill->mems[0] = clCreateBuffer(GPU->context, phy_mem_flag | CL_MEM_READ_ONLY, 
                                             sizeof(u_char) * src_size, src_ptr, &status);

    fill->mems[1] = clCreateBuffer(GPU->context,  phy_mem_flag | CL_MEM_WRITE_ONLY, 
                                             sizeof(u_char) * dst_size, dst_ptr, &status);
    CHECK_ERROR(status);



    status = clSetKernelArg(fill->kernel, 0, sizeof(cl_mem), &fill->mems[0]);
    status = clSetKernelArg(fill->kernel, 1, sizeof(cl_mem), &fill->mems[1]);
    status = clSetKernelArg(fill->kernel, 2, sizeof(cl_int), &src_w);
    status = clSetKernelArg(fill->kernel, 3, sizeof(cl_int), fill_right);
    CHECK_ERROR(status);
}

void padding_run(ocl_function* fill, int src_w, int src_h, bool flush)
{
    size_t local_size[2];
    size_t global_size[2];

    // size_t src_length = src_w * src_h * 4;

    local_size[0] = 8;
    local_size[1] = 8;
    global_size[0] = src_w;  // rounded up
    global_size[1] = src_h;
        
    status = clEnqueueNDRangeKernel(fill->GPU->queue, fill->kernel, 2, NULL, 
                                                global_size, local_size, 0, NULL, &fill->prof_event);
  
    if(true == flush) 
        clFlush(fill->GPU->queue);
    else
        clWaitForEvents(1, &fill->prof_event);
}

void padding_release(ocl_function* fill, struct g2d_buf *dst)
{
    g2d_free(dst);
    clReleaseMemObject(fill->mems[0]);
    clReleaseMemObject(fill->mems[1]);
    clReleaseKernel(fill->kernel);
    free(fill->mems);
}

void bgra2rgb_ocl(struct imx_gpu* GPU, struct g2d_buf* src, int width, int height, struct g2d_buf* dst)
{
    cl_event prof_event;
    cl_kernel kernel;
    cl_mem src_mem, dst_mem;
    size_t image_size =  width * height;
    size_t local_size[2];
    size_t global_size[2];

    uint64_t sigStart, sigEnd;
    float msVal;

    void* in_data =  (void*)(unsigned long)(unsigned int)src->buf_paddr;
    void* out_data = (void*)(unsigned long)(unsigned int)dst->buf_paddr;


    kernel = clCreateKernel(GPU->program, "BGRA2RGB", &status);
    CHECK_ERROR(status);

    src_mem = clCreateBuffer(GPU->context, phy_mem_flag | CL_MEM_READ_ONLY, 
                                        sizeof(u_char) * image_size * 4, in_data, &status);

    dst_mem = clCreateBuffer(GPU->context,  phy_mem_flag | CL_MEM_WRITE_ONLY, 
                                            sizeof(u_char) * image_size * 3, out_data, &status);
    CHECK_ERROR(status);

    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &src_mem);
    status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &dst_mem);
    status = clSetKernelArg(kernel, 2, sizeof(cl_int), &width);
    CHECK_ERROR(status);

    /*8MP work-gropu size == 8 */
    local_size[0] = 16;
    local_size[1] = 1;
    global_size[0] = (width + local_size[0] -1) / local_size[0] * local_size[0];  // rounded up
    global_size[1] = height;

    sigStart = get_perf_count();

    status = clEnqueueNDRangeKernel(GPU->queue, kernel, 2, NULL, 
                                                        global_size, local_size, 0, NULL, &prof_event);

    // clFlush(GPU->queue); 
    clWaitForEvents(1, &prof_event);

    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000000;
    printf("time 1: %.2fms \n", msVal);


    clReleaseMemObject(src_mem);
    clReleaseMemObject(dst_mem);
    clReleaseKernel(kernel);    
}

void BGRA2RGB_init(ocl_function* csc)
{
    csc->kernel = clCreateKernel(csc->GPU->program, "BGRA2RGB", &status);
    CHECK_ERROR(status);

    csc->mems = (cl_mem*)calloc(2, sizeof(cl_mem));
}

void BGRA2RGB_run(ocl_function* csc, void * host_src, void* host_dst, int width, int height)
{
    size_t image_size = width * height;
    size_t local_size[2];
    size_t global_size[2];

    csc->mems[0] = clCreateBuffer(csc->GPU->context, phy_mem_flag | CL_MEM_READ_ONLY, 
                                        sizeof(u_char) * image_size * 4, host_src, &status);

    csc->mems[1] = clCreateBuffer(csc->GPU->context,  phy_mem_flag | CL_MEM_WRITE_ONLY, 
                                            sizeof(u_char) * image_size * 3, host_dst, &status);
    CHECK_ERROR(status);

    status = clSetKernelArg(csc->kernel, 0, sizeof(cl_mem), &csc->mems[0]);
    status |= clSetKernelArg(csc->kernel, 1, sizeof(cl_mem), &csc->mems[1]);
    status |= clSetKernelArg(csc->kernel, 2, sizeof(cl_int), &width);
    CHECK_ERROR(status);

    /*8MP work-gropu size == 8 */
    local_size[0] = 16;
    local_size[1] = 1;
    global_size[0] = (width + local_size[0] -1) / local_size[0] * local_size[0];  // rounded up
    global_size[1] = height;

    status = clEnqueueNDRangeKernel(csc->GPU->queue, csc->kernel, 2, NULL, 
                                                 global_size, local_size, 0, NULL, &csc->prof_event);

    // clFlush(GPU->queue); 
    clWaitForEvents(1, &csc->prof_event);

}

void BGRA2RGB_release(ocl_function* csc)
{
    clReleaseMemObject(csc->mems[0]);
    clReleaseMemObject(csc->mems[1]);
    clReleaseKernel(csc->kernel);  
    free(csc->mems);
}