//********************************************************************************
// Web Interface init
//********************************************************************************
void WebServerInit()
{
  WebServer.on("/", handle_root);
  WebServer.on("/edit", handle_edit);
  WebServer.on("/tools", handle_tools);
  WebServer.on("/filelist", handle_filelist);
  WebServer.on("/upload", HTTP_GET, handle_upload);
  WebServer.on("/upload", HTTP_POST, handle_upload_post, handleFileUpload);
  WebServer.onNotFound(handleNotFound);
  #if defined(ESP8266)
    httpUpdater.setup(&WebServer);
  #endif
  
  WebServer.begin();
}


//********************************************************************************
// Web page header
//********************************************************************************
void addHeader(boolean showMenu, String & reply) {
  reply += F("<meta name=\"viewport\" content=\"width=width=device-width, initial-scale=1\">");
  reply += F("<STYLE>* {font-family:sans-serif; font-size:12pt;}");
  reply += F("h1 {font-size: 16pt; color: #07D; margin: 8px 0; font-weight: bold;}");
  reply += F(".button {margin:4px; padding:5px 15px; background-color:#07D; color:#FFF; text-decoration:none; border-radius:4px}");
  reply += F(".button-link {padding:5px 15px; background-color:#07D; color:#FFF; border:solid 1px #FFF; text-decoration:none}");
  reply += F(".button-widelink {display: inline-block; width: 100%; text-align: center; padding:5px 15px; background-color:#07D; color:#FFF; border:solid 1px #FFF; text-decoration:none}");
  reply += F(".button-nodelink {display: inline-block; width: 100%; text-align: center; padding:5px 15px; background-color:#888; color:#FFF; border:solid 1px #FFF; text-decoration:none}");
  reply += F(".button-nodelinkA {display: inline-block; width: 100%; text-align: center; padding:5px 15px; background-color:#28C; color:#FFF; border:solid 1px #FFF; text-decoration:none}");
  reply += F("td {padding:7px;}");
  reply += F("</STYLE>");
  reply += F("<h1>");
  reply += Settings.Name;
  reply += F("</h1>");
  reply += F("<a class=\"button-link\" href=\"/\">Main</a>");
  reply += F("<a class=\"button-link\" href=\"/edit?file=");
  reply += F(FILE_BOOT);
  reply += F("\">Boot</a>");
  reply += F("<a class=\"button-link\" href=\"/edit?file=");
  reply += F(FILE_RULES);
  reply += F("\">Rules</a>");
  reply += F("<a class=\"button-link\" href=\"/tools\">Tools</a>");
  reply += F("<BR><BR>");
}


//********************************************************************************
// Web Interface root page
//********************************************************************************
byte sortedIndex[UNIT_MAX + 1];
void handle_root() {

  String sCommand = WebServer.arg(F("cmd"));

  if (strcasecmp_P(sCommand.c_str(), PSTR("reboot")) != 0)
  {
    String reply = "";
    addHeader(true, reply);

    if (sCommand.length() > 0)
      ExecuteCommand(sCommand.c_str());

    String event = F("Web#Print");
    rulesProcessing(FILE_RULES, event);
    
    reply += printWebString;
    reply += F("<form><table>");

    // first get the list in alphabetic order
    for (byte x = 0; x < UNIT_MAX; x++)
      sortedIndex[x] = x;
    sortDeviceArray();
    
    for (byte x = 0; x < UNIT_MAX; x++)
    {
      byte index = sortedIndex[x];
      if (Nodes[index].IP[0] != 0)
      {
        String buttonclass ="";
        if ((String)Settings.Name == Nodes[index].nodeName)
          buttonclass = F("button-nodelinkA");
        else
          buttonclass = F("button-nodelink");
        reply += F("<TR><TD><a class=\"");
        reply += buttonclass;
        reply += F("\" ");
        char url[40];
        sprintf_P(url, PSTR("href='http://%u.%u.%u.%u'"), Nodes[index].IP[0], Nodes[index].IP[1], Nodes[index].IP[2], Nodes[index].IP[3]);
        reply += url;
        reply += ">";
        reply += Nodes[index].nodeName;
        reply += F("</a>");
        reply += F("<TD>");
      }
    }

    reply += "</table></form>";
    WebServer.send(200, "text/html", reply);
  }
  else
  {
    // have to disconnect or reboot from within the main loop
    // because the webconnection is still active at this point
    // disconnect here could result into a crash/reboot...
    if (strcasecmp_P(sCommand.c_str(), PSTR("reboot")) == 0)
    {
      cmd_within_mainloop = CMD_REBOOT;
    }
    WebServer.send(200, "text/html", "OK");
  }
}


