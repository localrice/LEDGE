#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <SPI.h>

// LED matrix configuration
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define DATA_PIN D7
#define CLK_PIN  D5
#define CS_PIN   D8

MD_Parola display = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

const char* ssid = "no";
const char* password = "xdtamate69";

AsyncWebServer server(80);
WebSocketsServer webSocket(81);

// default display variables
String currentText = "";
textEffect_t animIn = PA_SCROLL_LEFT;
textEffect_t animOut = PA_SCROLL_LEFT;
uint16_t scrollSpeed = 50;


void handleWebSocketMessage(uint8_t num, uint8_t* payload, size_t length) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.println("Failed to parse JSON");
    return;
  }

  // Extract values from JSON
  int id = doc["id"];
  String text = doc["text"];
  String animation = doc["animation"];
  int speed = doc["speed"];
  bool invert = doc["invert"] | false;  // Default false if not provided

  currentText = text;
  scrollSpeed = speed;

  // Match animation style
 if (animation == "PA_PRINT") {
  animIn = PA_PRINT; animOut = PA_NO_EFFECT;
} else if (animation == "PA_SLICE") {
  animIn = animOut = PA_SLICE;
} else if (animation == "PA_MESH") {
  animIn = animOut = PA_MESH;
} else if (animation == "PA_FADE") {
  animIn = animOut = PA_FADE;
} else if (animation == "PA_WIPE") {
  animIn = animOut = PA_WIPE;
} else if (animation == "PA_WIPE_CURSOR") {
  animIn = animOut = PA_WIPE_CURSOR;
} else if (animation == "PA_OPENING") {
  animIn = animOut = PA_OPENING;
} else if (animation == "PA_OPENING_CURSOR") {
  animIn = animOut = PA_OPENING_CURSOR;
} else if (animation == "PA_CLOSING") {
  animIn = animOut = PA_CLOSING;
} else if (animation == "PA_CLOSING_CURSOR") {
  animIn = animOut = PA_CLOSING_CURSOR;
} else if (animation == "PA_RANDOM") {
  animIn = animOut = PA_RANDOM;
} else if (animation == "PA_BLINDS") {
  animIn = animOut = PA_BLINDS;
} else if (animation == "PA_DISSOLVE") {
  animIn = animOut = PA_DISSOLVE;
} else if (animation == "PA_SCROLL_UP") {
  animIn = animOut = PA_SCROLL_UP;
} else if (animation == "PA_SCROLL_DOWN") {
  animIn = animOut = PA_SCROLL_DOWN;
} else if (animation == "PA_SCROLL_LEFT") {
  animIn = animOut = PA_SCROLL_LEFT;
} else if (animation == "PA_SCROLL_RIGHT") {
  animIn = animOut = PA_SCROLL_RIGHT;
} else if (animation == "PA_SCROLL_UP_LEFT") {
  animIn = animOut = PA_SCROLL_UP_LEFT;
} else if (animation == "PA_SCROLL_UP_RIGHT") {
  animIn = animOut = PA_SCROLL_UP_RIGHT;
} else if (animation == "PA_SCROLL_DOWN_LEFT") {
  animIn = animOut = PA_SCROLL_DOWN_LEFT;
} else if (animation == "PA_SCROLL_DOWN_RIGHT") {
  animIn = animOut = PA_SCROLL_DOWN_RIGHT;
} else if (animation == "PA_SCAN_HORIZ") {
  animIn = animOut = PA_SCAN_HORIZ;
} else if (animation == "PA_SCAN_HORIZX") {
  animIn = animOut = PA_SCAN_HORIZX;
} else if (animation == "PA_SCAN_VERT") {
  animIn = animOut = PA_SCAN_VERT;
} else if (animation == "PA_SCAN_VERTX") {
  animIn = animOut = PA_SCAN_VERTX;
} else if (animation == "PA_GROW_UP") {
  animIn = animOut = PA_GROW_UP;
} else if (animation == "PA_GROW_DOWN") {
  animIn = animOut = PA_GROW_DOWN;
} else {
  animIn = PA_PRINT;
  animOut = PA_NO_EFFECT;
}


  // Invert display if requested
  display.setInvert(invert);

  // Set up and show
  display.displayClear();
  display.displayText(currentText.c_str(), PA_CENTER, scrollSpeed, 1000, animIn, animOut);

  Serial.println("Updated display via WebSocket");
}


void setup() {
  Serial.begin(115200);
  ESP.getResetReason();

  display.begin();
  display.setIntensity(0); // Set brightness (0-15)
  display.displayClear();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    yield();
    Serial.println(".");
  }

  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());
  LittleFS.begin();
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  // websocket
  webSocket.begin();
  webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
      Serial.printf("Message from client %d: %s\n", num, payload);
      // Echo the message back to the client
      handleWebSocketMessage(num, payload, length);
    }
  });


  server.begin();
}

void loop() {
    webSocket.loop();
    if (display.displayAnimate()) {
      display.displayReset();  // Restart the animation from the beginning
    }
}