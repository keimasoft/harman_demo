#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline, *src, *conv1, *scale, *filter, *flip, *invert, *conv2, *encoder, *pay, *sink;
    GstBus *bus;
    GstMessage *msg;
    GstCaps *caps;

    gst_init(&argc, &argv);

    if (argc < 3) {
        g_print("Usage: %s <server_ip> <server_port>\n", argv[0]);
        return -1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

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
                 "tune", 0x00000004,     // zerolatency enum/flag
                 "speed-preset", 1,      // 1 = ultrafast (reduces CPU cycles drastically)
                 "threads", 4,           // limit internal threads
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
        gst_message_unref(msg);
    }

    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;
}
