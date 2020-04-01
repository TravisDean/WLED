#ifndef WLED_DMX_H
#define WLED_DMX_H
#include <Arduino.h>
#include "const.h"
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
class ESPAsyncE131;
class ArtnetnodeWifi;

extern ESPAsyncE131 e131;
extern bool e131NewData;
extern ArtnetnodeWifi artnet;
extern bool artnetNewData; 

#define E131_MAX_UNIVERSE_COUNT 9
extern uint16_t e131Universe;                                      // settings for E1.31 (sACN) protocol (only DMX_MODE_MULTIPLE_* can span over consequtive universes)
extern uint8_t DMXMode;                        
extern uint16_t DMXAddress;                                        // DMX start address of fixture, a.k.a. first Channel [for E1.31 (sACN) protocol]
extern uint8_t DMXOldDimmer;                                       // only update brightness on change
extern bool e131Multicast;                                     // multicast or unicast
extern bool e131SkipOutOfSequence;                             // freeze instead of flickering



void handleArtnetPacket(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data);


#ifdef WLED_ENABLE_DMXOUT
  extern byte DMXChannels; 
  extern byte DMXFixtureMap[15];
  extern uint16_t DMXGap;   
  extern uint16_t DMXStart; 
#endif

//dmx.cpp
void handleDMXOutput();

#endif