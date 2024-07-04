#include <iostream>
#include <json-c/json.h>
#include <libsoup/soup.h>
#include <libsoup/soup-server.h>
#include <gst/gst.h>
#include <string>
#include <sstream>

class VideoOverlay {
public:
    VideoOverlay() {
        gst_init(nullptr, nullptr);
        build_pipeline(352, 180, 320, 240);
    }

    ~VideoOverlay() {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    }

    void build_pipeline(int x, int y, int width, int height) {
        pipeline = gst_pipeline_new("video_pipeline");
        compositor = gst_element_factory_make("compositor", "comp");
        GstElement *video_convert = gst_element_factory_make("videoconvert", "convert");
        GstElement *waylandsink = gst_element_factory_make("waylandsink", "sink");
        GstElement *videotestsrc = gst_element_factory_make("videotestsrc", "source");

        if (!pipeline || !compositor || !video_convert || !waylandsink || !videotestsrc) {
            std::cerr << "Not all elements could be created." << std::endl;
            return;
        }

        g_object_set(videotestsrc, "pattern", 0, nullptr);
        g_object_set(compositor, "background", 0xff000000, nullptr);

        gst_bin_add_many(GST_BIN(pipeline), compositor, video_convert, waylandsink, videotestsrc, nullptr);
        gst_element_link_many(compositor, video_convert, waylandsink, nullptr);
        gst_element_link(videotestsrc, compositor);

        sink_pad = gst_element_request_pad_simple(compositor, "sink_%u");
        if (!sink_pad) {
            std::cerr << "Unable to get the requested pad" << std::endl;
            return;
        }

        g_object_set(sink_pad, "xpos", x, "ypos", y, "width", width, "height", height, nullptr);
    }

    void set_rectangle(int x, int y, int width, int height) {
        if (sink_pad) {
            g_object_set(sink_pad, "xpos", x, "ypos", y, "width", width, "height", height, nullptr);
        }
    }

    void start() {
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
    }

    void stop() {
        gst_element_set_state(pipeline, GST_STATE_NULL);
    }

private:
    GstElement *pipeline;
    GstElement *compositor;
    GstPad *sink_pad;
};

static VideoOverlay videoOverlay;

static void on_websocket_message(SoupWebsocketConnection *conn, SoupWebsocketDataType type, GBytes *message, gpointer user_data) {
    gsize size;
    const char *data = (const char *)g_bytes_get_data(message, &size);
    std::string msg(data, size);
    
    std::cout << "Received message from client: " << msg << std::endl;
    
    // Parse the message as JSON using json-c
    struct json_object *parsed_json;
    struct json_object *action, *dataObj, *left, *top, *width, *height;
    
    parsed_json = json_tokener_parse(msg.c_str());
    
    if (parsed_json != nullptr && json_object_object_get_ex(parsed_json, "action", &action)) {
        std::string actionStr = json_object_get_string(action);
        
        if (actionStr == "sendVideoRectangle" && json_object_object_get_ex(parsed_json, "data", &dataObj)) {
            // Extract rectangle data
            json_object_object_get_ex(dataObj, "left", &left);
            json_object_object_get_ex(dataObj, "top", &top);
            json_object_object_get_ex(dataObj, "width", &width);
            json_object_object_get_ex(dataObj, "height", &height);
            
            std::cout << "Received rectangle data - Left: " << json_object_get_int(left)
                      << ", Top: " << json_object_get_int(top)
                      << ", Width: " << json_object_get_int(width)
                      << ", Height: " << json_object_get_int(height) << std::endl;
            
            // Here, you can process the rectangle data as needed
        }
    } else {
        std::cout << "Received non-JSON message or failed to parse JSON" << std::endl;
    }
    
    // Clean up
    if (parsed_json != nullptr) {
        json_object_put(parsed_json);
    }
}

static void on_websocket_closed(SoupWebsocketConnection *conn, gpointer user_data) {
    std::cout << "WebSocket connection closed" << std::endl;
}

static void on_new_websocket(SoupServer* server,
                            SoupServerMessage* msg,
                            const char* path,
                            SoupWebsocketConnection* connection,
                            gpointer user_data)
{
    std::cout << "New WebSocket connection established" << std::endl;
    
    // Connect the message and closed signals
    g_signal_connect(connection, "message", G_CALLBACK(on_websocket_message), nullptr);
    g_signal_connect(connection, "closed", G_CALLBACK(on_websocket_closed), nullptr);
}

int main() {
    // Initialize main loop
    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
    
    // Create a SoupServer listening on port 8080
    SoupServer *server = soup_server_new(nullptr, nullptr);
    
    // Specify the port to listen on
    soup_server_listen_local(server, 8080, SOUP_SERVER_LISTEN_IPV4_ONLY, nullptr);
    
    // Set up the endpoint for WebSocket connections
    soup_server_add_websocket_handler(server, nullptr, nullptr, nullptr, on_new_websocket, nullptr, nullptr);
    
    std::cout << "WebSocket server running on port 8080" << std::endl;
    
    // Start the main loop
    g_main_loop_run(loop);
    
    // Cleanup
    g_object_unref(server);
    g_main_loop_unref(loop);
    
    return 0;
}
