#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <gst/gst.h>
#include <gst/allocators/gstdmabuf.h>


#include <opencv2/opencv.hpp>
#include <cairo/cairo.h>
#include <fstream>

#include <g2d.h>
#include "ocl/ocl_cv.h"
#include "gst/gstimx.h"


#define ENABLE_G2D
// #define ENABLE_FPS

#define WIDTH  1920
#define HEIGHT 1080


#ifdef ENABLE_G2D
#define BUFFER_NUMS 2
#define VIDEOCONVERT "imxvideoconvert_g2d"
#else
#define VIDEOCONVERT "videoconvert"
#endif

/*
appsink use fd, not need map, improve the system performance


enviroment:
imx8mp, L5.15.5_1.0.0
show video size 1080p

performance:
fps 30
cup loading:
open_camera_0 : 13 %
weston: 7 %
*/

struct appdata {
    GMutex g_mutex;
    ocl_function bgra2rgb;
    struct g2d_buf * g2d_buffers[BUFFER_NUMS];
};

int count = 0;

static void
debug_dma_buffer(int fd, struct appdata* app, int width, int height, int channels)
{
    void* src_paddr = 0;
    void* dst_paddr = 0;
    u_int size = width * height;

    count++;

    if(50 == count) {          
        printf("appsink cap img shape %d x %d \n", width, height);

        uint64_t sigStart, sigEnd;
        float msVal;

        sigStart = get_perf_count();

        src_paddr = (void*)phy_addr_from_fd(fd);
        dst_paddr = (void*)(unsigned long)(unsigned int)app->g2d_buffers[1]->buf_paddr;

        BGRA2RGB_run(&app->bgra2rgb, src_paddr, dst_paddr, width, height);




        cv::Mat dst_image, show_image; 
        dst_image.create (height, width, CV_8UC3);
        dst_image.data = (uchar *) ((unsigned long) app->g2d_buffers[1]->buf_vaddr);

        cv::copyMakeBorder(dst_image, dst_image, 0, 300 - height, 0, 300- width, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));


        sigEnd = get_perf_count();
        msVal = (sigEnd - sigStart)/1000000;
        printf("time 1: %.2fms \n", msVal);

        cv::cvtColor(dst_image, show_image, cv::COLOR_RGB2BGR);
        cv::imwrite("rgb.jpg", show_image);


        count = 0;
    }
}




static GstFlowReturn
new_sample(GstElement* sink, gpointer* data)
{
    int dam_buf_fd;

    struct appdata* app = (struct appdata*) data;
    
    GstSample* sample;
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if (!sample) return GST_FLOW_ERROR;

    GstCaps*      caps       = gst_sample_get_caps(sample);
    GstStructure* gst_struct = gst_caps_get_structure(caps, 0);

    int width, height;
    gst_structure_get_int(gst_struct, "width", &width);
    gst_structure_get_int(gst_struct, "height", &height);


    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        g_printerr("Unable to get buffer from sample\n");
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    dam_buf_fd = gst_dmabuf_memory_get_fd (gst_buffer_peek_memory (buffer,0));
    // printf("fd = %d \n",dam_buf_fd);

    g_mutex_lock(&app->g_mutex);
    
    // debug_image(mapinfo.data, mapinfo.size, width, height);

    debug_dma_buffer(dam_buf_fd, app, width, height, 4);

    // gst_buffer_unmap(buffer, &mapinfo);
    gst_sample_unref(sample);


    g_mutex_unlock(&app->g_mutex);

    return GST_FLOW_OK;
}


