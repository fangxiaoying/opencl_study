#include "ocl/ocl_functions.h"
#include <string.h>
#include <math.h>
#include <time.h>
#include <malloc.h>

#include <g2d.h>
#include <opencv2/opencv.hpp>


int main(int argc, char** argv)
{
    struct imx_gpu IMX_GPU;
    uint64_t sigStart, sigEnd;
    float msVal;

    memset(&IMX_GPU, 0, sizeof(imx_gpu));
    cl_init(&IMX_GPU);

    struct g2d_buf *in_buffer, *out_buffer;
    int width  = 16;
    int height = 2;

    size_t in_length = width * height * 4;
    size_t out_length = width * height * 3;
    in_buffer = g2d_alloc(in_length, 1);
    out_buffer = g2d_alloc(out_length, 1);

    uint8_t* src_data = (uint8_t*)calloc(in_length, sizeof(uint8_t));
    for(int i=0; i<in_length; ++i)
        src_data[i] = i % 255;

    memcpy(in_buffer->buf_vaddr, src_data, in_length);


    sigStart = get_perf_count();
    vector_ocl(&IMX_GPU, in_buffer, width, height, out_buffer);
    sigEnd = get_perf_count();
    msVal = (sigEnd - sigStart)/1000000;
    printf("Average %.2fms \n", msVal);
    

    uint8_t* dst_data = (uchar *) ((unsigned long) out_buffer->buf_vaddr);

    for(int j=0; j< 2; ++j) {
        for(int i=0; i< 16; ++i)
            printf(" %d ",dst_data[16*j + i]);
        printf("\n");
    }

    cl_release(&IMX_GPU);
    free(src_data);

}