#include "wled.h"
#include <Arduino.h>

// Global Variable definitions
char versionString[] = "0.9.1n";

// AP and OTA default passwords (for maximum change them!)
char apPass[65] = DEFAULT_AP_PASS;
char otaPass[33] = DEFAULT_OTA_PASS;

// Hardware CONFIG (only changeble HERE, not at runtime)
// LED strip pin, button pin and IR pin changeable in NpbWrapper.h!

byte auxDefaultState = 0;                              // 0: input 1: high 2: low
byte auxTriggeredState = 0;                            // 0: input 1: high 2: low
char ntpServerName[33] = "0.wled.pool.ntp.org";        // NTP server to use

// WiFi CONFIG (all these can be changed via web UI, no need to set them here)
char clientSSID[33] = CLIENT_SSID;
char clientPass[65] = CLIENT_PASS;
char cmDNS[33] = "x";                              // mDNS address (placeholder, will be replaced by wledXXXXXXXXXXXX.local)
char apSSID[33] = "";                              // AP off by default (unless setup)
byte apChannel = 1;                                // 2.4GHz WiFi AP channel (1-13)
byte apHide = 0;                                   // hidden AP SSID
byte apBehavior = AP_BEHAVIOR_BOOT_NO_CONN;        // access point opens when no connection after boot by default
IPAddress staticIP(0, 0, 0, 0);                    // static IP of ESP
IPAddress staticGateway(0, 0, 0, 0);               // gateway (router) IP
IPAddress staticSubnet(255, 255, 255, 0);          // most common subnet in home networks
bool noWifiSleep = false;                          // disabling modem sleep modes will increase heat output and power usage, but may help with connection issues

// LED CONFIG
uint16_t ledCount = 30;          // overcurrent prevented by ABL
bool useRGBW = false;            // SK6812 strips can contain an extra White channel
bool turnOnAtBoot = true;        // turn on LEDs at power-up
byte bootPreset = 0;             // save preset to load after power-up

byte col[] { 255, 160, 0, 0 };        // current RGB(W) primary color. col[] should be updated if you want to change the color.
byte colSec[] { 0, 0, 0, 0 };         // current RGB(W) secondary color
byte briS = 128;                      // default brightness

byte nightlightTargetBri = 0;        // brightness after nightlight is over
byte nightlightDelayMins = 60;
bool nightlightFade = true;              // if enabled, light will gradually dim towards the target bri. Otherwise, it will instantly set after delay over
bool nightlightColorFade = false;        // if enabled, light will gradually fade color from primary to secondary color.
bool fadeTransition = true;              // enable crossfading color transition
uint16_t transitionDelay = 750;          // default crossfade duration in ms

bool skipFirstLed = false;        // ignore first LED in strip (useful if you need the LED as signal repeater)
byte briMultiplier = 100;         // % of brightness to set (to limit power, if you set it to 50 and set bri to 255, actual brightness will be 127)

// User Interface CONFIG
char serverDescription[33] = "WLED";        // Name of module
bool syncToggleReceive = false;             // UIs which only have a single button for sync should toggle send+receive if this is true, only send otherwise

// Sync CONFIG
bool buttonEnabled = true;
byte irEnabled = 0;        // Infrared receiver

uint16_t udpPort = 21324;           // WLED notifier default port
uint16_t udpRgbPort = 19446;        // Hyperion port

bool receiveNotificationBrightness = true;        // apply brightness from incoming notifications
bool receiveNotificationColor = true;             // apply color
bool receiveNotificationEffects = true;           // apply effects setup
bool notifyDirect = false;                        // send notification if change via UI or HTTP API
bool notifyButton = false;                        // send if updated by button or infrared remote
bool notifyAlexa = false;                         // send notification if updated via Alexa
bool notifyMacro = false;                         // send notification for macro
bool notifyHue = true;                            // send notification if Hue light changes
bool notifyTwice = false;                         // notifications use UDP: enable if devices don't sync reliably

bool alexaEnabled = true;                      // enable device discovery by Amazon Echo
char alexaInvocationName[33] = "Light";        // speech control name of device. Choose something voice-to-text can understand

char blynkApiKey[36] = "";        // Auth token for Blynk server. If empty, no connection will be made