//********************************************************************************
// Web Tools page
//********************************************************************************
void handle_tools() {

  String webrequest = WebServer.arg("cmd");

  String reply = "";
  addHeader(true, reply);

  reply += F("<form><table><TH>Tools<TH>");
  reply += F("<TR><TD><a class=\"button-widelink\" href=\"/filelist\">Files</a><TD>File System");
  #if defined(ESP8266)
    reply += F("<TR><TD><a class=\"button-widelink\" href=\"/update\">Firmware</a><TD>Update Firmware");
  #endif
  reply += F("<TR><TD><a class=\"button-widelink\" href=\"/?cmd=reboot\">Reboot</a><TD>Reboot System");

  #if defined(ESP8266)
    reply += F("<TR><TR><TD>FS:<TD>");
    reply += ESP.getFlashChipRealSize() / 1024;
    reply += F(" kB");

    reply += F(" (US:");
    reply += ESP.getFreeSketchSpace() / 1024;
    reply += F(" kB)");
  #endif

  reply += F("<TR><TD>Mem:<TD>");
  reply += ESP.getFreeHeap();

  reply += F("<TR><TD>Time:<TD>");
  reply += getTimeString(':');

  reply += F("<TR><TD>Uptime:<TD>");
  reply += uptime;

  reply += F("</table></form>");
  WebServer.send(200, "text/html", reply);
}


