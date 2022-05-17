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
#include "periodic.h"

#define LIGHT_PORT 38899
#define RECEIVE_PORT 4210

#define MAX_WAIT 15000
#define MAX_INTERVAL 3000

#define STATE_BULB_UNAVAILABLE -1

#define FILE_LIGHT_CONFIG "/lights.json"

#define LED_PIN LED_BUILTIN

#define BUTTON_PIN 4
#define BUTTON_TWO_PIN 5
#define BUTTON_THREE_PIN 12

LedBlinker Blink(LED_PIN);
ButtonController Buttons(BUTTON_PIN, BUTTON_TWO_PIN, BUTTON_THREE_PIN);

char packet[255];

WiFiClient net = WiFiClient();
WiFiUDP UDP;

unsigned int chill = 250;

bool fetched;
StaticJsonDocument<255> responseDoc;

void parseStateConfigDoc(DynamicJsonDocument stateDoc) {
  JsonArray lights  = stateDoc["lights"];
  for (int i = 0 ; i < lights.size() ; ++i) {
    registerFoundBulb(lights[i]["mac"], lights[i]["address"]);
  }

  JsonArray scene  = stateDoc["scene"];
  for (int i = 0 ; i < scene.size() ; ++i) {
    addBulbToScene(scene[i], sceneBulbs);
  }

  JsonArray scene2  = stateDoc["scene2"];
  for (int i = 0 ; i < scene.size() ; ++i) {
    addBulbToScene(scene2[i], sceneTwoBulbs);
  }

  JsonArray actions  = stateDoc["actions"];
  sceneActions[0] = actions[0];
  sceneActions[1] = actions[1];
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
  JsonArray scene2  = doc->createNestedArray("scene2");

  for (int i = 0 ; i < sceneTwoBulbs.getSize() ; ++i) {
    wizaddress wa = sceneTwoBulbs[i];
    Serial.print("sm ");
    Serial.println(wa.mac);
    if (!scene2.add(wa.mac)) {
      Serial.println("Couldn't add scene. Doc full");
    }
  }

  JsonArray actions  = doc->createNestedArray("actions");

  actions.add(sceneActions[0]);
  actions.add(sceneActions[1]);
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

  saveStateDoc();
}

void saveStateDoc() {
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
  if (!UDP.endPacket()) {
    Serial.println("UDP Packet Send Failure");
  }
}

void registerPacketReceived(IPAddress address, char packet[]) {
  deserializeJson(responseDoc, packet);
  int scene = responseDoc["result"]["sceneId"];
  const char* mac = responseDoc["result"]["mac"];

  char addressRaw[16];
  sprintf(addressRaw, "%d.%d.%d.%d", address[0], address[1], address[2], address[3]);
  const char* addressPtr = addressRaw;

  registerFoundBulb(mac, addressPtr);

  //  if (scene == 19) {
  //    Serial.println("adding to scene");
  //    addBulbToScene(mac);
  //  }
  printState();
}