uint16_t realtimeTimeoutMs = 2500;             // ms timeout of realtime mode before returning to normal mode
int arlsOffset = 0;                            // realtime LED offset
bool receiveDirect = true;                     // receive UDP realtime
bool arlsDisableGammaCorrection = true;        // activate if gamma correction is handled by the source
bool arlsForceMaxBri = false;                  // enable to force max brightness if source has very dark colors that would be black

uint16_t e131Universe = 1;                                      // settings for E1.31 (sACN) protocol (only DMX_MODE_MULTIPLE_* can span over consequtive universes)
uint8_t DMXMode = DMX_MODE_MULTIPLE_RGB;                        // DMX mode (s.a.)
uint16_t DMXAddress = 1;                                        // DMX start address of fixture, a.k.a. first Channel [for E1.31 (sACN) protocol]
uint8_t DMXOldDimmer = 0;                                       // only update brightness on change
uint8_t e131LastSequenceNumber[E131_MAX_UNIVERSE_COUNT];        // to detect packet loss
bool e131Multicast = false;                                     // multicast or unicast
bool e131SkipOutOfSequence = false;                             // freeze instead of flickering

bool mqttEnabled = false;
char mqttDeviceTopic[33] = "";               // main MQTT topic (individual per device, default is wled/mac)
char mqttGroupTopic[33] = "wled/all";        // second MQTT topic (for example to group devices)
char mqttServer[33] = "";                    // both domains and IPs should work (no SSL)
char mqttUser[41] = "";                      // optional: username for MQTT auth
char mqttPass[41] = "";                      // optional: password for MQTT auth
char mqttClientID[41] = "";                  // override the client ID
uint16_t mqttPort = 1883;

bool huePollingEnabled = false;           // poll hue bridge for light state
uint16_t huePollIntervalMs = 2500;        // low values (< 1sec) may cause lag but offer quicker response
char hueApiKey[47] = "api";               // key token will be obtained from bridge
byte huePollLightId = 1;                  // ID of hue lamp to sync to. Find the ID in the hue app ("about" section)
IPAddress hueIP = (0, 0, 0, 0);           // IP address of the bridge
bool hueApplyOnOff = true;
bool hueApplyBri = true;
bool hueApplyColor = true;

// Time CONFIG
bool ntpEnabled = false;         // get internet time. Only required if you use clock overlays or time-activated macros
bool useAMPM = false;            // 12h/24h clock format
byte currentTimezone = 0;        // Timezone ID. Refer to timezones array in wled10_ntp.ino
int utcOffsetSecs = 0;           // Seconds to offset from UTC before timzone calculation

byte overlayDefault = 0;                               // 0: no overlay 1: analog clock 2: single-digit clocl 3: cronixie
byte overlayMin = 0, overlayMax = ledCount - 1;        // boundaries of overlay mode

byte analogClock12pixel = 0;                 // The pixel in your strip where "midnight" would be
bool analogClockSecondsTrail = false;        // Display seconds as trail of LEDs instead of a single pixel
bool analogClock5MinuteMarks = false;        // Light pixels at every 5-minute position

char cronixieDisplay[7] = "HHMMSS";        // Cronixie Display mask. See wled13_cronixie.ino
bool cronixieBacklight = true;             // Allow digits to be back-illuminated

bool countdownMode = false;                         // Clock will count down towards date
byte countdownYear = 20, countdownMonth = 1;        // Countdown target date, year is last two digits
byte countdownDay = 1, countdownHour = 0;
byte countdownMin = 0, countdownSec = 0;

byte macroBoot = 0;        // macro loaded after startup
byte macroNl = 0;          // after nightlight delay over
byte macroCountdown = 0;
byte macroAlexaOn = 0, macroAlexaOff = 0;
byte macroButton = 0, macroLongPress = 0, macroDoublePress = 0;

// Security CONFIG
bool otaLock = false;           // prevents OTA firmware updates without password. ALWAYS enable if system exposed to any public networks
bool wifiLock = false;          // prevents access to WiFi settings when OTA lock is enabled
bool aOtaEnabled = true;        // ArduinoOTA allows easy updates directly from the IDE. Careful, it does not auto-disable when OTA lock is on

uint16_t userVar0 = 0, userVar1 = 0;

