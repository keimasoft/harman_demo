#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    GKeyFile *config;
    GError *error = NULL;
    GstElement *pipeline, *previous = NULL;
    GstBus *bus;
    GstMessage *msg;
    gchar **elements;
    gsize num_elements = 0;
    gint port;
    gchar *caps_str;

    gst_init(&argc, &argv);

    config = g_key_file_new();
    if (!g_key_file_load_from_file(config, "./cfg/server.ini", G_KEY_FILE_NONE, &error)) {
        g_printerr("[-] Config error: %s\n", error->message);
        g_clear_error(&error);
        g_key_file_free(config);
        return -1;
    }

    port = g_key_file_get_integer(config, "Network", "port", NULL);
    caps_str = g_key_file_get_string(config, "Network", "caps", NULL);
    elements = g_key_file_get_string_list(config, "Pipeline", "elements", &num_elements, NULL);

    if (num_elements == 0) {
        g_free(caps_str);
        g_key_file_free(config);
        return -1;
    }

    pipeline = gst_pipeline_new("server-pipeline");

    for (gsize i = 0; i < num_elements; i++) {
        gchar *type = elements[i];
        gchar *name = g_key_file_get_string(config, "Names", type, NULL);
        if (!name) name = g_strdup(type);

        GstElement *el = gst_element_factory_make(type, name);
        if (!el) {
            g_printerr("[-] Failed to create element: %s\n", type);
            g_free(name);
            g_strfreev(elements);
            g_free(caps_str);
            g_key_file_free(config);
            gst_object_unref(pipeline);
            return -1;
        }

        gst_bin_add(GST_BIN(pipeline), el);

        if (g_strcmp0(type, "udpsrc") == 0) {
            g_object_set(G_OBJECT(el), "port", port, NULL);
        } else if (g_strcmp0(type, "capsfilter") == 0 && caps_str != NULL) {
            GstCaps *caps = gst_caps_from_string(caps_str);
            g_object_set(G_OBJECT(el), "caps", caps, NULL);
            gst_caps_unref(caps);
        }

        if (previous != NULL) {
            if (!gst_element_link(previous, el)) {
                g_printerr("[-] Failed to link elements\n");
                g_strfreev(elements);
                g_free(caps_str);
                g_key_file_free(config);
                gst_object_unref(pipeline);
                return -1;
            }
        }

        previous = el;
        g_free(name);
    }

    g_strfreev(elements);
    g_free(caps_str);
    g_key_file_free(config);

    g_print("[+] Server listening on UDP port %d...\n", port);
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
