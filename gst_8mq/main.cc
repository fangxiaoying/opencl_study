#include <iostream>
#include <gst/gst.h>

#include <malloc.h>
#include "ocl/ocl_cv.h"

#define NANOSEC (1000000000)

#define OPEN_DEBUG
#define USE_APPSINK
#define BUFFER_NUMS   2


static GstElement *pipeline = NULL;
static int frame_cnt = 0;

struct appdata {
    GMutex g_mutex;
    ocl_function bgra2rgb;
    void* virtual_buffer[1];
};

static void
debug_image(uint8_t * raw_data, struct appdata* app, int width, int height)
{
  BGRA2RGB_run(&app->bgra2rgb, raw_data, app->virtual_buffer[0], width, height, false);

}

static GstFlowReturn on_sample_handler(GstElement *ele, gpointer* data)
{
  GstSample *sample = NULL;
  g_signal_emit_by_name (ele, "try-pull-sample", 1, &sample);
  if (sample == NULL) {
    g_print ("Failed to pull sample\n");
    return GST_FLOW_OK;
  }

  struct appdata* app = (struct appdata*) data;
  GstCaps*      caps       = gst_sample_get_caps(sample);
  GstStructure* gst_struct = gst_caps_get_structure(caps, 0);

  int width, height;
  gst_structure_get_int(gst_struct, "width", &width);
  gst_structure_get_int(gst_struct, "height", &height);

  bool ok = false;
  GstBuffer *buffer = gst_sample_get_buffer(sample);
  if (buffer) {
    GstMapInfo mapinfo;
    if (gst_buffer_map(buffer, &mapinfo, GST_MAP_READ)) {
      debug_image(mapinfo.data, app, width, height);
      ok = true;
      gst_buffer_unmap(buffer, &mapinfo);
    }
    gst_sample_unref(sample);
  }

  if (ok) {
    frame_cnt++;
  }
  return GST_FLOW_OK;
}

static bool save_dot = true;
static int last_cnt = 0;
static int count = 0;
static gboolean timeout (gpointer user_data)
{
  count++;
  if (save_dot && count > 3) {
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "test_pipeline");
    g_printerr("Save to dot\n");
    save_dot = false;
  }

  g_print("frame rate %d\n", frame_cnt - last_cnt);
  last_cnt = frame_cnt;
  return true;
}

int main (int argc, char **argv)
{
  GstState state;
  GError *error = NULL;

  ocl_device     imxgpu            = {0};
  ocl_function   rgba2bgr          = {0};
  struct appdata app               = {0};
  cl_init(&imxgpu);
  app.bgra2rgb.GPU = &imxgpu;
  BGRA2RGB_init(&app.bgra2rgb);
  size_t length = 300 * 300 * 3;
  app.virtual_buffer[0] = (uint8_t *)memalign(64, length * sizeof(uint8_t)); //RGB output

  gst_init (&argc, &argv);

#if defined(USE_APPSINK)
  const char *launch = "videotestsrc ! video/x-raw, framerate=120/1, format=NV12, width=1280, height=720 ! glupload ! glcolorscale ! glcolorconvert ! gldownload ! video/x-raw, format=BGRx, width=300, height=300 ! appsink name=mysink";
#else
  const char *launch = "videotestsrc ! video/x-raw, format=NV12, width=1280, height=720 ! glupload ! glcolorscale ! glcolorconvert ! gldownload ! video/x-raw, width=300, height=300 !  fpsdisplaysink sync=false video-sink=\"fakesink\"";
#endif

  pipeline = gst_parse_launch (launch, &error);
  if (error) {
    g_print ("Error while parsing pipeline description: %s\n", error->message);
    return -1;
  }

#if defined(USE_APPSINK)
  GstElement *ele =  gst_bin_get_by_name(GST_BIN(pipeline), "mysink");
  if (ele == NULL) {
    g_print ("Error getting appsink\n");
    exit(1);
  }
  g_object_set(ele, "emit-signals", TRUE, NULL);
  g_signal_connect(ele, "new-sample", G_CALLBACK(on_sample_handler), &app);
#endif

  GMainLoop * loop = g_main_loop_new (NULL, FALSE);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  if (gst_element_get_state (pipeline, &state, NULL,
                             GST_CLOCK_TIME_NONE) == GST_STATE_CHANGE_FAILURE ||
      state != GST_STATE_PLAYING) {
    g_warning ("State change to playing failed");
  }

  g_timeout_add(1000, timeout, NULL);

  bool saved = false;
  while (true)
  {
    g_main_context_iteration(g_main_loop_get_context(loop), FALSE);
  }
  g_main_loop_unref (loop);


  BGRA2RGB_release(&app.bgra2rgb);
  cl_release(app.bgra2rgb.GPU);
  return 0;
}