#ifdef WLED_ENABLE_DMX
  // dmx CONFIG
  byte DMXChannels = 7;        // number of channels per fixture
  byte DMXFixtureMap[15] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  // assigns the different channels to different functions. See wled21_dmx.ino for more information.
  uint16_t DMXGap = 10;          // gap between the fixtures. makes addressing easier because you don't have to memorize odd numbers when climbing up onto a rig.
  uint16_t DMXStart = 10;        // start address of the first fixture
#endif

// internal global variable declarations
// wifi
bool apActive = false;
bool forceReconnect = false;
uint32_t lastReconnectAttempt = 0;
bool interfacesInited = false;
bool wasConnected = false;

// color
byte colOld[] { 0, 0, 0, 0 };        // color before transition
byte colT[] { 0, 0, 0, 0 };          // color that is currently displayed on the LEDs
byte colIT[] { 0, 0, 0, 0 };         // color that was last sent to LEDs
byte colSecT[] { 0, 0, 0, 0 };
byte colSecOld[] { 0, 0, 0, 0 };
byte colSecIT[] { 0, 0, 0, 0 };

byte lastRandomIndex = 0;        // used to save last random color so the new one is not the same

// transitions
bool transitionActive = false;
uint16_t transitionDelayDefault = transitionDelay;
uint16_t transitionDelayTemp = transitionDelay;
unsigned long transitionStartTime;
float tperLast = 0;        // crossfade transition progress, 0.0f - 1.0f
bool jsonTransitionOnce = false;

// nightlight
bool nightlightActive = false;
bool nightlightActiveOld = false;
uint32_t nightlightDelayMs = 10;
uint8_t nightlightDelayMinsDefault = nightlightDelayMins;
unsigned long nightlightStartTime;
byte briNlT = 0;                     // current nightlight brightness
byte colNlT[] { 0, 0, 0, 0 };        // current nightlight color

// brightness
unsigned long lastOnTime = 0;
bool offMode = !turnOnAtBoot;
byte bri = briS;
byte briOld = 0;
byte briT = 0;
byte briIT = 0;
byte briLast = 128;          // brightness before turned off. Used for toggle function
byte whiteLast = 128;        // white channel before turned off. Used for toggle function

// button
bool buttonPressedBefore = false;
bool buttonLongPressed = false;
unsigned long buttonPressedTime = 0;
unsigned long buttonWaitTime = 0;

// notifications
bool notifyDirectDefault = notifyDirect;
bool receiveNotifications = true;
unsigned long notificationSentTime = 0;
byte notificationSentCallMode = NOTIFIER_CALL_MODE_INIT;
bool notificationTwoRequired = false;

// effects
byte effectCurrent = 0;
byte effectSpeed = 128;
byte effectIntensity = 128;
byte effectPalette = 0;

// network
bool udpConnected = false, udpRgbConnected = false;

// ui style
bool showWelcomePage = false;

// hue
byte hueError = HUE_ERROR_INACTIVE;
// uint16_t hueFailCount = 0;
float hueXLast = 0, hueYLast = 0;
uint16_t hueHueLast = 0, hueCtLast = 0;
byte hueSatLast = 0, hueBriLast = 0;
unsigned long hueLastRequestSent = 0;
bool hueAuthRequired = false;
bool hueReceived = false;
bool hueStoreAllowed = false, hueNewKey = false;

// overlays
byte overlayCurrent = overlayDefault;
byte overlaySpeed = 200;
unsigned long overlayRefreshMs = 200;
unsigned long overlayRefreshedTime;

// cronixie
byte dP[] { 0, 0, 0, 0, 0, 0 };
bool cronixieInit = false;

// countdown
unsigned long countdownTime = 1514764800L;
bool countdownOverTriggered = true;

