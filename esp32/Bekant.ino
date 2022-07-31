#include <WiFi.h>
#include "server.h"
#include "lin.h"
#include "data.h"

const char* ssid     = "";
const char* password = "";

#define ONBOARD_LED  2
constexpr uint8_t empty[3] = {0,0,0};

BekantServer server;
Lin lin;
BekantData data(server, lin);

bool wifiConnected = false;
uint64_t wifiRecoonectTimeout = 0;

void setup()
{
    pinMode(ONBOARD_LED,OUTPUT);

    Serial.begin(115200);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    wifiConnected = true;
    digitalWrite(ONBOARD_LED, HIGH);

    lin.setup();
   
    // Create the linbus handler
    TaskHandle_t handle;
    //xTaskCreate(&BekantServer::threadFunc, "serverFunc", 4000, &server, 0, &handle);
    if(xTaskCreatePinnedToCore(&BekantServer::threadFunc, "serverFunc", 4000, &server, 1, &handle, 0) != pdPASS)
    {
      Serial.printf("server task failed\n");
    }

    Serial.printf("starting on %d\n", xPortGetCoreID());
}

void verifyWifi() 
{
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    digitalWrite(ONBOARD_LED, LOW);
    if(wifiRecoonectTimeout < esp_timer_get_time()) {
      wifiRecoonectTimeout = esp_timer_get_time() + 30000000;
      WiFi.reconnect();
    }
  }
  if (!wifiConnected && WiFi.status() == WL_CONNECTED) {
    digitalWrite(ONBOARD_LED, HIGH);
    wifiConnected = true;
  }
}

void loop(){
  verifyWifi();

  data.sendPreFrame();
  usleep(500);
  uint32_t node8 = data.getEnginePos(BekantData::Engine8);
  usleep(500);
  uint32_t node9 = data.getEnginePos(BekantData::Engine9);
  data.setEnginePos(node8, node9);
  usleep(500);
  data.sendCommandFrame();

  usleep(100000);
}

