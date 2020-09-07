/***********************************
Double buffer serial line reading 
****************************************/

#ifndef _NMEA_SERIAL_H
#define _NMEA_SERIAL_H

#ifdef __AVR__
  #if ARDUINO >= 100
    #include <SoftwareSerial.h>
  #else
    #include <NewSoftSerial.h>
  #endif
#endif

// how long to wait when we're waiting for a response
#define MAXWAITSENTENCE 5
// how long are max NMEA lines to parse?
#define MAXLINELENGTH 120

#if ARDUINO >= 100
 #include "Arduino.h"
#if defined (__AVR__) && !defined(__AVR_ATmega32U4__)
 #include "SoftwareSerial.h"
#endif
#else
 #include "WProgram.h"
 #include "NewSoftSerial.h"
#endif


class NMEA_serial {
 public:

#ifdef __AVR__
  #if ARDUINO >= 100 
    NMEA_serial(SoftwareSerial *ser); // Constructor when using SoftwareSerial
  #else
    NMEA_serial(NewSoftSerial  *ser); // Constructor when using NewSoftSerial
  #endif
#endif
  NMEA_serial(HardwareSerial *ser); // Constructor when using HardwareSerial

  // set up serial port to appropriate baud rate
  void begin(uint16_t baud); 
  // return pointer to last complete NMEA string received
  char *lastNMEA(void);
  // indicate if unprocessed NMEA string is waiting
  boolean newNMEAreceived();
  // set up the double-buffer pointers, reset received flag
  void common_init(void);
  // pipe a command to the channel (radio or GPS mode changing option)
  void sendCommand(const char *);
  // read a byte waiting at the serial port
  char read(void);
  //  not defined in the library? don't know what interruptReads is about
  //void interruptReads(boolean r);
  // when we absolutely have to see a particular sentence
  boolean waitForSentence(char *wait, uint8_t max = MAXWAITSENTENCE);
  
  // all of our public variables.  some should probably be private and accessed via methods, oh well
   // we double buffer: read one line in and leave one for the main program
  volatile char line1[MAXLINELENGTH];
  volatile char line2[MAXLINELENGTH];
  // our index into filling the current line
  volatile uint8_t lineidx;
  // pointers to the double buffers
  volatile char *currentline;
  volatile char *lastline;
  volatile boolean recvdflag;

 //private:
  //boolean paused;
  
  

#ifdef __AVR__
  #if ARDUINO >= 100
    SoftwareSerial *gpsSwSerial;
  #else
    NewSoftSerial  *gpsSwSerial;
  #endif
#endif
  HardwareSerial *gpsHwSerial;
};


#endif
