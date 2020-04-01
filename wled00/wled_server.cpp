#include "wled.h"

/*
 * Integrated HTTP web server page declarations
 */

//Is this an IP?
bool isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

bool captivePortal(AsyncWebServerRequest *request)
{
  if (ON_STA_FILTER(request)) return false; //only serve captive in AP mode
  String hostH;
  if (!request->hasHeader("Host")) return false;
  hostH = request->getHeader("Host")->value();
  
  if (!isIp(hostH) && hostH.indexOf("wled.me") < 0 && hostH.indexOf(cmDNS) < 0) {
    DEBUG_PRINTLN("Captive portal");
    AsyncWebServerResponse *response = request->beginResponse(302);
    response->addHeader("Location", "http://4.3.2.1");
    request->send(response);
    return true;
  }
  return false;
}

void initServer()
{
  //CORS compatiblity
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

  server.on("/liveview", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", PAGE_liveview);
  });
  
  //settings page
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){
    serveSettings(request);
  });
  
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!handleFileRead(request, "/favicon.ico"))
    {
      request->send_P(200, "image/x-icon", favicon, 156);
    }
  });
  
  server.on("/sliders", HTTP_GET, [](AsyncWebServerRequest *request){
    serveIndex(request);
  });
  
  server.on("/welcome", HTTP_GET, [](AsyncWebServerRequest *request){
    serveSettings(request);
  });
  
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    serveMessage(request, 200,"Rebooting now...",F("Please wait ~10 seconds..."),129);
    doReboot = true;
  });
  
  server.on("/settings/wifi", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!(wifiLock && otaLock)) handleSettingsSet(request, 1);
    serveMessage(request, 200,F("WiFi settings saved."),F("Please connect to the new IP (if changed)"),129);
    forceReconnect = true;
  });

  server.on("/settings/leds", HTTP_POST, [](AsyncWebServerRequest *request){
    handleSettingsSet(request, 2);
    serveMessage(request, 200,F("LED settings saved."),"Redirecting...",1);
  });

  server.on("/settings/ui", HTTP_POST, [](AsyncWebServerRequest *request){
    handleSettingsSet(request, 3);
    serveMessage(request, 200,F("UI settings saved."),"Redirecting...",1);
  });

  server.on("/settings/dmx", HTTP_POST, [](AsyncWebServerRequest *request){
    handleSettingsSet(request, 7);
    serveMessage(request, 200,F("UI settings saved."),"Redirecting...",1);
  });

  server.on("/settings/sync", HTTP_POST, [](AsyncWebServerRequest *request){
    handleSettingsSet(request, 4);
    serveMessage(request, 200,F("Sync settings saved."),"Redirecting...",1);
  });

  server.on("/settings/time", HTTP_POST, [](AsyncWebServerRequest *request){
    handleSettingsSet(request, 5);
    serveMessage(request, 200,F("Time settings saved."),"Redirecting...",1);
  });

  server.on("/settings/sec", HTTP_POST, [](AsyncWebServerRequest *request){
    handleSettingsSet(request, 6);
    if (!doReboot) serveMessage(request, 200,F("Security settings saved."),F("Rebooting, please wait ~10 seconds..."),129);
    doReboot = true;
  });

  server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request){
    serveJson(request);
  });

  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/json", [](AsyncWebServerRequest *request) {
    bool verboseResponse = false;
    { //scope JsonDocument so it releases its buffer
      DynamicJsonDocument jsonBuffer(8192);
      DeserializationError error = deserializeJson(jsonBuffer, (uint8_t*)(request->_tempObject));
      JsonObject root = jsonBuffer.as<JsonObject>();
      if (error || root.isNull()) {
        request->send(400, "application/json", "{\"error\":10}"); return;
      }
      verboseResponse = deserializeState(root);
    }
    if (verboseResponse) { //if JSON contains "v"
      serveJson(request); return; 
    } 
    request->send(200, "application/json", "{\"success\":true}");
  });
  server.addHandler(handler);

  //*******DEPRECATED*******
  server.on("/version", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", (String)VERSION);
    });
    
  server.on("/uptime", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", (String)millis());
    });
    
  server.on("/freeheap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", (String)ESP.getFreeHeap());
    });
  //*******END*******/
  
  server.on("/u", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", PAGE_usermod);
    });
    
  server.on("/url", HTTP_GET, [](AsyncWebServerRequest *request){
    URL_response(request);
    });
    
  server.on("/teapot", HTTP_GET, [](AsyncWebServerRequest *request){
    serveMessage(request, 418, F("418. I'm a teapot."), F("(Tangible Embedded Advanced Project Of Twinkling)"), 254);
    });
    
  //if OTA is allowed
  if (!otaLock){
    #if !defined WLED_DISABLE_FILESYSTEM && defined WLED_ENABLE_FS_EDITOR
     #ifdef ARDUINO_ARCH_ESP32
      server.addHandler(new SPIFFSEditor(SPIFFS));//http_username,http_password));
     #else
      server.addHandler(new SPIFFSEditor());//http_username,http_password));
     #endif
    #else
    server.on("/edit", HTTP_GET, [](AsyncWebServerRequest *request){
      serveMessage(request, 501, "Not implemented", F("The SPIFFS editor is disabled in this build."), 254);
    });
    #endif
    //init ota page
    #ifndef WLED_DISABLE_OTA
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", PAGE_update);
    });
    
    server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
      if (Update.hasError())
      {
        serveMessage(request, 500, F("Failed updating firmware!"), F("Please check your file and retry!"), 254); return;
      }
      serveMessage(request, 200, F("Successfully updated firmware!"), F("Please wait while the module reboots..."), 131); 
      doReboot = true;
    },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
      if(!index){
        DEBUG_PRINTLN("OTA Update Start");
        #ifdef ESP8266
        Update.runAsync(true);
        #endif
        Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000);
      }
      if(!Update.hasError()) Update.write(data, len);
      if(final){
        if(Update.end(true)){
          DEBUG_PRINTLN("Update Success");
        } else {
          DEBUG_PRINTLN("Update Failed");
        }
      }
    });
    
    #else
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
      serveMessage(request, 501, "Not implemented", F("OTA updates are disabled in this build."), 254);
    });
    #endif
  } else
  {
    server.on("/edit", HTTP_GET, [](AsyncWebServerRequest *request){
      serveMessage(request, 500, "Access Denied", "Please unlock OTA in security settings!", 254);
    });
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
      serveMessage(request, 500, "Access Denied", "Please unlock OTA in security settings!", 254);
    });
  }


    #ifdef WLED_ENABLE_DMXOUT
    server.on("/dmxmap", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", PAGE_dmxmap     , dmxProcessor);
    });
    #else
    server.on("/dmxmap", HTTP_GET, [](AsyncWebServerRequest *request){
      serveMessage(request, 501, "Not implemented", F("DMX support is not enabled in this build."), 254);
    });
    #endif
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if (captivePortal(request)) return;
    serveIndexOrWelcome(request);
  });
  
  //called when the url is not defined here, ajax-in; get-settings
  server.onNotFound([](AsyncWebServerRequest *request){
    DEBUG_PRINTLN("Not-Found HTTP call:");
    DEBUG_PRINTLN("URI: " + request->url());
    if (captivePortal(request)) return;

    //make API CORS compatible
    if (request->method() == HTTP_OPTIONS)
    {
      request->send(200); return;
    }
    
    if(handleSet(request, request->url())) return;
    #ifndef WLED_DISABLE_ALEXA
    if(espalexa.handleAlexaApiCall(request)) return;
    #endif
    #ifdef WLED_ENABLE_FS_SERVING
    if(handleFileRead(request, request->url())) return;
    #endif
    request->send(404, "text/plain", "Not Found");
  });
}


