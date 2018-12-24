#if defined(ESP8266)

struct tcp_pcb;
extern struct tcp_pcb* tcp_tw_pcbs;
extern "C" void tcp_abort (struct tcp_pcb* pcb);

void tcpCleanup()
{

  while (tcp_tw_pcbs != NULL)
  {
    tcp_abort(tcp_tw_pcbs);
  }

}
#endif


void reboot() {
#if defined(ESP8266)
  ESP.reset();
#endif
#if defined(ESP32)
  ESP.restart();
#endif
}


String toString(float value, byte decimals)
{
  String sValue = String(value, decimals);
  sValue.trim();
  return sValue;
}

/********************************************************************************************\
  Get numerical Uservar by name
  \*********************************************************************************************/
float getNvar(String varName) {
  for (byte x = 0; x < USER_VAR_MAX; x++) {
    if (nUserVar[x].Name == varName) {
      return nUserVar[x].Value;
    }
  }
  return -99999;
}


/********************************************************************************************\
  Set numerical Uservar by name
  \*********************************************************************************************/
void setNvar(String varName, float value, int decimals) {
  int pos = -1;
  for (byte x = 0; x < USER_VAR_MAX; x++) {
    if (nUserVar[x].Name == varName) {
      nUserVar[x].Value = value;
      if (decimals != -1)
        nUserVar[x].Decimals = decimals;
      return;
    }
    if (pos == -1 && nUserVar[x].Name.length() == 0)
      pos = x;
  }
  if (pos != -1) {
    nUserVar[pos].Name = varName;
    nUserVar[pos].Value = value;
    if (decimals != -1)
      nUserVar[pos].Decimals = decimals;
    else
      nUserVar[pos].Decimals = 2;
  }
}


/********************************************************************************************\
  Set numerical Uservar Decimals by name
  \*********************************************************************************************/
void setNvarDecimals(String varName, byte decimals) {

  boolean found = false;
  for (byte x = 0; x < USER_VAR_MAX; x++) {
    if (nUserVar[x].Name == varName) {
      nUserVar[x].Decimals = decimals;
      found = true;
      break;
    }
  }
}


/********************************************************************************************\
  Get string Uservar by name
  \*********************************************************************************************/
String getSvar(String varName) {
  for (byte x = 0; x < USER_STRING_VAR_MAX; x++) {
    if (sUserVar[x].Name == varName) {
      return sUserVar[x].Value;
    }
  }
  return "";
}


/********************************************************************************************\
  Set string Uservar by name
  \*********************************************************************************************/
void setSvar(String varName, String value) {
  int pos = -1;
  for (byte x = 0; x < USER_STRING_VAR_MAX; x++) {
    if (sUserVar[x].Name == varName) {
      sUserVar[x].Value = value;
      return;
    }
    if (pos == -1 && sUserVar[x].Name.length() == 0)
      pos = x;
  }
  if (pos != -1) {
    sUserVar[pos].Name = varName;
    sUserVar[pos].Value = value;
  }
}


/********************************************************************************************\
  Set Timer by name
  \*********************************************************************************************/
void setTimer(String varName, unsigned int value) {

  int pos = -1;
  for (byte x = 0; x < RULES_TIMER_MAX; x++) {
    if (RulesTimer[x].Name == varName) {
      RulesTimer[x].Value = value;
      return;
    }
    if (pos == -1 && RulesTimer[x].Name.length() == 0)
      pos = x;
  }
  if (pos != -1) {
    RulesTimer[pos].Name = varName;
    RulesTimer[pos].Value = value;
  }
}


/********************************************************************************************\
  Convert a char string to integer
  \*********************************************************************************************/
unsigned long str2int(char *string)
{
  unsigned long temp = atof(string);
  return temp;
}

/********************************************************************************************\
  Classic Node list update
  \*********************************************************************************************/
/*
  #define NODE_TYPE_ID 1
  void sendSysInfoUDP(byte repeats)
  {
  // send my info to the world...
  for (byte counter = 0; counter < repeats; counter++)
  {
    uint8_t mac[] = {0, 0, 0, 0, 0, 0};
    uint8_t* macread = WiFi.macAddress(mac);
    byte data[80];
    data[0] = 255;
    data[1] = 1;
    for (byte x = 0; x < 6; x++)
      data[x + 2] = macread[x];
    IPAddress ip = WiFi.localIP();
    for (byte x = 0; x < 4; x++)
      data[x + 8] = ip[x];
    data[12] = Settings.Unit;
    data[13] = Settings.Build & 0xff;
    data[14] = Settings.Build >> 8;
    memcpy((byte*)data + 15, Settings.Name, 25);
    data[40] = NODE_TYPE_ID;

    IPAddress broadcastIP(255, 255, 255, 255);
    portUDP.beginPacket(broadcastIP, Settings.UDPPort);
    portUDP.write(data, 80);
    portUDP.endPacket();
    if (counter < (repeats - 1))
      delay(500);
  }
  }
*/

//********************************************************************************************
// Maintain node list
//********************************************************************************************
void nodelist(IPAddress remoteIP, String msg) {

  boolean found = false;
  for (byte x = 0; x < UNIT_MAX; x++) {
    if (Nodes[x].nodeName == msg) {
      for (byte y = 0; y < 4; y++)
        Nodes[x].IP[y] = remoteIP[y];
      Nodes[x].age = 0;
      found = true;
      break;
    }
  }
  if (!found) {
    for (byte x = 0; x < UNIT_MAX; x++) {
      if (Nodes[x].IP[0] == 0) {
        Nodes[x].nodeName = msg;
        for (byte y = 0; y < 4; y++)
          Nodes[x].IP[y] = remoteIP[y];
        Nodes[x].age = 0;
        break;
      }
    }
  }
}


