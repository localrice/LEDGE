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

// Button pins
#define BUTTON1_PIN D1
#define BUTTON2_PIN D2

// Create LED matrix instances
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
MD_Parola display = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// WiFi credentials
const char* ssid = "no";
const char* password = "xdtamate69";

// Server & WebSocket
AsyncWebServer server(80);
WebSocketsServer webSocket(81);

// Display variables
String currentText = "";
textEffect_t animIn = PA_SCROLL_LEFT;
textEffect_t animOut = PA_SCROLL_LEFT;
uint16_t scrollSpeed = 50;
int mode = 1;  // 1 = all LEDs on, 2 = config display

// Forward declarations
void displayAllLEDsOn();
void loadConfigAndDisplay();
void matchAnimation(const String& animation);
void saveConfig(const char* json);

void handleWebSocketMessage(uint8_t num, uint8_t* payload, size_t length) {
  Serial.println("WebSocket message received");

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.println("Failed to parse incoming JSON");
    return;
  }

  saveConfig((const char*)payload);
  Serial.println("Config saved to /config.json");

  currentText = doc["text"] | "";
  String animation = doc["animation"] | "PA_PRINT";
  scrollSpeed = doc["speed"] | 50;
  bool invert = doc["invert"] | false;

  matchAnimation(animation);
  display.setInvert(invert);
  display.displayClear();
  display.displayText(currentText.c_str(), PA_CENTER, scrollSpeed, 1000, animIn, animOut);

  mode = 2;

  Serial.println("Display updated from WebSocket");
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Booting...");

  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  mx.begin();
  display.begin();
  display.setIntensity(5);
  display.displayClear();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    return;
  }
  Serial.println("LittleFS mounted");

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  webSocket.begin();
  webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
      Serial.printf("Client %d sent: %s\n", num, payload);
      handleWebSocketMessage(num, payload, length);
    }
  });

  server.begin();
  Serial.println("Server started");
}

void loop() {
  webSocket.loop();

  if (digitalRead(BUTTON1_PIN) == LOW) {
    Serial.println("Button 1 pressed: Switching to mode 1 (all LEDs on)");
    mode = 1;
    displayAllLEDsOn();
    delay(300);
  }
  if (digitalRead(BUTTON2_PIN) == LOW) {
    Serial.println("Button 2 pressed: Switching to mode 2 (config display)");
    mode = 2;
    loadConfigAndDisplay();
    delay(300);
  }

  if (mode == 2) {
    if (display.displayAnimate()) {
      display.displayReset();
    }
  }
}

void displayAllLEDsOn() {
  Serial.println("Turning on all LEDs...");

  mx.begin();               // Make sure mx is initialized
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);  // Disable auto update for performance
  mx.clear();               // Clear the display before drawing

  for (uint8_t x = 0; x < MAX_DEVICES * 8; x++) {
    for (uint8_t y = 0; y < 8; y++) {
      mx.setPoint(y, x, true);  // Be careful with X/Y order depending on orientation
    }
  }

  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);  // Enable auto update
  mx.update();              // Force update
  Serial.println("All LEDs ON.");
}


void loadConfigAndDisplay() {
  if (!LittleFS.exists("/config.json")) {
    Serial.println("No config file found");
    return;
  }

  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return;
  }

  String json = configFile.readString();
  configFile.close();
  Serial.println("Loaded config JSON: " + json);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.println("Failed to parse saved config JSON");
    return;
  }

  currentText = doc["text"] | "";
  String animation = doc["animation"] | "PA_PRINT";
  scrollSpeed = doc["speed"] | 50;
  bool invert = doc["invert"] | false;

  matchAnimation(animation);

  display.setInvert(invert);
  display.displayClear();
  display.displayText(currentText.c_str(), PA_CENTER, scrollSpeed, 1000, animIn, animOut);

  Serial.println("Display loaded from config");
}

void saveConfig(const char* json) {
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }
  configFile.print(json);
  configFile.close();
  Serial.println("Config file saved successfully");
}

void matchAnimation(const String& animation) {
  if (animation == "PA_PRINT") animIn = PA_PRINT, animOut = PA_NO_EFFECT;
  else if (animation == "PA_SLICE") animIn = animOut = PA_SLICE;
  else if (animation == "PA_MESH") animIn = animOut = PA_MESH;
  else if (animation == "PA_FADE") animIn = animOut = PA_FADE;
  else if (animation == "PA_WIPE") animIn = animOut = PA_WIPE;
  else if (animation == "PA_WIPE_CURSOR") animIn = animOut = PA_WIPE_CURSOR;
  else if (animation == "PA_OPENING") animIn = animOut = PA_OPENING;
  else if (animation == "PA_OPENING_CURSOR") animIn = animOut = PA_OPENING_CURSOR;
  else if (animation == "PA_CLOSING") animIn = animOut = PA_CLOSING;
  else if (animation == "PA_CLOSING_CURSOR") animIn = animOut = PA_CLOSING_CURSOR;
  else if (animation == "PA_RANDOM") animIn = animOut = PA_RANDOM;
  else if (animation == "PA_BLINDS") animIn = animOut = PA_BLINDS;
  else if (animation == "PA_DISSOLVE") animIn = animOut = PA_DISSOLVE;
  else if (animation == "PA_SCROLL_UP") animIn = animOut = PA_SCROLL_UP;
  else if (animation == "PA_SCROLL_DOWN") animIn = animOut = PA_SCROLL_DOWN;
  else if (animation == "PA_SCROLL_LEFT") animIn = animOut = PA_SCROLL_LEFT;
  else if (animation == "PA_SCROLL_RIGHT") animIn = animOut = PA_SCROLL_RIGHT;
  else if (animation == "PA_SCROLL_UP_LEFT") animIn = animOut = PA_SCROLL_UP_LEFT;
  else if (animation == "PA_SCROLL_UP_RIGHT") animIn = animOut = PA_SCROLL_UP_RIGHT;
  else if (animation == "PA_SCROLL_DOWN_LEFT") animIn = animOut = PA_SCROLL_DOWN_LEFT;
  else if (animation == "PA_SCROLL_DOWN_RIGHT") animIn = animOut = PA_SCROLL_DOWN_RIGHT;
  else if (animation == "PA_SCAN_HORIZ") animIn = animOut = PA_SCAN_HORIZ;
  else if (animation == "PA_SCAN_HORIZX") animIn = animOut = PA_SCAN_HORIZX;
  else if (animation == "PA_SCAN_VERT") animIn = animOut = PA_SCAN_VERT;
  else if (animation == "PA_SCAN_VERTX") animIn = animOut = PA_SCAN_VERTX;
  else if (animation == "PA_GROW_UP") animIn = animOut = PA_GROW_UP;
  else if (animation == "PA_GROW_DOWN") animIn = animOut = PA_GROW_DOWN;
  else animIn = PA_PRINT, animOut = PA_NO_EFFECT;

  Serial.println("Animation matched: " + animation);
}