void serveIndexOrWelcome(AsyncWebServerRequest *request)
{
  if (!showWelcomePage){
    serveIndex(request);
  } else {
    serveSettings(request);
  }
}


void serveIndex(AsyncWebServerRequest* request)
{
  #ifdef WLED_ENABLE_FS_SERVING
  if (handleFileRead(request, "/index.htm")) return;
  #endif

  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", PAGE_index, PAGE_index_L);

  response->addHeader("Content-Encoding","gzip");
  
  request->send(response);
}


String msgProcessor(const String& var)
{
  if (var == "MSG") {
    String messageBody = messageHead;
    messageBody += "</h2>";
    messageBody += messageSub;
    uint32_t optt = optionType;

    if (optt < 60) //redirect to settings after optionType seconds
    {
      messageBody += "<script>setTimeout(RS," + String(optt*1000) + ")</script>";
    } else if (optt < 120) //redirect back after optionType-60 seconds, unused
    {
      //messageBody += "<script>setTimeout(B," + String((optt-60)*1000) + ")</script>";
    } else if (optt < 180) //reload parent after optionType-120 seconds
    {
      messageBody += "<script>setTimeout(RP," + String((optt-120)*1000) + ")</script>";
    } else if (optt == 253)
    {
      messageBody += F("<br><br><form action=/settings><button class=\"bt\" type=submit>Back</button></form>"); //button to settings
    } else if (optt == 254)
    {
      messageBody += F("<br><br><button type=\"button\" class=\"bt\" onclick=\"B()\">Back</button>");
    }
    return messageBody;
  }
  return String();
}