/*********************************************************************************************\
   Refresh aging for remote units, drop if too old...
  \*********************************************************************************************/
void refreshNodeList()
{
  // start at 1, 0 = myself and does not age...
  for (byte counter = 1; counter < UNIT_MAX; counter++)
  {
    if (Nodes[counter].IP[0] != 0)
    {
      Nodes[counter].age++;  // increment age counter
      if (Nodes[counter].age > 10) // if entry to old, clear this node ip from the list.
        for (byte x = 0; x < 4; x++)
          Nodes[counter].IP[x] = 0;
    }
  }
}


/********************************************************************************************\
  Load settings from SPIFFS
  \*********************************************************************************************/
boolean LoadSettings()
{
  //LoadFromFile((char*)FILE_CONFIG, 0, (byte*)&Settings, sizeof(struct SettingsStruct));
  LoadFromFile((char*)FILE_SECURITY, 0, (byte*)&SecuritySettings, sizeof(struct SecurityStruct));
}


/********************************************************************************************\
  Load data from config file on SPIFFS
  \*********************************************************************************************/
void LoadFromFile(char* fname, int index, byte * memAddress, int datasize)
{
  fs::File f = SPIFFS.open(fname, "r+");
  if (f)
  {
    f.seek(index, fs::SeekSet);
    f.read(memAddress, datasize);
    f.close();
  }
}


/********************************************************************************************\
  Save settings to SPIFFS
  \*********************************************************************************************/
boolean SaveSettings(void)
{
  //SaveToFile((char*)FILE_CONFIG, 0, (byte*)&Settings, sizeof(struct SettingsStruct));
  return SaveToFile((char*)FILE_SECURITY, 0, (byte*)&SecuritySettings, sizeof(struct SecurityStruct));
}


/********************************************************************************************\
  Save data into config file on SPIFFS
  \*********************************************************************************************/
boolean SaveToFile(char* fname, int index, byte * memAddress, int datasize)
{
  boolean success = false;

  fs::File f = SPIFFS.open(fname, "r+");
  if (f)
  {
    f.seek(index, fs::SeekSet);
    byte *pointerToByteToSave = memAddress;
    for (int x = 0; x < datasize ; x++)
    {
      f.write(*pointerToByteToSave);
      pointerToByteToSave++;
    }
    f.close();
    success = true;
  }
  telnetLog(F("DBG: Flash Save"));
  return success;
}



/*********************************************************************************************\
  Parse a string and get the xth command or parameter
  \*********************************************************************************************/
String parseString(String& string, byte indexFind, char separator)
{
  String tmpString = string;
  tmpString += separator;
  tmpString.replace(" ", (String)separator);
  String locateString = "";
  byte count = 0;
  int index = tmpString.indexOf(separator);
  while (index > 0)
  {
    count++;
    locateString = tmpString.substring(0, index);
    tmpString = tmpString.substring(index + 1);
    index = tmpString.indexOf(separator);
    if (count == indexFind)
    {
      return locateString;
    }
  }
  return "";
}

//*********************************************************************************************
// Parse a string and get the xth command or parameter
//*********************************************************************************************
int getParamStartPos(String& string, byte indexFind)
{
  String tmpString = string;
  byte count = 0;
  tmpString.replace(" ", ",");
  for (int x = 0; x < tmpString.length(); x++)
  {
    if (tmpString.charAt(x) == ',')
    {
      count++;
      if (count == (indexFind - 1))
        return x + 1;
    }
  }
  return -1;
}

//********************************************************************************************
// Convert a char string to IP byte array
//********************************************************************************************
boolean str2ip(char *string, byte * IP)
{
  byte c;
  byte part = 0;
  int value = 0;

  for (int x = 0; x <= strlen(string); x++)
  {
    c = string[x];
    if (isdigit(c))
    {
      value *= 10;
      value += c - '0';
    }

    else if (c == '.' || c == 0) // next octet from IP address
    {
      if (value <= 255)
        IP[part++] = value;
      else
        return false;
      value = 0;
    }
    else if (c == ' ') // ignore these
      ;
    else // invalid token
      return false;
  }
  if (part == 4) // correct number of octets
    return true;
  return false;
}

#ifdef FEATURE_ARDUINO_OTA
/********************************************************************************************\
  Allow updating via the Arduino OTA-protocol. (this allows you to upload directly from platformio)
  \*********************************************************************************************/
void ArduinoOTAInit()
{
  ArduinoOTA.setPort(ARDUINO_OTA_PORT);
  ArduinoOTA.setHostname(Settings.Name);

  ArduinoOTA.onStart([]() {
    Serial.println(F("OTA  : Start upload"));
    SPIFFS.end(); //important, otherwise it fails
  });

  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nOTA  : End"));
    //"dangerous": if you reset during flash you have to reflash via serial
    //so dont touch device until restart is complete
    Serial.println(F("\nOTA  : DO NOT RESET OR POWER OFF UNTIL BOOT+FLASH IS COMPLETE."));
    delay(100);
    reboot();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {

    Serial.printf("OTA  : Progress %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.print(F("\nOTA  : Error (will reboot): "));
    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));

    delay(100);
    reboot();
  });
  ArduinoOTA.begin();
}
#endif
