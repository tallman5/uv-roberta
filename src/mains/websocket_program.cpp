#include <iostream>
#include <libwebsockets.h>

static int callback_signalr(struct lws* wsi, enum lws_callback_reasons reason, void* user, void* in, size_t len)
{
    switch (reason)
    {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            std::cout << "Connection to SignalR server established" << std::endl;
            // Perform any necessary setup or send initial messages
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            std::cout << "Received data from SignalR server: " << static_cast<const char*>(in) << std::endl;
            // Process the received data
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            std::cout << "Connection error: " << static_cast<const char*>(in) << std::endl;
            break;

        case LWS_CALLBACK_CLOSED:
            std::cout << "Connection to SignalR server closed" << std::endl;
            // Perform any necessary cleanup or reconnect logic
            break;

        default:
            break;
    }

    return 0;
}

struct lws_protocols protocols[] = {
    {
        "signalr.client",
        callback_signalr,
        0,
        128,
    },
    { NULL, NULL, 0, 0 } // Terminate the list of protocols
};

int main()
{
    struct lws_context* context;
    struct lws_client_connect_info connect_info;

    // Create the libwebsockets context
    struct lws_context_creation_info context_info = {};
    context_info.port = CONTEXT_PORT_NO_LISTEN;
    context_info.protocols = protocols;
    context_info.gid = -1;
    context_info.uid = -1;

    lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_DEBUG | LLL_PARSER | LLL_HEADER, nullptr);

    context = lws_create_context(&context_info);
    if (!context)
    {
        std::cerr << "Failed to create libwebsockets context" << std::endl;
        return 1;
    }

    // Set up connection info
    connect_info.context = context;
    connect_info.address = "192.168.1.167";
    connect_info.port = 5000;
    connect_info.path = "/robertaHub";
    connect_info.host = connect_info.address;
    connect_info.origin = connect_info.address;
    connect_info.protocol = "signalr.client";
    connect_info.ietf_version_or_minus_one = -1;
    connect_info.userdata = nullptr;

    // Connect to the SignalR server
    struct lws* wsi = lws_client_connect_via_info(&connect_info);
    if (!wsi)
    {
        std::cerr << "Failed to connect to SignalR server" << std::endl;
        lws_context_destroy(context);
        return 1;
    }

    // Enter the event loop
    while (true)
    {
        lws_service(context, 50);
        // Additional processing or logic can be added here
    }

    // Clean up and exit
    lws_context_destroy(context);

    return 0;
}
