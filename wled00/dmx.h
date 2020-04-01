#ifndef WLED_DMX_H
#define WLED_DMX_H
#include "const.h"
#include <Arduino.h>

/*
Support for DMX512 control and output.
E131 and Artnet are supported for control.
Basic Remote Device Management (RDM) features are supported with Artnet.
DMX output is provided for the MAX485 transceiver.
The Artnet library contains scaffolding for WiFi transmission of DMX output,
but is unimplemented.
TODO:
 * Change the output pin in src/dependencies/ESPDMX.cpp if needed.
 */
// class ESPAsyncE131;
// class ArtnetnodeWifi;

#define E131_MAX_UNIVERSE_COUNT 9
// settings for E1.31 (sACN) protocol (only DMX_MODE_MULTIPLE_* can
// span over consequtive universes)
extern uint16_t e131Universe;
extern uint8_t DMXMode; 
extern uint16_t DMXAddress; 
                        
extern uint8_t DMXOldDimmer;         
extern bool e131Multicast;        
extern bool e131SkipOutOfSequence;
extern byte DMXChannels;
extern byte DMXFixtureMap[15];
extern uint16_t DMXGap;
extern uint16_t DMXStart;

//#ifdef WLED_ENABLE_ARTNET
#include "src/dependencies/artnet-node-wifi/ArtnetnodeWifi.h"
//#else
//class ArtnetnodeWifi {};
//#endif

#define DMX_FUNC_PARAM uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data
#include "src/dependencies/e131/ESPAsyncE131.h"

#include "src/dependencies/dmx/ESPDMX.h"

void handleE131Packet(e131_packet_t *p, IPAddress clientIP);
void handleArtnetPacket(DMX_FUNC_PARAM);
class DMX512 {
  friend void handleE131Packet(e131_packet_t *p, IPAddress clientIP);
  friend void handleArtnetPacket(DMX_FUNC_PARAM);
public:
  DMX512();
  //~DMX512();

  bool hasNewData() { return newData; }
  void resetNewData() { newData = false; }
  bool newData;
  ESPAsyncE131 e131;
  ArtnetnodeWifi artnet;

  // Testing only...
  void read() {
  #ifdef WLED_ENABLE_ARTNET
    artnet.read();
  #endif
    }  


  DMXESPSerial serial;
  void oinit(int i) { serial.init(i); }
  void handleDMXOutput();           
};
extern DMX512 dmx512;


#endif