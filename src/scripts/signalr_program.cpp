#include <iostream>
#include <vector>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <libwebsockets.h>
#include <nlohmann/json.hpp>

/*
g++ -o signalr_program signalr_program.cpp -lwebsockets -lssl -lcrypto
*/

using namespace std;
using namespace std::chrono;
using json = nlohmann::json;

// Define the ChannelData struct
struct ChannelData
{
  vector<int> channelValues;
  bool inFailsafe;
  bool ch17, ch18;
  system_clock::time_point timestamp;
};

static struct lws_extension exts[] = {
    {"permessage-deflate",
     lws_extension_callback_pm_deflate,
     "permessage-deflate; client_max_window_bits"},
    {nullptr, nullptr, nullptr}};

// WebSocket context
struct WebsocketContext
{
  struct lws_context *context;
  struct lws *wsi;
  volatile bool connected;
};

// WebSocket callback function
static int websocketCallback(struct lws_context *context,
                             struct lws *wsi,
                             enum lws_callback_reasons reason,
                             void *user, void *in, size_t len)
{
  switch (reason)
  {
  case LWS_CALLBACK_CLIENT_ESTABLISHED:
    // Connection established
    reinterpret_cast<WebsocketContext *>(user)->connected = true;
    break;

  case LWS_CALLBACK_CLIENT_RECEIVE:
    // Handle received messages here
    cout << "Received message: " << static_cast<char *>(in) << endl;
    break;

  default:
    break;
  }

  return 0;
}

int main()
{
  // Initialize the WebSocket context
  WebsocketContext websocketContext;
  websocketContext.connected = false;

  // WebSocket client creation
  struct lws_context_creation_info info;
  memset(&info, 0, sizeof(info));
  info.port = CONTEXT_PORT_NO_LISTEN;
  info.iface = nullptr;
  info.protocols = nullptr;
  info.extensions = exts;
  info.gid = -1;
  info.uid = -1;
  info.options = 0;
  info.user = &websocketContext;

  websocketContext.context = lws_create_context(&info);
  if (websocketContext.context == nullptr)
  {
    cerr << "Failed to create WebSocket context" << endl;
    return 1;
  }

  // WebSocket connection URL
  const string url = "http://192.168.1.167:5000/robertaHub";

  // Create the WebSocket connection
  struct lws_client_connect_info connect_info;
  memset(&connect_info, 0, sizeof(connect_info));
  connect_info.context = websocketContext.context;
  connect_info.address = url.c_str();
  connect_info.port = 5000;
  connect_info.path = "/robertaHub";
  connect_info.host = "192.168.1.167";
  connect_info.origin = nullptr;
  connect_info.protocol = nullptr;

  websocketContext.wsi = lws_client_connect_via_info(&connect_info);
  // websocketContext.wsi = lws_client_connect(websocketContext.context, "localhost", 5000, 0, "/robertaHub", "localhost", nullptr, nullptr, -1);
  if (websocketContext.wsi == nullptr)
  {
    cerr << "Failed to connect to " << url << endl;
    lws_context_destroy(websocketContext.context);
    return 1;
  }

  // Wait for the connection to be established
  // Wait for the connection to be established
  while (!websocketContext.connected)
  {
    lws_service(websocketContext.context, 0);
    usleep(1000);
  }

  // Create an instance of the ChannelData struct
  ChannelData data;
  data.channelValues = {1, 2, 3};
  data.inFailsafe = false;
  data.ch17 = true;
  data.ch18 = false;
  data.timestamp = system_clock::now();

  // Convert the struct to a JSON string
  json j;
  j["channelData"] = data.channelValues;
  j["inFailsafe"] = data.inFailsafe;
  j["ch17"] = data.ch17;
  j["ch18"] = data.ch18;
  stringstream ss;
  ss << setprecision(numeric_limits<double>::digits10 + 1)
     << duration<double>(data.timestamp.time_since_epoch()).count();
  j["timestamp"] = ss.str();

  // Create the payload
  json payload;
  payload["methodName"] = "UpdateRxState";
  payload["arguments"] = {j};

  // Convert the payload to a string
  const string payloadStr = payload.dump();

  // Send the payload to the hub
  lws_write(websocketContext.wsi, reinterpret_cast<unsigned char *>(const_cast<char *>(payloadStr.c_str())), payloadStr.length(), LWS_WRITE_TEXT);

  // Wait for user input to keep the program running
  cout << "Press Enter to exit..." << endl;
  string input;
  getline(cin, input);

  // Clean up and destroy the WebSocket context
  lws_context_destroy(websocketContext.context);

  return 0;
}