void updateScene(List<wizaddress> &scene) {
  Blink.ledON();
  unsigned int r_st = millis();

  Blink.blink();
  scene.clear();

  for (int i = 0 ; i < universe.getSize() ; ++i) {
    wizbulb b = universe[i];
    int sceneId = getBulbPropertyReliable(GET_LIGHT_STATE, b.address, "sceneId");
    if (sceneId == -1) {
      Serial.println("Couldn't communicate with bulb for scene update");
    }
    if (sceneId == 19) {
      Serial.println("adding to scene");
      addBulbToScene(b.mac, scene);
    }
    Blink.loop();
  }

  printState();

  saveStateDoc();
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


int getBulbPropertyReliable(const char* outboundMessage, const char* address, const char* property_id) {
  boolean timedOut = false;
  unsigned int t_st = millis();

  int next_interval = 750;
  unsigned int next_send = millis() - 1;
  while ((millis() - t_st) < MAX_WAIT) {
    Blink.loop();
    if (udpRawReceive()) {

      deserializeJson(responseDoc, packet);
      return responseDoc["result"][property_id];

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

void setStateOnBulbs(List<wizaddress> &scene, bool newState) {
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

void bulbFlip(List<wizaddress> &scene, int action) {
  if (isScenePopulated(scene)) {
    int newState = -1;

    if (action == WIZ_ACTION_CYCLE) {
      wizaddress trial = getFirstBulbInScene(scene);
      Serial.print("First Bulb Identified: ");
      Serial.println(trial.mac);
  
      newState = getBulbPropertyReliable(GET_LIGHT_STATE, getBulbAddress(trial).address, "state");
      if (newState == STATE_BULB_UNAVAILABLE) {
        Serial.println("No bulb state available");
        return;
      }
    } else if(action == WIZ_ACTION_ON) {
      newState = 0;
    } else if(action == WIZ_ACTION_OFF) {
      newState = 1;
    }
    
    setStateOnBulbs(scene, !newState);
  } else {
    Serial.println("No available sample bulb");
  }
}

void reportMem() {
  Serial.println(ESP.getFreeHeap(), DEC);
}

Periodic ReportMemory(reportMem, 30000);


#define REMOTE_STATE_BASELINE 0
#define REMOTE_STATE_PROGRAMMING 2

int remote_state = REMOTE_STATE_BASELINE;

int current_remote_button = -1;

void loop() {
  delay(100);
  Blink.loop();
  Buttons.loop();
  ReportMemory.loop();

  //See if there are any messages and clear the queue if any stale messages
  //are pending
  udpReceive(noOp);

  switch (remote_state) {
    case REMOTE_STATE_BASELINE:
      stateBaselineLoop();
      break;
    case REMOTE_STATE_PROGRAMMING:
      stateProgrammingLoop();
      break;
    default:
      break;
  }
}

void setProgrammingLedBlinkState() {
  int action = sceneActions[current_remote_button - 1];
  switch (action){
  case WIZ_ACTION_CYCLE:
    Blink.blink(BLINKER_PATTERN_ON_OFF);
    break;
  case WIZ_ACTION_ON:
    Blink.blink(BLINKER_PATTERN_MOSTLY_ON);
    break;
  case WIZ_ACTION_OFF:
    Blink.blink(BLINKER_PATTERN_MOSTLY_OFF);
    break;
  } 
}

void enterProgrammingMode(int button) {
  Serial.print("Enter Programming mode for button: ");
  Serial.println(button);
  remote_state = REMOTE_STATE_PROGRAMMING;
  current_remote_button = button;
  setProgrammingLedBlinkState();
}

void enterBaseline() {
  remote_state = REMOTE_STATE_BASELINE;
  current_remote_button = -1;
  Blink.ledOFF();
}

void stateBaselineLoop() {
    switch (Buttons.current_event) {
    case EVENT_BUTTON_ONE:
      Serial.println("Cycle Scene 1");
      bulbFlip(sceneBulbs, sceneActions[0]);
      break;
    case EVENT_BUTTON_ONE_LONG:
      Serial.println("Programming Button 1");
      enterProgrammingMode(1);
      break;
    case EVENT_BUTTON_TWO:
      Serial.println("Cycle Scene 2");
      bulbFlip(sceneTwoBulbs, sceneActions[1]);
      break;
    case EVENT_BUTTON_TWO_LONG:
      Serial.println("Programming Button 2");
      enterProgrammingMode(2);
      break;

    case EVENT_BUTTON_THREE_LONG:
      Serial.println("Programming Button 3");
      enterProgrammingMode(3);
      break;
    case EVENT_BUTTON_ONE_TWO_LONG:
      Serial.println("Discovery Scan Triggered");
      ipRangeScan();
      break;
    default:
      break;
  }
}

void cycleProgrammingButtonAction() {
  int current = sceneActions[current_remote_button -1];
  Serial.println("Current Scene ");
  Serial.println(current);
  sceneActions[current_remote_button -1] = (current + 1) % 3;
  Serial.println("New Scene ");
  Serial.println(sceneActions[current_remote_button -1]);
  setProgrammingLedBlinkState();
}

void stateProgrammingLoop() {
  int currentButtonLong = current_remote_button * 2;
  int currentButtonShort = currentButtonLong - 1;
  int event = Buttons.current_event;
  
  if (event == EVENT_IDLE) { 
    
  } else if(event == currentButtonShort) {
    Serial.println("Cycling button state");
    cycleProgrammingButtonAction();
  } else if(event == currentButtonLong) {
    Serial.println("Updating Scene Bulbs");
    if (current_remote_button == 1) { 
      updateScene(sceneBulbs);
    } else if (current_remote_button == 2) {
      updateScene(sceneTwoBulbs);
    }
    saveStateDoc();
  } else {
    Serial.print("Exiting Config for event ");
    Serial.println(event);
    saveStateDoc();
    enterBaseline();
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
