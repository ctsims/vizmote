struct wizbulb {
  char mac[13];
  char address[16];
};

struct wizaddress {
  char mac[13];
};


static const char REGISTER_MESSAGE[] = R"EOF({"method":"registration","params":{"phoneMac":"AAAAAAAAAAAA","register":false,"phoneIp":"1.2.3.4","id":"1"}})EOF";

static const char GET_LIGHT_STATE[] = R"EOF({"method":"getPilot","params":{}})EOF";
static const char SET_OFF[] = R"EOF({"method":"setPilot","params":{"state":false}})EOF";
static const char SET_ON[] = R"EOF({"method":"setPilot","params":{"state":true}})EOF";

List<wizbulb> universe;

List<wizaddress> sceneBulbs;

List<wizaddress> sceneTwoBulbs;

void registerFoundBulb(const char* mac, const char* address) {
  int bulbMatchingMac = -1;
  int bulbMatchingAddress = -1;
  for (int i = 0; i < universe.getSize(); ++i) {
    wizbulb bulb = universe[i];
    if (strcmp(mac, bulb.mac) == 0) {
      bulbMatchingMac = i;
      strlcpy(bulb.address, address, sizeof(bulb.address));
    } else if (strcmp(address, bulb.address) == 0) {
      bulbMatchingAddress = i;
    }
  }
  if (bulbMatchingMac == -1) {
    wizbulb newBulb;

    strlcpy(newBulb.mac, mac, sizeof(newBulb.mac));
    strlcpy(newBulb.address, address, sizeof(newBulb.address));
    
    Serial.print("Test Mac: ");
    Serial.println(newBulb.mac);
    Serial.print("Test Address: ");
    Serial.println(newBulb.address);
    
    universe.add(newBulb);
  }
  if (bulbMatchingAddress != -1 && (bulbMatchingMac != bulbMatchingAddress)) {
    universe.remove(bulbMatchingAddress);
  }
}

const wizbulb getBulbAddress(wizaddress wa){
  for (int i = 0; i < universe.getSize(); ++i) {
    wizbulb bulb = universe[i];
    if (strcmp(wa.mac, bulb.mac) == 0) {
      return bulb;
    }
  }
}

boolean isScenePopulated(List<wizaddress> &scene) {
  return scene.getSize() > 0;
}

const wizaddress getFirstBulbInScene(List<wizaddress> &scene) {
  return scene[0];
}

boolean isBulbInScene(const char* mac, List<wizaddress> &scene) {
  for (int i = 0; i < scene.getSize(); ++i) {
    wizaddress wa = scene[i];
    char* waMac = wa.mac;
    if (strcmp(mac, wa.mac) == 0) {
      return true;
    }
  }
  
  return false;
}

boolean addBulbToScene(const char* mac, List<wizaddress> &scene) {
  if (!isBulbInScene(mac,scene)) {
    wizaddress wa;
    strlcpy(wa.mac, mac, sizeof(wa.mac));
    
    scene.add(wa);
    return true;
  }
  return false;
}

void clearSceneBulbs() {
  sceneBulbs.clear();
}

void printState() {
  Serial.println("universe");
  for (int i = 0; i < universe.getSize(); ++i) {
    Serial.print("Bulb: ");
    wizbulb bulb = universe[i];
    Serial.print("Mac[");
    Serial.print(bulb.mac);
    Serial.print("] ip: ");
    Serial.println(bulb.address);
  }
  Serial.println("scene:");
  for (int i = 0; i < sceneBulbs.getSize(); ++i) {
    Serial.println(sceneBulbs[i].mac);
  }
}
