// functions to parse the NMEA GPS sentences

#ifndef _GPS_NMEA_H
#define _GPS_NMEA_H

#include "NMEA_serial.h"
#include "checksum.h"

// different commands to set the update rate from once a second (1 Hz) to 10 times a second (10Hz)
//#define PMTK_SET_NMEA_UPDATE_1HZ F("$PMTK220,1000*1F")
#define PMTK_SET_NMEA_UPDATE_5HZ F("$PMTK220,200*2C")
#define PMTK_SET_NMEA_UPDATE_10HZ F("$PMTK220,100*2F")


#define PMTK_SET_BAUD_57600 F("$PMTK251,57600*2C")
#define PMTK_SET_BAUD_9600 F("$PMTK251,9600*17")

// turn on only the second sentence (GPRMC)
#define PMTK_SET_NMEA_OUTPUT_RMCONLY F("$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29")
// turn on GPRMC and GGA
//#define PMTK_SET_NMEA_OUTPUT_RMCGGA F("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28")
// turn on ALL THE DATA
#define PMTK_SET_NMEA_OUTPUT_ALLDATA F("$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28")
// turn off output
#define PMTK_SET_NMEA_OUTPUT_OFF F("$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28")

// to generate your own sentences, check out the MTK command datasheet and use a checksum calculator
// such as the awesome http://www.hhhh.org/wiml/proj/nmeaxor.html

#define PMTK_LOCUS_STARTLOG  F("$PMTK185,0*22")
#define PMTK_LOCUS_LOGSTARTED "$PMTK001,185,3*3C"
#define PMTK_LOCUS_QUERY_STATUS "$PMTK183*38"
#define PMTK_LOCUS_ERASE_FLASH "$PMTK184,1*22"
#define LOCUS_OVERLAP 0
#define LOCUS_FULLSTOP 1

// standby command & boot successful message
#define PMTK_STANDBY "$PMTK161,0*28"
#define PMTK_STANDBY_SUCCESS "$PMTK001,161,3*36"  // Not needed currently
#define PMTK_AWAKE "$PMTK010,002*2D"

// ask for the release and version
#define PMTK_Q_RELEASE "$PMTK605*31"

// request for updates on antenna status 
#define PGCMD_ANTENNA "$PGCMD,33,1*6C" 
#define PGCMD_NOANTENNA "$PGCMD,33,0*6C" 

class GPS_NMEA {
  public:
    GPS_NMEA(); // Constructor
  
    boolean parseNMEA(char *response);

    char *time, *date, *latitude, *longitude, *altitude, *speed, *angle;
    //uint8_t hour, minute, seconds, year, month, day;
    //uint16_t milliseconds;
    //float latitude, longitude, altitude;
    float geoidheight;
    //float fspeed, fangle;
    //float magvariation, HDOP;
    char lat, lon;
    //char mag;
    boolean fix;
    //uint8_t fixquality, satellites;
    char *fixquality, *satellites;

};

#endif
