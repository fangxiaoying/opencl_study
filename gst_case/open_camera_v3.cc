#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <gst/gst.h>
#include <gst/allocators/gstdmabuf.h>

#include <cairo/cairo.h>

#include <opencv2/opencv.hpp>
#include <fstream>

#include <g2d.h>
#include "ocl/ocl_cv.h"
#include "gst/gstimx.h"

#define ENABLE_G2D
// #define ENABLE_FPS

#define WIDTH  1920
#define HEIGHT 1080


#ifdef ENABLE_G2D
#define BUFFER_NUMS   1
#define VIDEOCONVERT "imxvideoconvert_g2d"
#else
#define VIDEOCONVERT "videoconvert"
#endif

// struct appdata {
//     GMutex g_mutex;
//     ocl_function bgra2rgb;
//     int pad_right;
//     int pad_bottom;
//     struct g2d_buf * g2d_buffers[BUFFER_NUMS];
// };
struct appdata {
    GMutex g_mutex;
    struct imx_gpu IMX_GPU;
    struct g2d_buf * g2d_buffers[BUFFER_NUMS];
};

/*
gstreamer base line.  
pipeline:
gst-launch-1.0 -v v4l2src device=/dev/video3 ! "video/x-raw,format=BGRx,width=1920,height=1080" ! \
tee name=t ! queue ! imxvideoconvert_g2d ! appsink caps="video/x-raw,format=BGRx" drop=true max-buffers=2 t. ! \
waylandsink

add gpu optimization OCL padding and G2D resize

enviroment:
imx8mp, L5.10.72
show video size 1080p

performance:
fps 30
cup loading: 
open_camera_0(one-core) :  8
weston: 6

if not need get image from gstreamer pipeline, the
CPU loading is only 4% on one-core.
*/

int count = 0;

static void
dma_buffer_proc(int fd, struct appdata* app, int width, int height, int channels)
{
    unsigned long src_paddr = 0;
    unsigned long dst_paddr = 0;
    u_int size = width * height;

    
    count++;

    if(50 == count) {            
        printf("appsink cap img shape %d x %d \n", width, height);

        src_paddr = phy_addr_from_fd(fd);
        dst_paddr = app->g2d_buffers[0]->buf_paddr;


        copy_viv(&app->IMX_GPU, (void*)src_paddr, (void*)dst_paddr, size * channels, true);

// //         cv::Mat src_image(height, width, CV_8UC4, raw_data);
// //         cv::Mat dst;
// // #ifdef OCL
// //         return ;
// // #else
// //         cv::cvtColor(src_image, dst, cv::COLOR_BGRA2BGR);
// // #endif
// //         // cv::imwrite("test.jpg", dst);

        cv::Mat dst_image, show_image; 
        dst_image.create (height, width, CV_8UC4);
        dst_image.data = (uchar *) ((unsigned long) app->g2d_buffers[0]->buf_vaddr);
        cv::cvtColor(dst_image, show_image, cv::COLOR_BGRA2BGR);
        cv::imwrite("test.jpg", show_image);

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
  

    g_mutex_lock(&app->g_mutex);

    dma_buffer_proc(dam_buf_fd, app, width, height, 4);

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

    // ocl_device     IMX_GPU;
    // struct appdata app               = {0};

    // /*for pad*/
    // float scale_w = (float)video_width / input_width;
    // float scale_h = (float)video_height / input_height;
    // scale = scale_w > scale_h ? scale_w : scale_h;

    // scaled_width = int(video_width / scale);
    // scaled_height = int(video_height / scale);
    // size =  scaled_width * scaled_height * 4;
    // printf("%f , %d . %d \n", scale, scaled_width, scaled_height);

    // app.pad_right = input_width - scaled_width;
    // app.pad_bottom = input_height - scaled_height;
    // app.g2d_buffers[0] = g2d_alloc(size, 1);    //RGB

    // cl_init(&IMX_GPU);
    // app.bgra2rgb.GPU = &IMX_GPU;
    



    struct appdata app;
    memset(&app, 0, sizeof(appdata));
    cl_init(&app.IMX_GPU);
    for(int i = 0; i < BUFFER_NUMS; i++)
	{
        switch (i)
        {
        case 0:
            /* appsink src pad format RGBX */
            size = input_width * input_height * 4;
            app.g2d_buffers[i] = g2d_alloc(size, 1);
            break;
        case 1:
            /* RGBX convert to RGB24 */
            size = input_width * input_width * 3;
            app.g2d_buffers[i] = g2d_alloc(size, 1);
            break;
        default:
            break;
        }
		
	}


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
                                 "width", G_TYPE_INT, input_width,
                                "height", G_TYPE_INT, input_height,
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


    cl_release(&app.IMX_GPU);

    return EXIT_SUCCESS;
}