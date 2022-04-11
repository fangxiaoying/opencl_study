#ifndef _IMX_DEVICE_H_
#define _IMX_DEVICE_H_
#define CL_TARGET_OPENCL_VERSION 120


#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>

#define _debug_

#define CHECK_ERROR(err) \
    if (err != CL_SUCCESS) { \
        printf("[%s:%d] OpenCL error %d\n", __FILE__, __LINE__, err); \
        exit(EXIT_FAILURE); \
    }

#define BILLION      1000000000


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    cl_platform_id   firstPlatformId;
    cl_uint          numPlatforms;
    cl_context       context;
    cl_device_id*    devices;
    cl_uint          numDevices;
    cl_program       program;
    cl_command_queue queue;
} ocl_device;

typedef struct {
    ocl_device *GPU;
    cl_kernel kernel;
    cl_mem *mems;
    cl_event prof_event;
} ocl_function;
 

void create_context(ocl_device* GPU);
void cl_init(ocl_device* GPU);
void build_program(ocl_device* GPU,  char const* filename);
char* kernel_source(const char* filename, size_t *programSize);
void cl_release(ocl_device* GPU);

uint64_t get_perf_count();

extern int status;

#ifdef __cplusplus
}
#endif

#endif