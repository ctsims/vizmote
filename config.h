
void loadJSON(const char* configFilePath, void (*pFunc)(DynamicJsonDocument stateDoc)) {
  DynamicJsonDocument stateDoc(4096);
  
  File configFile = LittleFS.open(configFilePath, "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return;
  }

  size_t size = configFile.size();
  if (size > 4 * 1024) {
    Serial.println("Config file size is too large");
    return;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);

  auto error = deserializeJson(stateDoc, buf.get());
  if (error) {
    Serial.println("Failed to parse config file");
    Serial.println(error.f_str());
    return;
  }
  configFile.close();
  pFunc(stateDoc);
}

bool saveJSON(const char* configFilePath, void (*pFunc)(DynamicJsonDocument* stateDoc)) {
  DynamicJsonDocument doc(4096);

  pFunc(&doc);
  
  File configFile = LittleFS.open(configFilePath, "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }
  
  String test;
  serializeJson(doc, test);
  Serial.println("write");
  Serial.println(test.c_str());

  configFile.write(test.c_str());
  
  configFile.close();
  return true;
}
