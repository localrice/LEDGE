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

  int id = doc["id"];
  String text = doc["text"];
  String animation = doc["animation"];
  int speed = doc["speed"];

  currentText = text;
  scrollSpeed = speed;

  // Match animation style to effects
  if (animation == "scroll-left") {
    animIn = PA_SCROLL_LEFT;
    animOut = PA_SCROLL_LEFT;
  } else if (animation == "scroll-right") {
    animIn = PA_SCROLL_RIGHT;
    animOut = PA_SCROLL_RIGHT;
  } else if (animation == "fade") {
    animIn = PA_FADE;
    animOut = PA_FADE;
  } else if (animation == "wipe") {
    animIn = PA_WIPE;
    animOut = PA_WIPE;
  } else {
    animIn = PA_PRINT;
    animOut = PA_NO_EFFECT;
  }


   // Set up display
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