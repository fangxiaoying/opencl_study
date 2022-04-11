#include "ocl_cv.h"
#include <CL/cl_ext_viv.h>
#include <string.h>

#define benchmark

#define phy_mem_flag CL_MEM_USE_UNCACHED_HOST_MEMORY_VIV | CL_MEM_USE_HOST_PHYSICAL_ADDR_VIV


void BGRA2RGB_init(ocl_function* csc)
{
    csc->kernel = clCreateKernel(csc->GPU->program, "BGRA2RGB", &status);
    CHECK_ERROR(status);
    csc->mems = (cl_mem*)calloc(2, sizeof(cl_mem));
}

void BGRA2RGB_run(ocl_function* csc, void * host_src, void* host_dst, int width, int height, bool is_phy_addr)
{
    size_t image_size = width * height;
    size_t local_size[2];
    size_t global_size[2];

    if(true == is_phy_addr) {
        csc->mems[0] = clCreateBuffer(csc->GPU->context, phy_mem_flag | CL_MEM_READ_ONLY, 
                                            sizeof(u_char) * image_size * 4, host_src, &status);

        csc->mems[1] = clCreateBuffer(csc->GPU->context,  phy_mem_flag | CL_MEM_WRITE_ONLY, 
                                            sizeof(u_char) * image_size * 3, host_dst, &status);
    } else {
        csc->mems[0] = clCreateBuffer(csc->GPU->context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, 
                                            sizeof(u_char) * image_size * 4 , host_src, &status);

        csc->mems[1] = clCreateBuffer(csc->GPU->context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, 
                                            sizeof(u_char) * image_size * 3, host_dst, &status);
    }
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

    if(false == is_phy_addr) {
        host_dst = (uint8_t*)clEnqueueMapBuffer(csc->GPU->queue, csc->mems[1], CL_TRUE, CL_MAP_READ, 
                                        0, sizeof(cl_uchar) * image_size * 3, 0, NULL, NULL, &status);
    }

    status = clEnqueueNDRangeKernel(csc->GPU->queue, csc->kernel, 2, NULL, 
                                                global_size, local_size, 0, NULL, &csc->prof_event);
    
    clEnqueueUnmapMemObject(csc->GPU->queue, csc->mems[1], host_dst, 0, NULL, NULL);

    // clFlush(csc->GPU->queue); 
    clWaitForEvents(1, &csc->prof_event);

}

void BGRA2RGB_release(ocl_function* csc)
{
    clReleaseMemObject(csc->mems[0]);
    clReleaseMemObject(csc->mems[1]);
    clReleaseKernel(csc->kernel);  
    free(csc->mems);
}