// timer
byte lastTimerMinute = 0;
byte timerHours[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
byte timerMinutes[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
byte timerMacro[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
byte timerWeekday[] = { 255, 255, 255, 255, 255, 255, 255, 255 };        // weekdays to activate on
// bit pattern of arr elem: 0b11111111: sun,sat,fri,thu,wed,tue,mon,validity

// blynk
bool blynkEnabled = false;

// preset cycling
bool presetCyclingEnabled = false;
byte presetCycleMin = 1, presetCycleMax = 5;
uint16_t presetCycleTime = 1250;
unsigned long presetCycledTime = 0;
byte presetCycCurr = presetCycleMin;
bool presetApplyBri = true;
bool saveCurrPresetCycConf = false;

// realtime
byte realtimeMode = REALTIME_MODE_INACTIVE;
IPAddress realtimeIP = (0, 0, 0, 0);
unsigned long realtimeTimeout = 0;

// mqtt
long lastMqttReconnectAttempt = 0;
long lastInterfaceUpdate = 0;
byte interfaceUpdateCallMode = NOTIFIER_CALL_MODE_INIT;
char mqttStatusTopic[40] = "";        // this must be global because of async handlers

#if AUXPIN >= 0
  // auxiliary debug pin
  byte auxTime = 0;
  unsigned long auxStartTime = 0;
  bool auxActive = false, auxActiveBefore = false;
#endif

// alexa udp
String escapedMac;
#ifndef WLED_DISABLE_ALEXA
  Espalexa espalexa;
  EspalexaDevice* espalexaDevice;
#endif

// dns server
DNSServer dnsServer;

// network time
bool ntpConnected = false;
time_t local = 0;
unsigned long ntpLastSyncTime = 999000000L;
unsigned long ntpPacketSentTime = 999000000L;
IPAddress ntpServerIP;
uint16_t ntpLocalPort = 2390;

// Temp buffer
char* obuf;
uint16_t olen = 0;

// presets
uint16_t savedPresets = 0;
int8_t currentPreset = -1;
bool isPreset = false;

byte errorFlag = 0;

String messageHead, messageSub;
byte optionType;

bool doReboot = false;        // flag to initiate reboot from async handlers
bool doPublishMqtt = false;

// server library objects
AsyncWebServer server(80);
AsyncClient* hueClient = NULL;
AsyncMqttClient* mqtt = NULL;

// udp interface objects
WiFiUDP notifierUdp, rgbUdp;
WiFiUDP ntpUdp;
ESPAsyncE131 e131(handleE131Packet);
bool e131NewData = false;
ArtnetnodeWifi artnet;
bool artnetNewData = false;

// led fx library object
WS2812FX strip = WS2812FX();

// debug macro variable definitions
#ifdef WLED_DEBUG
  unsigned long debugTime = 0;
  int lastWifiState = 3;
  unsigned long wifiStateChangedTime = 0;
  int loops = 0;
#endif

WLED::WLED()
{
}

// turns all LEDs off and restarts ESP
void WLED::reset()
{
  briT = 0;
  long dly = millis();
  while (millis() - dly < 250) {
    yield();        // enough time to send response to client
  }
  setAllLeds();
  DEBUG_PRINTLN("MODULE RESET");
  ESP.restart();
}

bool oappendi(int i)
{
  char s[11];
  sprintf(s, "%ld", i);
  return oappend(s);
}

bool oappend(const char* txt)
{
  uint16_t len = strlen(txt);
  if (olen + len >= OMAX)
    return false;        // buffer full
  strcpy(obuf + olen, txt);
  olen += len;
  return true;
}

void WLED::loop()
{
  handleIR();        // 2nd call to function needed for ESP32 to return valid results -- should be good for ESP8266, too
  handleConnection();
  handleSerial();
  handleNotifications();
  handleTransitions();
#ifdef WLED_ENABLE_DMX
  handleDMX();
#endif
  userLoop();

  yield();
  handleIO();
  handleIR();
  handleNetworkTime();
  handleAlexa();

  handleOverlays();
  yield();
#ifdef WLED_USE_ANALOG_LEDS
  strip.setRgbwPwm();
#endif

  if (doReboot)
    reset();

  if (!realtimeMode)        // block stuff if WARLS/Adalight is enabled
  {
    if (apActive)
      dnsServer.processNextRequest();
#ifndef WLED_DISABLE_OTA
    if (WLED_CONNECTED && aOtaEnabled)
      ArduinoOTA.handle();
#endif
    handleNightlight();
    yield();

    handleHue();
    handleBlynk();

    yield();
    if (!offMode)
      strip.service();
  }
  yield();
#ifdef ESP8266
  MDNS.update();
#endif
  if (millis() - lastMqttReconnectAttempt > 30000)
    initMqtt();

// DEBUG serial logging
#ifdef WLED_DEBUG
  if (millis() - debugTime > 9999) {
    DEBUG_PRINTLN("---DEBUG INFO---");
    DEBUG_PRINT("Runtime: ");
    DEBUG_PRINTLN(millis());
    DEBUG_PRINT("Unix time: ");
    DEBUG_PRINTLN(now());
    DEBUG_PRINT("Free heap: ");
    DEBUG_PRINTLN(ESP.getFreeHeap());
    DEBUG_PRINT("Wifi state: ");
    DEBUG_PRINTLN(WiFi.status());
    if (WiFi.status() != lastWifiState) {
      wifiStateChangedTime = millis();
    }
    lastWifiState = WiFi.status();
    DEBUG_PRINT("State time: ");
    DEBUG_PRINTLN(wifiStateChangedTime);
    DEBUG_PRINT("NTP last sync: ");
    DEBUG_PRINTLN(ntpLastSyncTime);
    DEBUG_PRINT("Client IP: ");
    DEBUG_PRINTLN(WiFi.localIP());
    DEBUG_PRINT("Loops/sec: ");
    DEBUG_PRINTLN(loops / 10);
    loops = 0;
    debugTime = millis();
  }
  loops++;
#endif        // WLED_DEBU
}

void WLED::setup()
{
  EEPROM.begin(EEPSIZE);
  ledCount = EEPROM.read(229) + ((EEPROM.read(398) << 8) & 0xFF00);
  if (ledCount > MAX_LEDS || ledCount == 0)
    ledCount = 30;

#ifdef ESP8266
  #if LEDPIN == 3
    if (ledCount > MAX_LEDS_DMA)
      ledCount = MAX_LEDS_DMA;        // DMA method uses too much ram
  #endif
#endif
  Serial.begin(115200);
  Serial.setTimeout(50);
  DEBUG_PRINTLN();
  DEBUG_PRINT("---WLED ");
  DEBUG_PRINT(versionString);
  DEBUG_PRINT(" ");
  DEBUG_PRINT(VERSION);
  DEBUG_PRINTLN(" INIT---");
#ifdef ARDUINO_ARCH_ESP32
  DEBUG_PRINT("esp32 ");
  DEBUG_PRINTLN(ESP.getSdkVersion());
#else
  DEBUG_PRINT("esp8266 ");
  DEBUG_PRINTLN(ESP.getCoreVersion());
#endif
  int heapPreAlloc = ESP.getFreeHeap();
  DEBUG_PRINT("heap ");
  DEBUG_PRINTLN(ESP.getFreeHeap());

  strip.init(EEPROM.read(372), ledCount, EEPROM.read(2204));        // init LEDs quickly
  strip.setBrightness(0);

  DEBUG_PRINT("LEDs inited. heap usage ~");
  DEBUG_PRINTLN(heapPreAlloc - ESP.getFreeHeap());

#ifndef WLED_DISABLE_FILESYSTEM
  #ifdef ARDUINO_ARCH_ESP32
    SPIFFS.begin(true);
  #endif
    SPIFFS.begin();
#endif

  DEBUG_PRINTLN("Load EEPROM");
  loadSettingsFromEEPROM(true);
  beginStrip();
  userSetup();
  if (strcmp(clientSSID, DEFAULT_CLIENT_SSID) == 0)
    showWelcomePage = true;
  WiFi.persistent(false);

  if (macroBoot > 0)
    applyMacro(macroBoot);
  Serial.println("Ada");

  // generate module IDs
  escapedMac = WiFi.macAddress();
  escapedMac.replace(":", "");
  escapedMac.toLowerCase();
  if (strcmp(cmDNS, "x") == 0)        // fill in unique mdns default
  {
    strcpy(cmDNS, "wled-");
    sprintf(cmDNS + 5, "%*s", 6, escapedMac.c_str() + 6);
  }
  if (mqttDeviceTopic[0] == 0) {
    strcpy(mqttDeviceTopic, "wled/");
    sprintf(mqttDeviceTopic + 5, "%*s", 6, escapedMac.c_str() + 6);
  }
  if (mqttClientID[0] == 0) {
    strcpy(mqttClientID, "WLED-");
    sprintf(mqttClientID + 5, "%*s", 6, escapedMac.c_str() + 6);
  }

  strip.service();

#ifndef WLED_DISABLE_OTA
  if (aOtaEnabled) {
    ArduinoOTA.onStart([]() {
#ifdef ESP8266
      wifi_set_sleep_type(NONE_SLEEP_T);
#endif
      DEBUG_PRINTLN("Start ArduinoOTA");
    });
    if (strlen(cmDNS) > 0)
      ArduinoOTA.setHostname(cmDNS);
  }
#endif
#ifdef WLED_ENABLE_DMX
  dmx.init(512);        // initialize with bus length
#endif
  // HTTP server page init
  initServer();
}

void WLED::beginStrip()
{
  // Initialize NeoPixel Strip and button
  strip.setShowCallback(handleOverlayDraw);

#ifdef BTNPIN
  pinMode(BTNPIN, INPUT_PULLUP);
#endif

  if (bootPreset > 0)
    applyPreset(bootPreset, turnOnAtBoot);
  colorUpdated(NOTIFIER_CALL_MODE_INIT);

// init relay pin
#if RLYPIN >= 0
  pinMode(RLYPIN, OUTPUT);
#if RLYMDE
  digitalWrite(RLYPIN, bri);
#else
  digitalWrite(RLYPIN, !bri);
#endif
#endif

  // disable button if it is "pressed" unintentionally
#ifdef BTNPIN
  if (digitalRead(BTNPIN) == LOW)
    buttonEnabled = false;
#else
  buttonEnabled = false;
#endif
}

void WLED::initAP(bool resetAP)
{
  if (apBehavior == AP_BEHAVIOR_BUTTON_ONLY && !resetAP)
    return;

  if (!apSSID[0] || resetAP)
    strcpy(apSSID, "WLED-AP");
  if (resetAP)
    strcpy(apPass, DEFAULT_AP_PASS);
  DEBUG_PRINT("Opening access point ");
  DEBUG_PRINTLN(apSSID);
  WiFi.softAPConfig(IPAddress(4, 3, 2, 1), IPAddress(4, 3, 2, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID, apPass, apChannel, apHide);

  if (!apActive)        // start captive portal if AP active
  {
    DEBUG_PRINTLN("Init AP interfaces");
    server.begin();
    if (udpPort > 0 && udpPort != ntpLocalPort) {
      udpConnected = notifierUdp.begin(udpPort);
    }
    if (udpRgbPort > 0 && udpRgbPort != ntpLocalPort && udpRgbPort != udpPort) {
      udpRgbConnected = rgbUdp.begin(udpRgbPort);
    }

    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", WiFi.softAPIP());
  }
  apActive = true;
}

void WLED::initConnection()
{
  WiFi.disconnect();        // close old connections
#ifdef ESP8266
  WiFi.setPhyMode(WIFI_PHY_MODE_11N);
#endif

  if (staticIP[0] != 0 && staticGateway[0] != 0) {
    WiFi.config(staticIP, staticGateway, staticSubnet, IPAddress(8, 8, 8, 8));
  } else {
    WiFi.config(0U, 0U, 0U);
  }

  lastReconnectAttempt = millis();

  if (!WLED_WIFI_CONFIGURED) {
    DEBUG_PRINT("No connection configured. ");
    if (!apActive)
      initAP();        // instantly go to ap mode
    return;
  } else if (!apActive) {
    if (apBehavior == AP_BEHAVIOR_ALWAYS) {
      initAP();
    } else {
      DEBUG_PRINTLN("Access point disabled.");
      WiFi.softAPdisconnect(true);
    }
  }
  showWelcomePage = false;

  DEBUG_PRINT("Connecting to ");
  DEBUG_PRINT(clientSSID);
  DEBUG_PRINTLN("...");

#ifdef ESP8266
  WiFi.hostname(serverDescription);
#endif

  WiFi.begin(clientSSID, clientPass);

#ifdef ARDUINO_ARCH_ESP32
  WiFi.setSleep(!noWifiSleep);
  WiFi.setHostname(serverDescription);
#else
  wifi_set_sleep_type((noWifiSleep) ? NONE_SLEEP_T : MODEM_SLEEP_T);
#endif
}

void WLED::initInterfaces()
{
  DEBUG_PRINTLN("Init STA interfaces");

  if (hueIP[0] == 0) {
    hueIP[0] = WiFi.localIP()[0];
    hueIP[1] = WiFi.localIP()[1];
    hueIP[2] = WiFi.localIP()[2];
  }

  // init Alexa hue emulation
  if (alexaEnabled)
    alexaInit();

#ifndef WLED_DISABLE_OTA
  if (aOtaEnabled)
    ArduinoOTA.begin();
#endif

  strip.service();
  // Set up mDNS responder:
  if (strlen(cmDNS) > 0) {
    if (!aOtaEnabled)
      MDNS.begin(cmDNS);

    DEBUG_PRINTLN("mDNS started");
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("wled", "tcp", 80);
    MDNS.addServiceTxt("wled", "tcp", "mac", escapedMac.c_str());
  }
  server.begin();

  if (udpPort > 0 && udpPort != ntpLocalPort) {
    udpConnected = notifierUdp.begin(udpPort);
    if (udpConnected && udpRgbPort != udpPort)
      udpRgbConnected = rgbUdp.begin(udpRgbPort);
  }
  if (ntpEnabled)
    ntpConnected = ntpUdp.begin(ntpLocalPort);

  initBlynk(blynkApiKey);
  e131.begin((e131Multicast) ? E131_MULTICAST : E131_UNICAST, e131Universe, E131_MAX_UNIVERSE_COUNT);
  #ifndef WLED_DISABLE_ARTNET
    artnet.setArtDmxCallback(handleArtnetPacket);
    artnet.setName("ESP32-ArtnetTEST");   // TODO: Change to pull from config.
    artnet.setNumPorts(1);
    artnet.enableDMXOutput(0);
    artnet.setStartingUniverse(1);    // TODO: Pull.
    artnet.begin();                   // TODO: If writing, need to change to remote IP.
  #endif
  reconnectHue();
  initMqtt();
  interfacesInited = true;
  wasConnected = true;
}

byte stacO = 0;
uint32_t lastHeap;
unsigned long heapTime = 0;

void WLED::handleConnection()
{
  if (millis() < 2000 && (!WLED_WIFI_CONFIGURED || apBehavior == AP_BEHAVIOR_ALWAYS))
    return;
  if (lastReconnectAttempt == 0)
    initConnection();

  // reconnect WiFi to clear stale allocations if heap gets too low
  if (millis() - heapTime > 5000) {
    uint32_t heap = ESP.getFreeHeap();
    if (heap < 9000 && lastHeap < 9000) {
      DEBUG_PRINT("Heap too low! ");
      DEBUG_PRINTLN(heap);
      forceReconnect = true;
    }
    lastHeap = heap;
    heapTime = millis();
  }

  byte stac = 0;
  if (apActive) {
#ifdef ESP8266
    stac = wifi_softap_get_station_num();
#else
    wifi_sta_list_t stationList;
    esp_wifi_ap_get_sta_list(&stationList);
    stac = stationList.num;
#endif
    if (stac != stacO) {
      stacO = stac;
      DEBUG_PRINT("Connected AP clients: ");
      DEBUG_PRINTLN(stac);
      if (!WLED_CONNECTED && WLED_WIFI_CONFIGURED) {        // trying to connect, but not connected
        if (stac)
          WiFi.disconnect();        // disable search so that AP can work
        else
          initConnection();        // restart search
      }
    }
  }
  if (forceReconnect) {
    DEBUG_PRINTLN("Forcing reconnect.");
    initConnection();
    interfacesInited = false;
    forceReconnect = false;
    wasConnected = false;
    return;
  }
  if (!WLED_CONNECTED) {
    if (interfacesInited) {
      DEBUG_PRINTLN("Disconnected!");
      interfacesInited = false;
      initConnection();
    }
    if (millis() - lastReconnectAttempt > ((stac) ? 300000 : 20000) && WLED_WIFI_CONFIGURED)
      initConnection();
    if (!apActive && millis() - lastReconnectAttempt > 12000 && (!wasConnected || apBehavior == AP_BEHAVIOR_NO_CONN))
      initAP();
  } else if (!interfacesInited) {        // newly connected
    DEBUG_PRINTLN("");
    DEBUG_PRINT("Connected! IP address: ");
    DEBUG_PRINTLN(WiFi.localIP());
    initInterfaces();
    userConnected();

    // shut down AP
    if (apBehavior != AP_BEHAVIOR_ALWAYS && apActive) {
      dnsServer.stop();
      WiFi.softAPdisconnect(true);
      apActive = false;
      DEBUG_PRINTLN("Access point disabled.");
    }
  }
}
