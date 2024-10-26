#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "SPIFFS.h"
#include <AccelStepper.h>

#define LED_PIN   8

// Define to use WiFi as STA or AP
#define USE_STA_MODE

// WiFi credentials (for station mode)
const char *ssid = "ssid";
const char *password = "password";

// Stepper motor setup
#define IN1_PIN 1
#define IN2_PIN 2
#define IN3_PIN 3
#define IN4_PIN 4
#define MAX_SPEED 600

AccelStepper stepper(AccelStepper::FULL4WIRE, IN1_PIN, IN3_PIN, IN2_PIN, IN4_PIN);

// Web server on port 80
AsyncWebServer server(80);
Preferences preferences;

// Motor control variables
int rotationSpeed = 50; // Percentage of max speed
int intervalTime = 15;  // Direction duration in seconds
bool motorRunning = false;
unsigned long previousMillis = 0;

void setupWebServer();

void setup() {
  Serial.begin(115200);

    // Initialize SPIFFS
    if(!SPIFFS.begin(true)){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

  // Load preferences
  preferences.begin("motor-config", false);
  rotationSpeed = preferences.getInt("rotationSpeed", 50);
  intervalTime = preferences.getInt("intervalTime", 15);

  // WiFi setup
#ifdef USE_STA_MODE
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
#else
  WiFi.softAP("Dev Tank", "film");
  Serial.println("Access Point Started");
#endif

    // Set stepper speed
    stepper.setMaxSpeed(MAX_SPEED);
    stepper.setSpeed(map(rotationSpeed, 0, 100, 0, MAX_SPEED));

  // Start web server
  setupWebServer();
}

void loop() {
  if (motorRunning) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= intervalTime * 1000) {
      previousMillis = currentMillis;
      // Reverse direction
      stepper.setSpeed(-stepper.speed());
      Serial.println("Direction changed");
    }
    stepper.runSpeed();
  }
}

void setupWebServer() {
  // Serve HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to load script.js file
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js", "application/javascript");
  });

  // Handle slider updates
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("rotation_speed", true)) {
      rotationSpeed = request->getParam("rotation_speed", true)->value().toInt();
    }
    if (request->hasParam("interval_time", true)) {
      intervalTime = request->getParam("interval_time", true)->value().toInt();
    }
    request->send(200, "text/plain", "OK");
    stepper.setSpeed(map(rotationSpeed, 0, 100, 0, MAX_SPEED));
    Serial.printf("Set rotation speed to %d%\nSet interval time to %ds\n", rotationSpeed, intervalTime);
  });

  // Handle start button
  server.on("/start", HTTP_POST, [](AsyncWebServerRequest *request) {
    motorRunning = true;
    request->send(200, "text/plain", "Motor started");
    Serial.println("Start");
  });

  // Handle stop button
  server.on("/stop", HTTP_POST, [](AsyncWebServerRequest *request) {
    motorRunning = false;
    request->send(200, "text/plain", "Motor stopped");
    Serial.println("Stop");
  });

  // Handle save button
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("rotation_speed", true)) {
      rotationSpeed = request->getParam("rotation_speed", true)->value().toInt();
    }
    if (request->hasParam("interval_time", true)) {
      intervalTime = request->getParam("interval_time", true)->value().toInt();
    }
    preferences.putInt("rotationSpeed", rotationSpeed);
    preferences.putInt("intervalTime", intervalTime);
    request->send(200, "text/plain", "Settings saved");
    stepper.setSpeed(map(rotationSpeed, 0, 100, 0, MAX_SPEED));
    Serial.println("Save");
  });

  // Handle get settings request
  server.on("/get_settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{\"rotation_speed\":" + String(rotationSpeed) + ", \"interval_time\":" + String(intervalTime) + "}";
    request->send(200, "application/json", json);
  });

  server.begin();
}