//********************************************************************************
// Web Interface handle other requests
//********************************************************************************
void handleNotFound() {

  if (loadFromFS(true, WebServer.uri())) return;
  #ifdef FEATURE_SD
    if (loadFromFS(false, WebServer.uri())) return;
  #endif  
  String message = F("URI: ");
  message += WebServer.uri();
  message += "\nMethod: ";
  message += (WebServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += WebServer.args();
  message += "\n";
  for (uint8_t i = 0; i < WebServer.args(); i++) {
    message += " NAME:" + WebServer.argName(i) + "\n VALUE:" + WebServer.arg(i) + "\n";
  }
  WebServer.send(404, "text/plain", message);
}

//********************************************************************************
// Web Interface server web file from SPIFFS
//********************************************************************************
bool loadFromFS(boolean spiffs, String path) {

  String dataType = F("text/plain");
  if (path.endsWith("/")) path += F("index.htm");

  if (path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if (path.endsWith(".htm")) dataType = F("text/html");
  else if (path.endsWith(".css")) dataType = F("text/css");
  else if (path.endsWith(".js")) dataType = F("application/javascript");
  else if (path.endsWith(".png")) dataType = F("image/png");
  else if (path.endsWith(".gif")) dataType = F("image/gif");
  else if (path.endsWith(".jpg")) dataType = F("image/jpeg");
  else if (path.endsWith(".ico")) dataType = F("image/x-icon");
  else if (path.endsWith(".txt")) dataType = F("application/octet-stream");
  else if (path.endsWith(".dat")) dataType = F("application/octet-stream");
  else if (path.endsWith(".esp")) return handle_custom(path);

  path = path.substring(1);
  if (spiffs)
  {
    fs::File dataFile = SPIFFS.open(path.c_str(), "r");
    if (!dataFile)
      return false;

    //prevent reloading stuff on every click
    WebServer.sendHeader("Cache-Control","max-age=3600, public");
    WebServer.sendHeader("Vary","*");
    WebServer.sendHeader("ETag","\"2.0.0\"");

    if (path.endsWith(".dat"))
      WebServer.sendHeader("Content-Disposition", "attachment;");
    WebServer.streamFile(dataFile, dataType);
    dataFile.close();
  }
  else
  {
#ifdef FEATURE_SD
    File dataFile = SD.open(path.c_str());
    if (!dataFile)
      return false;
    if (path.endsWith(".DAT"))
      WebServer.sendHeader("Content-Disposition", "attachment;");
    WebServer.streamFile(dataFile, dataType);
    dataFile.close();
#endif
  }
  return true;
}

//********************************************************************************
// Web Interface custom page handler
//********************************************************************************
boolean handle_custom(String path) {

  path = path.substring(1);
  String reply = "";

  // handle commands from a custom page
  String webrequest = WebServer.arg(F("cmd"));
  if (webrequest.length() > 0 ){
    ExecuteCommand(webrequest.c_str());
  }

  // create a dynamic custom page
  fs::File dataFile = SPIFFS.open(path.c_str(), "r");
  if (dataFile)
  {
    String page = "";
    page.reserve(dataFile.size());
    while (dataFile.available())
      page += ((char)dataFile.read());

    reply += parseTemplate(page,0);
    dataFile.close();
  }
  else
      return false; // unknown file that does not exist...

  WebServer.send(200, "text/html", reply);
  return true;
}

//********************************************************************************
// Device Sort routine, switch array entries
//********************************************************************************
void switchArray(byte value)
{
  byte temp;
  temp = sortedIndex[value - 1];
  sortedIndex[value - 1] = sortedIndex[value];
  sortedIndex[value] = temp;
}


//********************************************************************************
// Device Sort routine, compare two array entries
//********************************************************************************
boolean arrayLessThan(const String& ptr_1, const String& ptr_2)
{
  unsigned int i = 0;
  while (i < ptr_1.length())    // For each character in string 1, starting with the first:
  {
    if (ptr_2.length() < i)    // If string 2 is shorter, then switch them
    {
      return true;
    }
    else
    {
      const char check1 = (char)ptr_1[i];  // get the same char from string 1 and string 2
      const char check2 = (char)ptr_2[i];
      if (check1 == check2) {
        // they're equal so far; check the next char !!
        i++;
      } else {
        return (check2 > check1);
      }
    }
  }
  return false;
}


//********************************************************************************
// Device Sort routine, actual sorting
//********************************************************************************
void sortDeviceArray()
{
  int innerLoop ;
  int mainLoop ;
  for ( mainLoop = 1; mainLoop < UNIT_MAX; mainLoop++)
  {
    innerLoop = mainLoop;
    while (innerLoop  >= 1)
    {
      String one = Nodes[sortedIndex[innerLoop]].nodeName;
      String two = Nodes[sortedIndex[innerLoop-1]].nodeName;
      if (arrayLessThan(one,two))
      {
        switchArray(innerLoop);
      }
      innerLoop--;
    }
  }
}


//********************************************************************************
// Web Interface file list
//********************************************************************************
void handle_filelist() {

#if defined(ESP8266)

  String fdelete = WebServer.arg("delete");

  if (fdelete.length() > 0)
  {
    SPIFFS.remove(fdelete);
  }

  String reply = "";
  addHeader(true, reply);
  reply += F("<table><TH><TH>Filename:<TH>Size");

  Dir dir = SPIFFS.openDir("");
  while (dir.next())
  {
    reply += F("<TR><TD>");
    reply += F("<a class=\"button-link\" href=\"edit?file=");
    reply += dir.fileName();
    reply += F("\">Edit</a>");



    reply += F("<TD><a href=\"");
    reply += dir.fileName();
    reply += F("\">");
    reply += dir.fileName();
    reply += F("</a>");
    File f = dir.openFile("r");
    reply += F("<TD>");
    reply += f.size();
    reply += F("<TD>");
    if (dir.fileName() != FILE_BOOT)
    {
      reply += F("<a class=\"button-link\" href=\"filelist?delete=");
      reply += dir.fileName();
      reply += F("\">Del</a>");
    }

  }
  reply += F("<TR><TD><a class=\"button-link\" href=\"/upload\">Upload</a>");

  reply += F("</table></form>");
  WebServer.send(200, "text/html", reply);
#endif

#if defined(ESP32)
  String fdelete = WebServer.arg(F("delete"));

  if (fdelete.length() > 0)
  {
    SPIFFS.remove(fdelete);
  }

  String reply = "";
  addHeader(true, reply);
  reply += F("<table border=1px frame='box' rules='all'><TH><TH>Filename:<TH>Size");

  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file)
  {
    if(!file.isDirectory()){
      reply += F("<TR><TD>");
      if (file.name() != FILE_BOOT)
      {
        reply += F("<a class='button link' href=\"filelist?delete=");
        reply += file.name();
        reply += F("\">Del</a>");
      }

      reply += F("<TD><a href=\"");
      reply += file.name();
      reply += F("\">");
      reply += file.name();
      reply += F("</a>");
      reply += F("<TD>");
      reply += file.size();
      file = root.openNextFile();
    }
  }
  reply += F("</table></form>");
  reply += F("<BR><a class='button link' href=\"/upload\">Upload</a>");
  WebServer.send(200, "text/html", reply);
#endif
}


//********************************************************************************
// Web Interface upload page
//********************************************************************************
byte uploadResult = 0;
void handle_upload() {

  String reply = "";
  addHeader(true, reply);
  reply += F("<form enctype=\"multipart/form-data\" method=\"post\"><p>Upload file:<br><input type=\"file\" name=\"datafile\" size=\"40\"></p><div><input class=\"button-link\" type='submit' value='Upload'></div><input type='hidden' name='edit' value='1'></form>");
  WebServer.send(200, "text/html", reply);
}


//********************************************************************************
// Web Interface upload page
//********************************************************************************
void handle_upload_post() {

  String reply = "";
  if (uploadResult == 1)
  {
    reply += F("Upload OK!<BR>You may need to reboot to apply all settings...");
    LoadSettings();
  }

  if (uploadResult == 2)
    reply += F("<font color=\"red\">Upload file invalid!</font>");

  if (uploadResult == 3)
    reply += F("<font color=\"red\">No filename!</font>");

  addHeader(true, reply);
  reply += F("Upload finished");
  WebServer.send(200, "text/html", reply);
}


//********************************************************************************
// Web Interface upload handler
//********************************************************************************
File uploadFile;
void handleFileUpload() {
  static boolean valid = false;

  HTTPUpload& upload = WebServer.upload();

  if (upload.filename.c_str()[0] == 0)
  {
    uploadResult = 3;
    return;
  }

  if (upload.status == UPLOAD_FILE_START)
  {
    valid = false;
    uploadResult = 0;
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    // first data block, if this is the config file, check PID/Version
    if (upload.totalSize == 0)
    {
      if (strcasecmp(upload.filename.c_str(), "config.txt") == 0)
      {
        struct TempStruct {
          unsigned long PID;
          int Version;
        } Temp;
        for (int x = 0; x < sizeof(struct TempStruct); x++)
        {
          byte b = upload.buf[x];
          memcpy((byte*)&Temp + x, &b, 1);
        }
        if (Temp.Version == VERSION && Temp.PID == ESP_PROJECT_PID)
          valid = true;
      }
      else
      {
        // other files are always valid...
        valid = true;
      }
      if (valid)
      {
        // once we're safe, remove file and create empty one...
        SPIFFS.remove((char *)upload.filename.c_str());
        uploadFile = SPIFFS.open(upload.filename.c_str(), "w");
      }
    }
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (uploadFile) uploadFile.close();
  }

  if (valid)
    uploadResult = 1;
  else
    uploadResult = 2;
}


//********************************************************************************
// File editor
//********************************************************************************
void handle_edit() {

  String reply = "";
  addHeader(true, reply);

  String fileName = WebServer.arg(F("file"));
  String content = WebServer.arg(F("content"));

  if (WebServer.args() == 2) {
    if (content.length() > RULES_MAX_SIZE)
      reply += F("<span style=\"color:red\">Data was not saved, exceeds web editor limit!</span>");
    else
    {
      fs::File f = SPIFFS.open(fileName, "w");
      if (f)
      {
        f.print(content);
        f.close();
        telnetLog(F("DBG: Flash Save"));
      }
    }
  }

  int size = 0;
  fs::File f = SPIFFS.open(fileName, "r+");
  if (f)
  {
    size = f.size();
    if (size > RULES_MAX_SIZE)
      reply += F("<span style=\"color:red\">Filesize exceeds web editor limit!</span>");
    else
    {
      reply += F("<form method='post'>");
      reply += F("<textarea name='content' rows='15' cols='80' wrap='off'>");
      while (f.available())
      {
        String c((char)f.read());
        htmlEscape(c);
        reply += c;
      }
      reply += F("</textarea>");
    }
    f.close();
  }

  reply += F("<br>Current size: ");
  reply += size;
  reply += F(" characters (Max ");
  reply += RULES_MAX_SIZE;
  reply += F(")");

  reply += F("<br><input class=\"button-link\" type='submit' value='Submit'>");
  reply += F("</form>");
  WebServer.send(200, "text/html", reply);
}


void htmlEscape(String & html)
{
  html.replace("&",  F("&amp;"));
  html.replace("\"", F("&quot;"));
  html.replace("'",  F("&#039;"));
  html.replace("<",  F("&lt;"));
  html.replace(">",  F("&gt;"));
  html.replace("/", F("&#047;"));
}
