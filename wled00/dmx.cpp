#include "dmx.h"
#include "wled.h"
#include <functional>
// https://isocpp.org/wiki/faq/ctors#static-const-with-initializers
// Note about how references to unused static variables are elided out of the data area

uint16_t e131Universe = 1;
uint8_t DMXMode = DMX_MODE_MULTIPLE_RGB; // DMX mode (s.a.)
uint16_t DMXAddress = 1; // DMX start address of fixture, a.k.a. first Channel
                          // [for E1.31 (sACN) protocol]
uint8_t DMXOldDimmer = 0;           // only update brightness on change
bool e131Multicast = false;         // multicast or unicast
bool e131SkipOutOfSequence = false; // freeze instead of flickering
uint8_t
    e131LastSequenceNumber[E131_MAX_UNIVERSE_COUNT]; // to detect packet loss
DMX512 dmx512;

#ifdef WLED_ENABLE_DMXOUT
  byte DMXChannels = 7; // number of channels per fixture
  byte DMXFixtureMap[15] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  // assigns the different channels to different functions.
  uint16_t DMXGap =
      10; // gap between the fixtures. makes addressing easier because you don't
          // have to memorize odd numbers when climbing up onto a rig.
  uint16_t DMXStart = 10; // start address of the first fixture
#endif



DMX512::DMX512() : e131(handleE131Packet) {
  //e131 = ESPAsyncE131(std::function<void>(std::bind(DMX512::handleE131Packet, std::placeholders::_1), this));
  // e131 = ESPAsyncE131([this](auto p, auto c) { this->handleE131Packet(p, c);});
  // e131 = ESPAsyncE131(std::bind(&DMX512::handleE131Packet, this, std::placeholders::_1));
#ifdef WLED_ENABLE_E131
  e131.begin((e131Multicast) ? E131_MULTICAST : E131_UNICAST, e131Universe, E131_MAX_UNIVERSE_COUNT);
#endif
#ifdef WLED_ENABLE_ARTNET

  artnet.setArtDmxFunc(handleArtnetPacket);
  artnet.setName("ESP32-Artnet");   // TODO: Change to pull from config.
  artnet.setNumPorts(1);
  artnet.enableDMXOutput(0);
  artnet.setStartingUniverse(1);    // TODO: Pull.
  artnet.begin();                   // TODO: If writing, need to change to remote IP.
#endif
}

DMX512::~DMX512() = default;

/*
 * E1.31 handler
 */
