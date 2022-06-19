#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char * ssid = "RickyBobby";
const char * password = "shakeandbake";
//gotta google the pinout, board label is wrong
int PINS_TEMP[] = {16,5,4,0};

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <h1>ESP WebSocket Server</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2 id="txtInMsg">Incoming MSG</h2>
      <input id="txtMsg" />
      <p><button id="button" class="button">Send</button></p>
    </div>
  </div>
<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {    
    document.getElementById('txtInMsg').innerHTML = event.data;
  }
  function onLoad(event) {
    initWebSocket();
    initButton();
  }
  function initButton() {
    document.getElementById('button').addEventListener('click', sendMsg);
  }
  function sendMsg(){
    websocket.send(document.getElementById('txtMsg').value);
  }
</script>
</body>
</html>
)rawliteral";

void notifyClients() {
  ws.textAll("ECHO");
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    Serial.print("REC:");
    Serial.println((char*)data);
    String msgIn = String((char*)data);
    processCMD(msgIn);
    
  }
}

void processCMD(String in){
  /*if (strcmp((char*)data, "toggle") == 0) {
      ledState = !ledState;
      notifyClients();
    }*/
    
    if (in.indexOf("init") >= 0) {
      ws.textAll("a-okay");
    }else if (in.indexOf(":on") >= 0 || in.indexOf(":off") >= 0  ){
      //the input is the index of the LED followed by command
      int idx = in.substring(0, in.indexOf(":")).toInt();     
      String status = in.substring(in.indexOf(":") + 1);
      if (status == "on"){
        Serial.println(String(idx) + ":ON");
        digitalWrite(PINS_TEMP[idx],HIGH);
      } else {
         Serial.println(String(idx) + ":OFF");
         digitalWrite(PINS_TEMP[idx],LOW);
      }

    
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    
  }
  return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
 
  for(int i=0; i < 4; i++){
    pinMode(PINS_TEMP[i],OUTPUT);
    digitalWrite(PINS_TEMP[i],HIGH);
    delay(1000);
    digitalWrite(PINS_TEMP[i],LOW);
  }
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Start server
  server.begin();
}

void loop() {
  ws.cleanupClients();
}
