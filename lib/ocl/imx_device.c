#include <time.h>

#include "imx_device.h"

int status = CL_SUCCESS;

void create_context(struct imx_gpu* GPU)
{
    status = clGetPlatformIDs(2, &GPU->firstPlatformId, &GPU->numPlatforms);
    CHECK_ERROR(status);

    cl_context_properties contextProperties[] =
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)GPU->firstPlatformId,
        0
    };

    GPU->context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU,NULL, NULL, &status);
    CHECK_ERROR(status);
}

char* kernel_source(const char* filename, size_t *programSize)
{
    FILE *pFile = NULL;
    char *programSource = NULL;
    pFile = fopen((char *)filename, "rb");
    int ret;

    if (pFile != NULL && programSize)
    {
        // obtain file size:
        fseek(pFile, 0, SEEK_END);
        *programSize = ftell(pFile);
        rewind(pFile);

        int size = *programSize + 1;
        programSource = (char*)malloc(sizeof(char)*(size));
        if (programSource == NULL)
        {
            fclose(pFile);
            free(programSource);
            return NULL;
        }

        ret=fread(programSource, sizeof(char), *programSize, pFile);
        if(ret == 0){
            printf("read file %s failed! \n",filename);
            return NULL;
        }
        programSource[*programSize] = '\0';
        fclose(pFile);
    }
    return programSource;
}

void build_program(struct imx_gpu* GPU,  const char* filename)
{
    /* Read program file and place content into buffer */
    size_t programLen = 0;
    const char* programSrc = kernel_source(filename, &programLen);
    if(programLen == 0) {
        printf("kernel_file %s is empty! \n", filename);
        exit(EXIT_FAILURE);
    }

	/* Create program from file */
	GPU->program = clCreateProgramWithSource(GPU->context, 1,
		(const char**)&programSrc, &programLen, &status);   
    CHECK_ERROR(status);
	free((void*)programSrc);
    programSrc = NULL;

    /* compile opencl program */
    size_t  log_size;
    char* build_options = "-cl-fast-relaxed-math"; 
	status = clBuildProgram(GPU->program, GPU->numDevices, GPU->devices, build_options, NULL, NULL);
	if (status < 0) {
        for (cl_uint i = 0; i < GPU->numDevices; ++ i) {
		    /* Find size of log and print to std output */
		    clGetProgramBuildInfo(GPU->program, GPU->devices[i], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		    char * program_log = (char*)malloc(log_size + 1);
		    program_log[log_size] = '\0';
		    clGetProgramBuildInfo(GPU->program, GPU->devices[i], CL_PROGRAM_BUILD_LOG,log_size + 1, program_log, NULL);
		    printf("%s\n", program_log);
		    free(program_log);
        }
        return;
	}

}

void cl_init(struct imx_gpu* GPU)
{
    create_context(GPU);

    status = clGetDeviceIDs(GPU->firstPlatformId, CL_DEVICE_TYPE_GPU, 1, NULL, &GPU->numDevices);
    GPU->devices = (cl_device_id *)malloc(GPU->numDevices * sizeof(cl_device_id));
    status = clGetDeviceIDs(GPU->firstPlatformId, CL_DEVICE_TYPE_ALL, GPU->numDevices, GPU->devices, NULL);
    CHECK_ERROR(status);

    GPU->queue = clCreateCommandQueue(GPU->context, GPU->devices[0], 0, &status);
    CHECK_ERROR(status);

#ifdef _debug_
    size_t devicesListSize;
    char device_name[48];

    status = clGetContextInfo(GPU->context,CL_CONTEXT_DEVICES, 0,NULL, &devicesListSize);  //devicesListSize = 16
    status = clGetContextInfo(GPU->context,CL_CONTEXT_DEVICES, 16, GPU->devices, NULL);
    for (cl_uint i = 0; i < GPU->numDevices; ++i) {
        clGetDeviceInfo(GPU->devices[i], CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
        printf("Device %d  name: %s \n", i, device_name);
    }  
#endif  

    build_program(GPU, "_kernel.cl");
}


void cl_release(struct imx_gpu* GPU)
{
    clReleaseCommandQueue(GPU->queue);
    clReleaseProgram(GPU->program);
    clReleaseContext(GPU->context);  
}

uint64_t get_perf_count()
{
#if defined(__linux__) || defined(__ANDROID__) || defined(__QNX__) || defined(__CYGWIN__)
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)((uint64_t)ts.tv_nsec + (uint64_t)ts.tv_sec * BILLION);
#elif defined(_WIN32) || defined(UNDER_CE)
    LARGE_INTEGER ln;

    QueryPerformanceCounter(&ln);

    return (uint64_t)ln.QuadPart;
#endif
}