void serveMessage(AsyncWebServerRequest* request, uint16_t code, String headl, String subl, byte optionT)
{
  messageHead = headl;
  messageSub = subl;
  optionType = optionT;
  
  request->send_P(code, "text/html", PAGE_msg, msgProcessor);
}


String settingsProcessor(const String& var)
{
  if (var == "CSS") {
    char buf[2048];
    getSettingsJS(optionType, buf);
    return String(buf);
  }
  
  #ifdef WLED_ENABLE_DMXOUT

  if (var == "DMXMENU") {
    return String(F("<form action=/settings/dmx><button type=submit>DMX Output</button></form>"));
  }
  
  #endif
  if (var == "SCSS") return String(FPSTR(PAGE_settingsCss));
  return String();
}

String dmxProcessor(const String& var)
{
  String mapJS;
  #ifdef WLED_ENABLE_DMXOUT
    if (var == "DMXVARS") {
      mapJS += "\nCN=" + String(DMXChannels) + ";\n";
      mapJS += "CS=" + String(DMXStart) + ";\n";
      mapJS += "CG=" + String(DMXGap) + ";\n";
      mapJS += "LC=" + String(ledCount) + ";\n";
      mapJS += "var CH=[";
      for (int i=0;i<15;i++) {
        mapJS += String(DMXFixtureMap[i]) + ",";
      }
      mapJS += "0];";
    }
  #endif
  
  return mapJS;
}


void serveSettings(AsyncWebServerRequest* request)
{
  byte subPage = 0;
  const String& url = request->url();
  if (url.indexOf("sett") >= 0) 
  {
    if      (url.indexOf("wifi") > 0) subPage = 1;
    else if (url.indexOf("leds") > 0) subPage = 2;
    else if (url.indexOf("ui")   > 0) subPage = 3;
    else if (url.indexOf("sync") > 0) subPage = 4;
    else if (url.indexOf("time") > 0) subPage = 5;
    else if (url.indexOf("sec")  > 0) subPage = 6;
    #ifdef WLED_ENABLE_DMXOUT // include only if DMX is enabled
    else if (url.indexOf("dmx")  > 0) subPage = 7;
    #endif
  } else subPage = 255; //welcome page

  if (subPage == 1 && wifiLock && otaLock)
  {
    serveMessage(request, 500, "Access Denied", "Please unlock OTA in security settings!", 254); return;
  }
  
  #ifdef WLED_DISABLE_MOBILE_UI //disable welcome page if not enough storage
   if (subPage == 255) {serveIndex(request); return;}
  #endif

  optionType = subPage;
  
  switch (subPage)
  {
    case 1:   request->send_P(200, "text/html", PAGE_settings_wifi, settingsProcessor); break;
    case 2:   request->send_P(200, "text/html", PAGE_settings_leds, settingsProcessor); break;
    case 3:   request->send_P(200, "text/html", PAGE_settings_ui  , settingsProcessor); break;
    case 4:   request->send_P(200, "text/html", PAGE_settings_sync, settingsProcessor); break;
    case 5:   request->send_P(200, "text/html", PAGE_settings_time, settingsProcessor); break;
    case 6:   request->send_P(200, "text/html", PAGE_settings_sec , settingsProcessor); break;
    case 7:   request->send_P(200, "text/html", PAGE_settings_dmx , settingsProcessor); break;
    case 255: request->send_P(200, "text/html", PAGE_welcome); break;
    default:  request->send_P(200, "text/html", PAGE_settings     , settingsProcessor); 
  }
}