void handleE131Packet(e131_packet_t* p, IPAddress clientIP){
  #ifdef WLED_ENABLE_E131
  //E1.31 protocol support

  uint16_t uni = htons(p->universe);
  uint8_t previousUniverses = uni - e131Universe;
  uint16_t possibleLEDsInCurrentUniverse;
  uint16_t dmxChannels = htons(p->property_value_count) -1;

  // only listen for universes we're handling & allocated memory
  if (uni >= (e131Universe + E131_MAX_UNIVERSE_COUNT)) return;

  if (e131SkipOutOfSequence)
    if (p->sequence_number < e131LastSequenceNumber[uni-e131Universe] && p->sequence_number > 20 && e131LastSequenceNumber[uni-e131Universe] < 250){
      DEBUG_PRINT("skipping E1.31 frame (last seq=");
      DEBUG_PRINT(e131LastSequenceNumber[uni-e131Universe]);
      DEBUG_PRINT(", current seq=");
      DEBUG_PRINT(p->sequence_number);
      DEBUG_PRINT(", universe=");
      DEBUG_PRINT(uni);
      DEBUG_PRINTLN(")");
      return;
    }
  e131LastSequenceNumber[uni-e131Universe] = p->sequence_number;

  // update status info
  realtimeIP = clientIP;
  
  switch (DMXMode) {
    case DMX_MODE_DISABLED:
      return;  // nothing to do
      break;

    case DMX_MODE_SINGLE_RGB:
      if (uni != e131Universe) return;
      if (dmxChannels-DMXAddress+1 < 3) return;
      arlsLock(realtimeTimeoutMs, REALTIME_MODE_E131);
      for (uint16_t i = 0; i < ledCount; i++)
        setRealtimePixel(i, p->property_values[DMXAddress+0], p->property_values[DMXAddress+1], p->property_values[DMXAddress+2], 0);
      break;

    case DMX_MODE_SINGLE_DRGB:
      if (uni != e131Universe) return;
      if (dmxChannels-DMXAddress+1 < 4) return;
      arlsLock(realtimeTimeoutMs, REALTIME_MODE_E131);
      if (DMXOldDimmer != p->property_values[DMXAddress+0]) {
        DMXOldDimmer = p->property_values[DMXAddress+0];
        bri = p->property_values[DMXAddress+0];
        strip.setBrightness(bri);
      }
      for (uint16_t i = 0; i < ledCount; i++)
        setRealtimePixel(i, p->property_values[DMXAddress+1], p->property_values[DMXAddress+2], p->property_values[DMXAddress+3], 0);
      break;

    case DMX_MODE_EFFECT:
      if (uni != e131Universe) return;
      if (dmxChannels-DMXAddress+1 < 11) return;
      if (DMXOldDimmer != p->property_values[DMXAddress+0]) {
        DMXOldDimmer = p->property_values[DMXAddress+0];
        bri = p->property_values[DMXAddress+0];
      }
      if (p->property_values[DMXAddress+1] < MODE_COUNT)
        effectCurrent = p->property_values[DMXAddress+ 1];
      effectSpeed     = p->property_values[DMXAddress+ 2];  // flickers
      effectIntensity = p->property_values[DMXAddress+ 3];
      effectPalette   = p->property_values[DMXAddress+ 4];
      col[0]          = p->property_values[DMXAddress+ 5];
      col[1]          = p->property_values[DMXAddress+ 6];
      col[2]          = p->property_values[DMXAddress+ 7];
      colSec[0]       = p->property_values[DMXAddress+ 8];
      colSec[1]       = p->property_values[DMXAddress+ 9];
      colSec[2]       = p->property_values[DMXAddress+10];
      if (dmxChannels-DMXAddress+1 > 11)
      {
        col[3]          = p->property_values[DMXAddress+11]; //white
        colSec[3]       = p->property_values[DMXAddress+12];
      }
      transitionDelayTemp = 0;                        // act fast
      colorUpdated(NOTIFIER_CALL_MODE_NOTIFICATION);  // don't send UDP
      return;                                         // don't activate realtime live mode
      break;

    case DMX_MODE_MULTIPLE_RGB:
      arlsLock(realtimeTimeoutMs, REALTIME_MODE_E131);
      if (previousUniverses == 0) {
        // first universe of this fixture
        possibleLEDsInCurrentUniverse = (dmxChannels - DMXAddress + 1) / 3;
        for (uint16_t i = 0; i < ledCount; i++) {
          if (i >= possibleLEDsInCurrentUniverse) break;  // more LEDs will follow in next universe(s)
          setRealtimePixel(i, p->property_values[DMXAddress+i*3+0], p->property_values[DMXAddress+i*3+1], p->property_values[DMXAddress+i*3+2], 0);
        }
      } else if (previousUniverses > 0 && uni < (e131Universe + E131_MAX_UNIVERSE_COUNT)) {
        // additional universe(s) of this fixture
        uint16_t numberOfLEDsInPreviousUniverses = ((512 - DMXAddress + 1) / 3);                            // first universe
        if (previousUniverses > 1) numberOfLEDsInPreviousUniverses += (512 / 3) * (previousUniverses - 1);  // extended universe(s) before current
        possibleLEDsInCurrentUniverse = dmxChannels / 3;
        for (uint16_t i = numberOfLEDsInPreviousUniverses; i < ledCount; i++) {
          uint8_t j = i - numberOfLEDsInPreviousUniverses;
          if (j >= possibleLEDsInCurrentUniverse) break;   // more LEDs will follow in next universe(s)
          setRealtimePixel(i, p->property_values[j*3+1], p->property_values[j*3+2], p->property_values[j*3+3], 0);
        }
      }
      break;

    case DMX_MODE_MULTIPLE_DRGB:
      arlsLock(realtimeTimeoutMs, REALTIME_MODE_E131);
      if (previousUniverses == 0) {
        // first universe of this fixture
        if (DMXOldDimmer != p->property_values[DMXAddress+0]) {
          DMXOldDimmer = p->property_values[DMXAddress+0];
          bri = p->property_values[DMXAddress+0];
          strip.setBrightness(bri);
        }
        possibleLEDsInCurrentUniverse = (dmxChannels - DMXAddress) / 3;
        for (uint16_t i = 0; i < ledCount; i++) {
          if (i >= possibleLEDsInCurrentUniverse) break;  // more LEDs will follow in next universe(s)
          setRealtimePixel(i, p->property_values[DMXAddress+i*3+1], p->property_values[DMXAddress+i*3+2], p->property_values[DMXAddress+i*3+3], 0);
        }
      } else if (previousUniverses > 0 && uni < (e131Universe + E131_MAX_UNIVERSE_COUNT)) {
        // additional universe(s) of this fixture
        uint16_t numberOfLEDsInPreviousUniverses = ((512 - DMXAddress + 1) / 3);                            // first universe
        if (previousUniverses > 1) numberOfLEDsInPreviousUniverses += (512 / 3) * (previousUniverses - 1);  // extended universe(s) before current
        possibleLEDsInCurrentUniverse = dmxChannels / 3;
        for (uint16_t i = numberOfLEDsInPreviousUniverses; i < ledCount; i++) {
          uint8_t j = i - numberOfLEDsInPreviousUniverses;
          if (j >= possibleLEDsInCurrentUniverse) break;   // more LEDs will follow in next universe(s)
          setRealtimePixel(i, p->property_values[j*3+1], p->property_values[j*3+2], p->property_values[j*3+3], 0);
        }
      }
      break;

    default:
      DEBUG_PRINTLN("unknown E1.31 DMX mode");
      return;  // nothing to do
      break;
  }

  dmx512.newData = true;
  #endif
}

