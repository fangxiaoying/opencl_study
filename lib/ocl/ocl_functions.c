#include "ocl_functions.h"
#include <CL/cl_ext_viv.h>

#define phy_mem_flag CL_MEM_USE_UNCACHED_HOST_MEMORY_VIV | CL_MEM_USE_HOST_PHYSICAL_ADDR_VIV


void copy_ocl(struct imx_gpu* GPU, uint8_t* src, int width, int height, int channels, uint8_t* dst)
{
    cl_event prof_event;
    cl_kernel kernel;
    cl_mem src_mem, dst_mem;
    size_t image_size =  width * height * channels;
    size_t global_size[2];

    uint64_t sigStart, sigEnd;
    float msVal;

    kernel = clCreateKernel(GPU->program, "copy_data", &status);
    CHECK_ERROR(status);

    src_mem = clCreateBuffer(GPU->context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, 
                                            sizeof(u_char) * image_size, src, &status);

    dst_mem = clCreateBuffer(GPU->context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, 
                                            sizeof(u_char) * image_size, dst, &status);
    CHECK_ERROR(status);


    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &src_mem);
    status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &dst_mem);
    status = clSetKernelArg(kernel, 2, sizeof(cl_int), &width);

    CHECK_ERROR(status);
    

    global_size[0] = width; 
    global_size[1] = height;

    sigStart = get_perf_count();
    status = clEnqueueNDRangeKernel(GPU->queue, kernel, 2, NULL, 
                                                        global_size, 0, 0, NULL, &prof_event);
    clWaitForEvents(1, &prof_event);
    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000000;
    printf("time 1: %.2fms \n", msVal);



    sigStart = get_perf_count();
    status = clEnqueueReadBuffer(GPU->queue, dst_mem, CL_TRUE, 0, 
                                           sizeof(u_char) * image_size, dst, 0, NULL, NULL);
                                                                                                         
    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000000;
    printf("time 2: %.2fms \n", msVal);                                                                                

    clReleaseMemObject(src_mem);
    clReleaseMemObject(dst_mem);
    clReleaseKernel(kernel); 
}

void zerocpy_ocl(struct imx_gpu* GPU, uint8_t* src, int width, int height, int channels, uint8_t* dst)
{
    cl_event prof_event;
    cl_kernel kernel;
    cl_mem src_mem, dst_mem;
    size_t image_size =  width * height * channels;
    size_t global_size[2];

    uint64_t sigStart, sigEnd;
    float msVal;

    kernel = clCreateKernel(GPU->program, "copy_data", &status);
    CHECK_ERROR(status);

    src_mem = clCreateBuffer(GPU->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR , 
                                            sizeof(u_char) * image_size , src, &status);

    dst_mem = clCreateBuffer(GPU->context,  CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, 
                                              sizeof(u_char) * image_size, dst, &status);
    CHECK_ERROR(status);

    // src = (uint8_t*)clEnqueueMapBuffer(GPU->queue, src_mem, CL_TRUE, CL_MAP_WRITE, 0, sizeof(cl_uchar) * image_size, 0, NULL, NULL, &status);
    // CHECK_ERROR(status);

    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &src_mem);
    status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &dst_mem);
    status = clSetKernelArg(kernel, 2, sizeof(cl_int), &width);
    CHECK_ERROR(status);

    global_size[0] = width; 
    global_size[1] = height;

    sigStart = get_perf_count();
    status = clEnqueueNDRangeKernel(GPU->queue, kernel, 2, NULL, 
                                                        global_size, 0, 0, NULL, &prof_event);
    clWaitForEvents(1, &prof_event);
    // clFlush(GPU->queue);
    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000000;
    printf("time 1: %.2fms \n", msVal);

    sigStart = get_perf_count();
    // dst = (uint8_t*)clEnqueueMapBuffer(GPU->queue, dst_mem, CL_TRUE, CL_MAP_READ, 0, sizeof(cl_uchar) * image_size, 0, NULL, NULL, &status);
    status = clEnqueueReadBuffer(GPU->queue, dst_mem, CL_TRUE, 0, sizeof(u_char) * image_size, dst, 0, NULL, NULL);
    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000;
    printf("time 2: %.2fus \n", msVal);

    // clEnqueueUnmapMemObject(GPU->queue, src_mem, src, 0, NULL, NULL);
    // clEnqueueUnmapMemObject(GPU->queue, dst_mem, dst, 0, NULL, NULL);

    clReleaseMemObject(src_mem);
    clReleaseMemObject(dst_mem);
    clReleaseKernel(kernel);    
}

