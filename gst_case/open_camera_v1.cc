#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <gst/gst.h>
#include <gst/allocators/gstdmabuf.h>
#include <cairo/cairo.h>

#include <opencv2/opencv.hpp>
#include <fstream>

#define WIDTH  1920
#define HEIGHT 1080

#define FONT_SIZE_LABEL_SCORE 25
#define FONT_SIZE_RUNTIME 35
#define INIT_POSITION_RUNTIME_STR 30

// #define ENABLE_G2D
// #define ENABLE_FPS

#define ENABLE_G2D
#ifdef ENABLE_G2D
#define VIDEOCONVERT "imxvideoconvert_g2d"
#else
#define VIDEOCONVERT "videoconvert"
#endif

GMutex g_mutex;

/*
use gstreamer pipe line without GPU optimization. because meet bug
with videocrop aligment issue, so the case use the 

enviroment:
imx8mp, L5.15.5_1.0.0


*/


int count = 0;

static void
debug_image(uint8_t * raw_data, uint64_t data_size, int width, int height)
{
    count++;

    if(50 == count) {
            
        printf("appsink cap img shape %d x %d \n", width, height);


        // cv::Mat src_image(height, width, CV_8UC4, raw_data);
        // cv::Mat dst;
        // cv::cvtColor(src_image, dst, cv::COLOR_RGBA2BGR);
        // cv::imwrite("test.jpg", dst);




        count = 0;
    }


}


static GstFlowReturn
new_sample(GstElement* sink, gpointer* data)
{

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

    // printf("GSTMemory:%d\n",gst_is_dmabuf_memory(gst_buffer_peek_memory (buffer,0)));
    
    GstMapInfo mapinfo;
    if (!gst_buffer_map(buffer, &mapinfo, GST_MAP_READ)) {
        g_printerr("Unable to map video frame\n");
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    g_mutex_lock(&g_mutex);
    
    debug_image(mapinfo.data, mapinfo.size, width, height);

    gst_buffer_unmap(buffer, &mapinfo);
    gst_sample_unref(sample);


    g_mutex_unlock(&g_mutex);

    return GST_FLOW_OK;
}

static void
draw_overlay(GstElement* overlay,
             cairo_t*    cr,
             guint64     timestamp,
             guint64     duration,
             gpointer    user_data)
{

    //Set Cairo config
    cairo_set_line_width(cr, 3);
    cairo_select_font_face(cr,
                           "Courier",
                           CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, FONT_SIZE_RUNTIME);

    // Draw model runtime
    cairo_move_to(cr, 10, INIT_POSITION_RUNTIME_STR);
    cairo_set_font_size(cr, FONT_SIZE_LABEL_SCORE);

    // Draw runtime string
    char runtime_str[1024];

    float xmin = 100.0F;
    float xmax = 500.0F;
    float ymin = 100.0F;
    float ymax = 500.0F;

    g_mutex_lock(&g_mutex); // Lock the mutex to avoid new_sample overwriting data

    cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
    cairo_move_to(cr, xmin, ymin);
    cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
    cairo_move_to(cr, xmin, ymin);
    cairo_line_to(cr, xmax, ymin);
    cairo_line_to(cr, xmax, ymax);
    cairo_line_to(cr, xmin, ymax);
    cairo_line_to(cr, xmin, ymin);
    cairo_stroke_preserve(cr);

    // printf("* run into draw_overlay ! \n*");

    g_mutex_unlock(&g_mutex);
}



int main(int argc, char** argv)
{
    const gchar*   camera            = "/dev/video3";
    int            video_width       = WIDTH;
    int            video_height      = HEIGHT;
    int            input_width       = 300;
    int            input_height      = 300;


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
    GstElement* crop       = gst_element_factory_make("videocrop", "crop");
    GstElement* tee        = gst_element_factory_make("tee", "tee");
    GstElement* queue      = gst_element_factory_make("queue", "queue");
    GstElement* videoScale = gst_element_factory_make("videoscale", "scale");
    GstElement* convert    = gst_element_factory_make(VIDEOCONVERT, "convert");
    GstElement* appsink    = gst_element_factory_make("appsink", "appsink");
    GstElement* adaptor1   = gst_element_factory_make(VIDEOCONVERT, "adaptor1");
    GstElement* overlay    = gst_element_factory_make("cairooverlay", "overlay");
    GstElement* adaptor2   = gst_element_factory_make(VIDEOCONVERT, "adaptor2");
    GstElement* udp_sink   = gst_element_factory_make("udpsink", NULL);
    GstElement* vpuenc_h264 = gst_element_factory_make("vpuenc_h264", NULL);
    GstElement* rtph264pay  = gst_element_factory_make("rtph264pay", NULL);
#ifdef ENABLE_FPS
    GstElement* fpsdisplay = gst_element_factory_make("fpsdisplaysink", "fpsdisplay");
#else
    GstElement* display = gst_element_factory_make("autovideosink", "display");
#endif

    if (!source || !filter || !crop || !tee || !queue || !videoScale ||
        !convert || !appsink || !overlay || !udp_sink || !adaptor1 ||
        !adaptor2 || !rtph264pay || !vpuenc_h264 ) {
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


    //3.appsink
    const char* nn_data_format = "RGBx";
    GstCaps* appsink_caps = gst_caps_new_simple("video/x-raw",
                                "format", G_TYPE_STRING, nn_data_format,
                                 "width", G_TYPE_INT, video_width,
                                "height", G_TYPE_INT, video_height,
                                NULL);
    //filter has no property name drop,  but appsink have this.
    g_object_set(appsink,
                 "caps", appsink_caps,
                 "drop", FALSE,
                 "emit-signals", TRUE,
                 NULL);
    gst_caps_unref(appsink_caps);


    /* create callback funtion */
    g_signal_connect(appsink, "new-sample", G_CALLBACK(new_sample), NULL);
    g_signal_connect(overlay, "draw", G_CALLBACK(draw_overlay), NULL);


    g_object_set(queue, "leaky", 2, "max-size-buffers", 1, NULL);

    /* create and construct pipe line*/
    GstElement* cap_pipeline = gst_pipeline_new ("test-pipeline");


    // thread 1:  split cam input data flow.
    gst_bin_add_many(GST_BIN(cap_pipeline),
                         source,
                         filter,
                         crop,
                         tee,
                         queue,
                         videoScale,
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
                            //    adaptor2,
                               convert,
                               appsink,
                               NULL)) {
        g_printerr("Failed to link appsink cap_pipeline\n");
        return EXIT_FAILURE;
    }

    // thread 3:  display pipeline
    if (!gst_element_link_many(tee,
                               queue,
                               adaptor1,
                            //    overlay,
                            //    adaptor2,
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

    return EXIT_SUCCESS;

}