#ifdef WLED_ENABLE_ARTNET
#include "src/dependencies/artnet-node-wifi/ArtnetnodeWifi.h"
// TODO:
// Check if we got all universes. TODO: Verify functionality
//int maxUniverses = numberOfChannels / 512 + ((numberOfChannels % 512) ? 1 : 0);
bool universesReceived[E131_MAX_UNIVERSE_COUNT];
int previousDataLength = 0;

void handleArtnetPacket(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{
  DEBUG_PRINTLN("Handle artnet begin.");
  bool sendFrame = true;

  // Store which universe has got in
  if ((universe - e131Universe) < E131_MAX_UNIVERSE_COUNT) {
    universesReceived[universe - e131Universe] = 1;
  }

  for (int i = 0 ; i < E131_MAX_UNIVERSE_COUNT ; i++)
  {
    if (universesReceived[i] == 0)
    {
      Serial.println("Broke");
      sendFrame = false;
      break;
    }
  }

  arlsLock(realtimeTimeoutMs, REALTIME_MODE_E131);

  // read universe and put into the right part of the display buffer
  for (int i = 0; i < length / 3; i++)
  {
    int led = i + (universe - e131Universe) * (previousDataLength / 3);
    if (led < ledCount) {
      setRealtimePixel(led, data[i * 3], data[i * 3 + 1], data[i * 3 + 2], 0);
    }
  }
  previousDataLength = length;

  if (sendFrame)
  {
    dmx512.newData = true;
    // Reset universeReceived to 0
    memset(universesReceived, 0, E131_MAX_UNIVERSE_COUNT);
  }
  DEBUG_PRINTLN("Handle artnet end.");
}
#else
void handleArtnetPacket(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {}
#endif


#ifdef WLED_ENABLE_DMXOUT

void DMX512::handleDMXOutput()
{
  // TODO: calculate brightness manually if no shutter channel is set
  uint8_t brightness = strip.getBrightness();

  for (int i = 0; i < ledCount; i++) {        // uses the amount of LEDs as fixture count

    uint32_t in = strip.getPixelColor(i);        // time to get the colors for the individual fixtures as suggested by AirCookie at issue #462
    byte w = in >> 24 & 0xFF;
    byte r = in >> 16 & 0xFF;
    byte g = in >> 8 & 0xFF;
    byte b = in & 0xFF;

    int DMXFixtureStart = DMXStart + (DMXGap * i);
    for (int j = 0; j < DMXChannels; j++) {
      int DMXAddr = DMXFixtureStart + j;
      switch (DMXFixtureMap[j]) {
        case 0:        // Set this channel to 0. Good way to tell strobe- and fade-functions to fuck right off.
          serial.write(DMXAddr, 0);
          break;
        case 1:        // Red
          serial.write(DMXAddr, r);
          break;
        case 2:        // Green
          serial.write(DMXAddr, g);
          break;
        case 3:        // Blue
          serial.write(DMXAddr, b);
          break;
        case 4:        // White
          serial.write(DMXAddr, w);
          break;
        case 5:        // Shutter channel. Controls the brightness.
          serial.write(DMXAddr, brightness);
          break;
        case 6:        // Sets this channel to 255. Like 0, but more wholesome.
          serial.write(DMXAddr, 255);
          break;
      }
    }
  }

  serial.update();        // update the DMX bus
}

#else
void DMX512::handleDMXOutput() {}
#endif
