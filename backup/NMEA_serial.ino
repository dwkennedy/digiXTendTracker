/***************************************
This is our NMEA-style serial library

This is the parent class of NMEA GPS and digi XTend

It handles the reading of lines  of the format $.....\n

****************************************/

#include "NMEA_serial.h"



// Initialization code used by all constructor types
void NMEA_serial::common_init(void) {
  recvdflag   = false;
  lineidx     = 0;
  currentline = line1;
  lastline    = line2;

}

#ifdef __AVR__
// Constructor when using SoftwareSerial or NewSoftSerial
#if ARDUINO >= 100
NMEA_serial::NMEA_serial(SoftwareSerial *ser)
#else
NMEA_serial::NMEA_serial(NewSoftSerial *ser) 
#endif
{
  // we double buffer: read one line in and leave one for the main program
  volatile char line1[MAXLINELENGTH];
  volatile char line2[MAXLINELENGTH];
  // our index into filling the current line
  volatile uint8_t lineidx=0;
  // pointers to the double buffers
  volatile char *currentline;
  volatile char *lastline;
  volatile boolean recvdflag;
 
  common_init();     // Set everything to common state, then...
  gpsSwSerial = ser; // ...override gpsSwSerial with value passed.
}
#endif

NMEA_serial::NMEA_serial(HardwareSerial *ser) {
  // we double buffer: read one line in and leave one for the main program
  volatile char line1[MAXLINELENGTH];
  volatile char line2[MAXLINELENGTH];
  // our index into filling the current line
  volatile uint8_t lineidx=0;
  // pointers to the double buffers
  volatile char *currentline;
  volatile char *lastline;
  volatile boolean recvdflag;

  common_init();  // Set everything to common state, then...
  gpsHwSerial = ser; // ...override gpsHwSerial with value passed.
}





char NMEA_serial::read(void) {
  char c = 0;
  
#ifdef __AVR__
  if(gpsSwSerial) {
    if(!gpsSwSerial->available()) return c;
    c = gpsSwSerial->read();
  }
  else
  {
    if(!gpsHwSerial->available()) return c;
    c = gpsHwSerial->read();
  }
#else
     //   if(!gpsHwSerial->available()) return c;
      //  c = gpsHwSerial->read();
    if(!Serial1.available()) return c;
    c = Serial1.read();
#endif


  //Serial.print(c);

// snatch up those NMEA-like codes

// if $ received, nail incoming bytes to start of line
  if (c == '$') {
    currentline[lineidx] = 0;
    lineidx = 0;
  }
  // newline (end of current line) detected, swap buffers and raise recv'd flag
  if (c == '\n') {
    currentline[lineidx] = 0;

    if (currentline == line1) {
      currentline = line2;
      lastline = line1;
    } else {
      currentline = line1;
      lastline = line2;
    }

    //Serial.println("----");
    //Serial.println((char *)lastline);
    //Serial.println("----");
    lineidx = 0;
    recvdflag = true;
  } else {

    // store recv'd character in current buffer
    currentline[lineidx++] = c;
    if (lineidx >= MAXLINELENGTH)
      lineidx = MAXLINELENGTH-1;
  }

  return c;
}



void NMEA_serial::begin(uint16_t baud)
{
#ifdef __AVR__
  if(gpsSwSerial) 
    gpsSwSerial->begin(baud);
  else 
    gpsHwSerial->begin(baud);
#else
  //  gpsHwSerial->begin(baud);
    Serial1.begin(baud);
#endif

  delay(10);
}

void NMEA_serial::sendCommand(const char str[]) {
#ifdef __AVR__
  if(gpsSwSerial) 
    gpsSwSerial->println(str);
  else
    gpsHwSerial->println(str);
#else
  //  gpsHwSerial->println(str);
    Serial1.println(str);
#endif

}

boolean NMEA_serial::newNMEAreceived(void) {
  return recvdflag;
}

char *NMEA_serial::lastNMEA(void) {
  recvdflag = false;
  return (char *)lastline;
}

// wait fo particular string (like OK response from digi XTend modem)
boolean NMEA_serial::waitForSentence(char *wait4me, uint8_t max) {
  char str[20];

  uint8_t i=0;
  while (i < max) {
    if (newNMEAreceived()) { 
      char *nmea = lastNMEA();
      strncpy(str, nmea, 20);
      str[19] = 0;
      i++;

      if (strstr(str, wait4me))
	return true;
    }
  }

  return false;
}
