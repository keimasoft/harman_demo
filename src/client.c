#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>

static int run_client(const char *ip, int port) {
    GstElement *pipeline, *src, *conv1, *scale, *filter, *flip, *invert, *conv2, *encoder, *pay, *sink;
    GstBus *bus;
    GstMessage *msg;
    GstCaps *caps;

    pipeline = gst_pipeline_new("client-pipeline");
    src      = gst_element_factory_make("v4l2src", "camera_src");
    conv1    = gst_element_factory_make("videoconvert", "conv_1");
    scale    = gst_element_factory_make("videoscale", "scaler");
    filter   = gst_element_factory_make("capsfilter", "caps_filter");
    flip     = gst_element_factory_make("videoflip", "vertical_flip");
    invert   = gst_element_factory_make("frei0r-filter-invert0r", "color_invert");
    conv2    = gst_element_factory_make("videoconvert", "conv_2");
    encoder  = gst_element_factory_make("x264enc", "h264_encoder");
    pay      = gst_element_factory_make("rtph264pay", "rtp_payloader");
    sink     = gst_element_factory_make("udpsink", "udp_sink");

    if (!pipeline || !src || !conv1 || !scale || !filter || !flip || !invert || !conv2 || !encoder || !pay || !sink) {
        g_printerr("[-] Element creation failed.\n");
        return -1;
    }

    g_object_set(G_OBJECT(src), "device", "/dev/video0", NULL);
    g_object_set(G_OBJECT(flip), "method", 4, NULL);
    caps = gst_caps_from_string("video/x-raw, width=640, height=480");
    g_object_set(G_OBJECT(filter), "caps", caps, NULL);
    gst_caps_unref(caps);

    g_object_set(G_OBJECT(encoder),
                 "tune", 0x00000004,
                 "speed-preset", 1,
                 "threads", 4,
                 NULL);

    g_object_set(G_OBJECT(sink), "host", ip, "port", port, "sync", FALSE, NULL);

    gst_bin_add_many(GST_BIN(pipeline), src, conv1, scale, filter, flip, invert, conv2, encoder, pay, sink, NULL);

    if (!gst_element_link_many(src, conv1, scale, filter, flip, invert, conv2, encoder, pay, sink, NULL)) {
        g_printerr("[-] Link failed.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    g_print("[+] Streaming to %s:%d...\n", ip, port);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    if (msg != NULL) {
        if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
            GError *err = NULL;
            gchar *dbg = NULL;
            gst_message_parse_error(msg, &err, &dbg);
            g_printerr("[-] Pipeline error: %s\n", err->message);
            g_clear_error(&err);
            g_free(dbg);
        }
        gst_message_unref(msg);
    }

    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    if (argc < 3) {
        g_print("Usage: %s <server_ip> <server_port>\n", argv[0]);
        return -1;
    }

    return run_client(argv[1], atoi(argv[2]));
}