int main(int argc, char** argv)
{
    const gchar*   camera            = "/dev/video3";
    int            video_width       = WIDTH;
    int            video_height      = HEIGHT;
    int            input_width       = 300;
    int            input_height      = 300;
    int            size              = 0;
    float          scale             = 0.0;
    int            scaled_width       = 0;
    int            scaled_height      = 0;

    ocl_device     imxgpu            = {0};
    ocl_function   rgba2bgr          = {0};
    struct appdata app               = {0};


    /* keep original sacling image */
    float scale_w = (float)video_width / input_width;
    float scale_h = (float)video_height / input_height;
    float ratio   = (float)video_width / video_height;

    /* width align with 16 */
    scale = scale_w > scale_h ? scale_w : scale_h;


    scaled_width = int(video_width / scale) >> 4 << 4;
    scaled_height = int(scaled_width / ratio);
    size =  scaled_width * scaled_height * 4;
    // printf("%f , %d . %d \n", scale, scaled_width, scaled_height);
    for(int i = 0; i < BUFFER_NUMS; i++)
	{
        switch (i)
        {
        case 0:
            /* appsink src pad format RGBX */
            size = scaled_width * scaled_height * 4;
            app.g2d_buffers[i] = g2d_alloc(size, 1);
            break;
        case 1:
            /* RGBX convert to RGB24 */
            size = scaled_width * scaled_height * 3;
            app.g2d_buffers[i] = g2d_alloc(size, 1);
            break;
        default:
            break;
        }
	}

    cl_init(&imxgpu);
    app.bgra2rgb.GPU = &imxgpu;
    BGRA2RGB_init(&app.bgra2rgb);

    /*step1  init gst */
    gst_init(&argc, &argv);
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    if (!loop) {
        g_printerr("failed to create main loop\n");
        return EXIT_FAILURE;
    }

    /*step2  create gst element*/
#if defined(WIN32) || defined(__APPLE__)
    GstElement* source = gst_element_factory_make("autovideosrc", "source");
#else
    GstElement* source = gst_element_factory_make("v4l2src", "source");
#endif

    GstElement* filter     = gst_element_factory_make("capsfilter", "filter");
    GstElement* tee        = gst_element_factory_make("tee", "tee");
    GstElement* queue      = gst_element_factory_make("queue", "queue");
    GstElement* convert    = gst_element_factory_make(VIDEOCONVERT, "convert");
    GstElement* appsink    = gst_element_factory_make("appsink", "appsink");
    GstElement* adaptor1   = gst_element_factory_make(VIDEOCONVERT, "adaptor1");
    GstElement* overlay    = gst_element_factory_make("cairooverlay", "overlay");
    GstElement* adaptor2   = gst_element_factory_make(VIDEOCONVERT, "adaptor2");
#ifdef ENABLE_FPS
    GstElement* fpsdisplay = gst_element_factory_make("fpsdisplaysink", "fpsdisplay");
#else
    GstElement* display = gst_element_factory_make("autovideosink", "display");
#endif

    if (!source || !filter || !tee || !queue ||
        !convert || !appsink || !overlay || !adaptor1 ||
        !adaptor2  ) {
        g_printerr("Failed to create elements for pipeline\n");
        return EXIT_FAILURE;
    }

    /* step3 set element's property*/

    // 1. source
    g_object_set(source, "device", camera, NULL);

    // 2. filter
    const char* cam_format = "YUY2";
    GstCaps* filter_caps = gst_caps_new_simple("video/x-raw",
                                "format", G_TYPE_STRING, cam_format,
                                 "width", G_TYPE_INT, video_width,
                                "height", G_TYPE_INT, video_height,
                                NULL);
    //filter has no property name drop,  but appsink have this.
    g_object_set(filter,
                 "caps", filter_caps,
                 NULL);
    gst_caps_unref(filter_caps);

    //3. app sink filter, used for nn inference
    const char* nn_data_format = "BGRx";
    GstCaps* appsink_caps = gst_caps_new_simple("video/x-raw",
                                "format", G_TYPE_STRING, nn_data_format,
                                "width",  G_TYPE_INT, scaled_width,
                                "height", G_TYPE_INT, scaled_height,
                                NULL);
    //filter has no property name drop,  but appsink have this.
    g_object_set(appsink,
                 "caps", appsink_caps,
                 "drop", FALSE,
                 "emit-signals", TRUE,
                 NULL);
    gst_caps_unref(appsink_caps);


    /* create callback funtion */
    g_signal_connect(appsink, "new-sample", G_CALLBACK(new_sample), &app);

    g_object_set(queue, "leaky", 2, "max-size-buffers", 1, NULL);

    /* create and construct pipe line*/
    GstElement* cap_pipeline = gst_pipeline_new ("test-pipeline");


    // thread 1:  split cam input data flow.
    gst_bin_add_many(GST_BIN(cap_pipeline),
                         source,
                         filter,
                         tee,
                         queue,
                         convert,
                         appsink,
                         adaptor1,
                         overlay,
                         adaptor2,
#ifdef ENABLE_FPS
                         fpsdisplay,
#else
                         display,
#endif
                         NULL);

    if (!gst_element_link_many(source, filter, tee, NULL)) {
        g_printerr("Failed to link input cap_pipeline\n");
        return EXIT_FAILURE;
    }

    // thread 2:  ink to appsink,  inference pipe line
    if (!gst_element_link_many(tee,
                               queue,
                               convert,
                               appsink,
                               NULL)) {
        g_printerr("Failed to link appsink cap_pipeline\n");
        return EXIT_FAILURE;
    }

    // thread 3:  display pipeline
    if (!gst_element_link_many(tee,
                               adaptor1,
#ifdef ENABLE_FPS
                               fpsdisplay,
#else
                               display,
#endif
                                NULL)) {
        g_printerr("Failed to link display cap_pipeline\n");
        return EXIT_FAILURE;
    }

    gst_element_set_state(cap_pipeline, GST_STATE_PLAYING);
    g_main_loop_run(loop);

    // FIXME: This cleanup code currently never happens, SIGINT will
    // terminate the program!
    gst_element_set_state(cap_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(cap_pipeline));
    g_main_loop_unref(loop);

    BGRA2RGB_release(&app.bgra2rgb);
    cl_release(app.bgra2rgb.GPU);
    return EXIT_SUCCESS;
}