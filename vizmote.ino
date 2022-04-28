#include "button.h"
#include "secrets.h"
#include "blinker.h"
#include <List.hpp>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <time.h>
#include "net_helpers.h"
#include <LittleFS.h>
#include "wiz.h"
#include "config.h"

#define LIGHT_PORT 38899
#define RECEIVE_PORT 4210

#define MAX_WAIT 13000
#define MAX_INTERVAL 3000

#define STATE_BULB_UNAVAILABLE -1

#define FILE_LIGHT_CONFIG "/lights.json"

#define LED_PIN 0

#define BUTTON_PIN 5
#define BUTTON_TWO_PIN 4

LedBlinker Blink(LED_PIN);
ButtonController Buttons(BUTTON_PIN, BUTTON_TWO_PIN);

char packet[255];

WiFiClient net= WiFiClient();
WiFiUDP UDP;

unsigned int chill = 250;

bool fetched;
StaticJsonDocument<255> responseDoc;

static const char DOC_TEMPLATE[] = R"EOF({"scene" : [], "lights": [] }})EOF";

void parseStateConfigDoc(DynamicJsonDocument stateDoc) {
  JsonArray lights  = stateDoc["lights"];
  for (int i = 0 ; i < lights.size() ; ++i) {
    registerFoundBulb(lights[i]["mac"], lights[i]["address"]);
  }

  JsonArray scene  = stateDoc["scene"];
  for (int i = 0 ; i < scene.size() ; ++i) {
    addBulbToScene(scene[i]);
  }
}

void populateStateConfigDoc(DynamicJsonDocument* doc) {
  JsonArray lights  = doc->createNestedArray("lights");

  for (int i = 0 ; i < universe.getSize() ; ++i) {
    wizbulb bulb = universe[i];
    JsonObject sBulb = lights.createNestedObject();
    
    sBulb["mac"] = bulb.mac;
    sBulb["address"] = bulb.address;
  }
  JsonArray scene  = doc->createNestedArray("scene");

  for (int i = 0 ; i < sceneBulbs.getSize() ; ++i) {
    wizaddress wa = sceneBulbs[i];
    Serial.print("sm ");
    Serial.println(wa.mac);
    if (!scene.add(wa.mac)) {
      Serial.println("Couldn't add scene. Doc full");
    }
  }
}

void initStateDoc() {
  if (LittleFS.exists(FILE_LIGHT_CONFIG)) {
    loadJSON(FILE_LIGHT_CONFIG, parseStateConfigDoc);
  } else {
  }
  printState();
}

void ipRangeScan() {
  unsigned int r_st = millis();

  Blink.blink();
  
  clearSceneBulbs();
  
  for (int i = 0; i < 256 ; ++i) {
    IPAddress address = IPAddress(192, 168, 1, i);
    Serial.print("Testing IP ");
    Serial.println(address);
    udpMessage(GET_LIGHT_STATE, address);
    Blink.loop();
    delay(chill);
    udpReceive(registerPacketReceived);
  }
  reportProgress(millis() - r_st);
  
  Serial.println("Saving Config");
  saveJSON(FILE_LIGHT_CONFIG, populateStateConfigDoc);
  Blink.ledOFF();
}

void reportProgress(unsigned int span) {
  printState();
  Serial.print("Discovered: [");
  int count = universe.getSize();
  Serial.println("]");
  Serial.print(count);
  Serial.print(" Devices in ");
  Serial.print(span / 1000);
  Serial.println(" seconds.");
}

void startListening() {
  // Begin listening to UDP port
  UDP.begin(RECEIVE_PORT);
  Serial.print("Listening on UDP port ");
  Serial.println(RECEIVE_PORT);
}

void udpMessage(const char msg[], auto address) {
  UDP.beginPacket(address, 38899);
  Serial.println(msg);
  UDP.write(msg);
  UDP.endPacket();
}