void zerocpy_ocl_g2d(struct imx_gpu* GPU, struct g2d_buf* src, int width, int height, int channels, struct g2d_buf* dst)
{
    cl_event prof_event;
    cl_kernel kernel;
    cl_mem src_mem, dst_mem;
    size_t image_size =  width * height;
    size_t global_size[2];

    uint64_t sigStart, sigEnd;
    float msVal;

    void* in_data =  (void*)(unsigned long)(unsigned int)src->buf_paddr;
    void* out_data = (void*)(unsigned long)(unsigned int) dst->buf_paddr;


    kernel = clCreateKernel(GPU->program, "copy_data", &status);
    CHECK_ERROR(status);

    src_mem = clCreateBuffer(GPU->context, phy_mem_flag | CL_MEM_READ_ONLY, 
                                        sizeof(u_char) * image_size * channels, in_data, &status);
    CHECK_ERROR(status);

    dst_mem = clCreateBuffer(GPU->context,  phy_mem_flag | CL_MEM_WRITE_ONLY, 
                                            sizeof(u_char) * image_size * channels, out_data, &status);
    CHECK_ERROR(status);

    status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &src_mem);
    status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &dst_mem);
    status = clSetKernelArg(kernel, 2, sizeof(cl_int), &width);
    CHECK_ERROR(status);

    global_size[0] = width; 
    global_size[1] = height;

    sigStart = get_perf_count();

    status = clEnqueueNDRangeKernel(GPU->queue, kernel, 2, NULL, 
                                                        global_size, 0, 0, NULL, &prof_event);
    clFlush(GPU->queue);
    // clFinish(GPU->queue);

    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000000;
    printf("time 1: %.2fms \n", msVal);


    clReleaseMemObject(src_mem);
    clReleaseMemObject(dst_mem);
    clReleaseKernel(kernel);    
}

void zerocpy_ocl_last(struct imx_gpu* GPU, struct g2d_buf* src, int width, int height, int channels, struct g2d_buf* dst)
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
    void* out_data = (void*)(unsigned long)(unsigned int) dst->buf_paddr;


    kernel = clCreateKernel(GPU->program, "zero_copy", &status);
    CHECK_ERROR(status);

    src_mem = clCreateBuffer(GPU->context, phy_mem_flag | CL_MEM_READ_ONLY, 
                                        sizeof(u_char) * image_size * channels, in_data, &status);
    CHECK_ERROR(status);

    dst_mem = clCreateBuffer(GPU->context,  phy_mem_flag | CL_MEM_WRITE_ONLY, 
                                            sizeof(u_char) * image_size * channels, out_data, &status);
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
    clFlush(GPU->queue);
    // clWaitForEvents(1, &prof_event);

    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000000;
    printf("time 1: %.2fms \n", msVal);


    clReleaseMemObject(src_mem);
    clReleaseMemObject(dst_mem);
    clReleaseKernel(kernel);    
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


void vector_ocl(struct imx_gpu* GPU, struct g2d_buf* src, int width, int height, struct g2d_buf* dst)
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

    kernel = clCreateKernel(GPU->program, "vector", &status);
    CHECK_ERROR(status);

    src_mem = clCreateBuffer(GPU->context, phy_mem_flag | CL_MEM_READ_ONLY, 
                                        sizeof(u_char) * image_size * 4, in_data, &status);
    CHECK_ERROR(status);

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
    clWaitForEvents(1, &prof_event);
    // clFlush(GPU->queue);
    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000000;
    printf("time 1: %.2fms \n", msVal);

    clReleaseMemObject(src_mem);
    clReleaseMemObject(dst_mem);
    clReleaseKernel(kernel);    
}