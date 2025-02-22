#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "SPIFFS.h"
#include <AccelStepper.h>
#include <ESPmDNS.h>

#define LED_PIN   8

// Define to use WiFi as STA or AP
#define USE_STA_MODE

// WiFi credentials (for station mode)
const char *ssid = "ssid";
const char *password = "password";
const char *hostname = "dev_tank";

// Stepper motor setup
#define IN1_PIN     4
#define IN2_PIN     3
#define IN3_PIN     2
#define IN4_PIN     1
#define LED_PIN     8
#define BUTTON_PIN  0
#define MAX_SPEED 600

AccelStepper stepper(AccelStepper::FULL4WIRE, IN1_PIN, IN3_PIN, IN2_PIN, IN4_PIN);

// Web server on port 80
AsyncWebServer server(80);
Preferences preferences;

// Motor control variables
int rotationSpeed = 50; // Percentage of max speed
int intervalTime = 15;  // Direction duration in seconds
bool motorRunning = false;
bool buttonPreviouslyPressed = false;
unsigned long previousMillis = 0;

void setupWebServer();
void setWifiLED(bool on);
void checkButton();
void motorThread(void *params);

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

  // Pin setup
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  // WiFi setup
#ifdef USE_STA_MODE
  setWifiLED(false);
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_15dBm);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf("Wifi status %d\r\n", WiFi.status());
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  setWifiLED(true);
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize mDNS
  if (!MDNS.begin(hostname)) {   // Set the hostname
    Serial.println("Error setting up MDNS responder!");
    while(1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
#else
  WiFi.softAP("Dev Tank", "film");
  Serial.println("Access Point Started");
#endif

    // Set stepper speed
    stepper.setMaxSpeed(MAX_SPEED);
    stepper.setSpeed(map(rotationSpeed, 0, 100, 0, MAX_SPEED));

    // Create a FreeRTOS task for motor control
    xTaskCreate(motorThread, "MotorControl", 2048, NULL, 1, NULL);

  // Start web server
  setupWebServer();
}

void loop() {
  if(WiFi.status() != WL_CONNECTED)
  {
    Serial.printf("Wifi status %d\r\n", WiFi.status());
    setWifiLED(false);
  }
  else
  {
    setWifiLED(true);
  }

  // TODO: Yeah this was connected to +5v... connect to 3.3v and try again
  //checkButton();
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
    Serial.printf("Set rotation speed to %d%\r\nSet interval time to %ds\r\n", rotationSpeed, intervalTime);
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

void motorThread(void *params)
{
  (void)params;

  for(;;)
  {
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

    // Loops every 1ms
    vTaskDelay( pdMS_TO_TICKS(1) );
  }
}

void setWifiLED(bool on)
{
  if(on)
  {
    digitalWrite(LED_PIN, LOW);
  }
  else
  {
    digitalWrite(LED_PIN, HIGH);
  }
}

void checkButton()
{
  int buttonState = digitalRead(BUTTON_PIN);
  if(HIGH == buttonState && !buttonPreviouslyPressed)
  {
    delay(50);
    if(HIGH == digitalRead(BUTTON_PIN))
    {
      motorRunning = !motorRunning;
      Serial.printf("Motor %s\r\n", motorRunning ? "started" : "stopped");
      buttonPreviouslyPressed = true;
    }
  }
  else
  if(LOW == buttonState && buttonPreviouslyPressed)
  {
    buttonPreviouslyPressed = false;
  }
}