void registerPacketReceived(IPAddress address, char packet[]) {
  deserializeJson(responseDoc, packet);
  int scene = responseDoc["result"]["sceneId"];
  const char* mac = responseDoc["result"]["mac"];
  
  char addressRaw[16];
  sprintf(addressRaw, "%d.%d.%d.%d", address[0], address[1], address[2], address[3]);
  const char* addressPtr = addressRaw;

  registerFoundBulb(mac, addressPtr);
  
  if (scene == 19) {
    Serial.println("adding to scene");
    addBulbToScene(mac);
  }
  printState();
}

void noOp(IPAddress address, char packet[]) {
  
}

boolean udpRawReceive() {
  // If packet received...
  int packetSize = UDP.parsePacket();
  if (packetSize) {
    Serial.print("Received packet! Size: ");
    Serial.println(packetSize); 
    int len = UDP.read(packet, 255);
    if (len > 0)
    {
      packet[len] = '\0';
    }
    Serial.print("Packet received from [");
    Serial.print(UDP.remoteIP());
    Serial.print("] ");
    Serial.println(packet);
    return true;
  } 
  return false;
}

boolean udpReceive(void (*pFunc)(IPAddress address, char packet[])) {
  if (udpRawReceive()) {
    pFunc(UDP.remoteIP(), packet);
    return true;
  }
  return false;
}


int getBulbStateReliable(const char* outboundMessage, const char* address) {
  boolean timedOut = false;
  unsigned int t_st = millis();

  int next_interval = 750;
  unsigned int next_send = millis() - 1;
  while ((millis() - t_st) < MAX_WAIT) {
    if (udpRawReceive()) {
      
      deserializeJson(responseDoc, packet);
      return responseDoc["result"]["state"];
      
    } else if (millis() > next_send) {
      //Repeat message
      udpMessage(outboundMessage, address);
      
      next_send = millis() + next_interval;
      if (next_interval < MAX_INTERVAL) {
        next_interval = next_interval * 2;
      }
    }
  }
  Serial.println("Connection to bulb timed out.");
  return STATE_BULB_UNAVAILABLE;
}

void setStateOnBulbs(bool newState) {
  for (int i = 0; i < sceneBulbs.getSize(); ++i) {
    wizaddress wa = sceneBulbs[i];
    
    if (newState) {
      udpMessage(SET_ON, getBulbAddress(wa).address);
    } else {
      udpMessage(SET_OFF, getBulbAddress(wa).address);
    }
    //TODO: We should be confirming message receipt
//    delay(chill);
//    udpReceive(noOp);
  }
}

int actionState = 0;
int stateLengths = 1;

void trigger() {
  Serial.print("Action: ");
  Serial.println(actionState);
  if (actionState == 0) {
    if(isScenePopulated()) {
      wizaddress trial = getFirstBulbInScene();
      Serial.print("First Bulb Identified: ");
      Serial.println(trial.mac);
      
      //int newState = getBulbState(trial); 
      int newState = getBulbStateReliable(GET_LIGHT_STATE, getBulbAddress(trial).address); 
      if (newState == STATE_BULB_UNAVAILABLE) {
        Serial.println("No bulb state available");
      } else {
        setStateOnBulbs(!newState);
      }
    } else {
      Serial.println("No available sample bulb");
    }
  }
  
  actionState = (actionState + 1) % stateLengths;
}


void loop() {
  delay(100);
  Blink.loop();
  Buttons.loop();
  
  //See if there are any messages and clear the queue if any stale messages
  //are pending
  udpReceive(noOp);

  switch(Buttons.current_event) {
    case EVENT_BUTTON_ONE:
      Serial.println("Button One");
      trigger();
      break;
    case EVENT_BUTTON_ONE_TWO_LONG:
      Serial.println("Discovery Scan Triggered");
      ipRangeScan();
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Buttons.setup();
  
  Blink.setup();
  delay(300);

  if (!LittleFS.begin()) {
    Serial.println("Failed to mount file system config");
  }

  initWIFI();  
  
  startListening();
  
  initStateDoc();
  
  Blink.ledOFF();
}
