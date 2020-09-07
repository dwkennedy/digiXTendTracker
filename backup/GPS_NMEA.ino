#include "GPS_NMEA.h"

const char badChecksumMessage[] PROGMEM = {"Bad GPS NMEA checksum!"};


GPS_NMEA::GPS_NMEA(void) {
}

boolean GPS_NMEA::parseNMEA(char *nmea) {
  
  // do checksum check
  
  if (!verifyChecksum(nmea)) {
     // bad checksum :(
     Serial.println(badChecksumMessage);
     return false;
  }
  
  // look for a few common sentences
  
/*  example data
 $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47

Where:
     GGA          Global Positioning System Fix Data
     123519       Fix taken at 12:35:19 UTC
     4807.038,N   Latitude 48 deg 07.038' N
     01131.000,E  Longitude 11 deg 31.000' E
     1            Fix quality: 0 = invalid
                               1 = GPS fix (SPS)
                               2 = DGPS fix
                               3 = PPS fix
			       4 = Real Time Kinematic
			       5 = Float RTK
                               6 = estimated (dead reckoning) (2.3 feature)
			       7 = Manual input mode
			       8 = Simulation mode
     08           Number of satellites being tracked
     0.9          Horizontal dilution of position
     545.4,M      Altitude, Meters, above mean sea level
     46.9,M       Height of geoid (mean sea level) above WGS84
                      ellipsoid
     (empty field) time in seconds since last DGPS update
     (empty field) DGPS station ID number
     *47          the checksum data, always begins with *
*/

  if (strstr(nmea, "$GPGGA")) {
    // found GGA
    //Serial.println(nmea);
    char *p = nmea;
    // get time
    //p = strchr(p, ',')+1;
    //float timef = atof(p);
    //uint32_t time = timef;
    //hour = time / 10000;
    //minute = (time % 10000) / 100;
    //seconds = (time % 100);
    //milliseconds = fmod(timef, 1.0) * 1000;
    time = strchr(p, ',')+1;
    p = strchr(time, ',');  // find end of time
    *p++ = '\0';  // terminate time with Null
    
    latitude = p;  // latitude comes right after ','
    p = strchr(p, ',');  // find end of latitude
    *p++ = '\0';  // terminate latitude with Null
    //latitude = atof(latitude);

    // find latitude "N/S"
    if (*p == 'N') lat = 'N';
    else if (*p == 'S') lat = 'S';
    else if (*p == ',') lat = 0;
    else return false;

    // parse out longitude
    longitude = strchr(p, ',')+1;
    p = strchr(longitude, ',');  // find end of longitude
    *p++ = '\0';  // terminate longitude with Null
    //longitude = atof(longitude);

    // find longitude "E/W"
    if (*p == 'W') lon = 'W';
    else if (*p == 'E') lon = 'E';
    else if (*p == ',') lon = 0;
    else return false;

    fixquality = strchr(p, ',')+1;  // skip to fixquality
    p = strchr(fixquality, ',');  // find end of fixquality
    *p++ = '\0';
    //fixquality = atoi(p);

    satellites = p; //strchr(p, ',')+1;  // skip to satellites tracked
    p = strchr(satellites, ','); // find end of satellites
    *p++ = '\0';
    //satellites = atoi(p);

    p = strchr(p, ',')+1;  // skip horizontal dilution of precision
//    HDOP = atof(p);

    altitude = p; //strchr(p, ',')+1; // skip to altitude
    p = strchr(altitude, ','); // find end of altitude
    *p++ = '\0';  // terminate altitude with Null
    //faltitude = atof(altitude);
    
    // skip one field (M for altitude in meters)
    p = strchr(p, ',')+1;

    //p = strchr(p, ',')+1;
    geoidheight = atof(p);
    return true;
  }
  
/*  $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A

Where:
     RMC          Recommended Minimum sentence C
     123519       Fix taken at 12:35:19 UTC
     A            Status A=active or V=Void.
     4807.038,N   Latitude 48 deg 07.038' N
     01131.000,E  Longitude 11 deg 31.000' E
     022.4        Speed over the ground in knots
     084.4        Track angle in degrees True
     230394       Date - 23rd of March 1994
     003.1,W      Magnetic Variation
     *6A          The checksum data, always begins with *
*/

  if (strstr(nmea, "$GPRMC")) {
    // found RMC
    //Serial.println(nmea);
    char *p = nmea;

    // get time
    time = strchr(p, ',')+1;
    p = strchr(time, ',');
    *p++ = '\0';  // mark end of time string, advance to next field
    //float timef = atof(p);
    //uint32_t time = timef;
    //hour = time / 10000;
    //minute = (time % 10000) / 100;
    //seconds = (time % 100);

    //milliseconds = fmod(timef, 1.0) * 1000;
    
    // fix status
    
    if (*p == 'A') 
      fix = true;
    else if (*p == 'V')
      fix = false;
    else
      return false;

    // parse out latitude
    latitude = strchr(p, ',')+1; // advance to next field
    //latitude = p;  // latitude comes right after ','
    p = strchr(latitude, ',');  // find end of latitude
    *p++ = '\0';  // terminate latitude with Null
    //latitude = atof(latitude);

    // find latitude "N/S"
    if (*p == 'N') lat = 'N';
    else if (*p == 'S') lat = 'S';
    else if (*p == ',') lat = 0;
    else return false;

    // parse out longitude
    longitude = strchr(p, ',')+1;
    p = strchr(longitude, ',');  // find end of longitude
    *p++ = '\0';  // terminate longitude with Null
    //longitude = atof(longitude);

    // find longitude "E/W"
    if (*p == 'W') lon = 'W';
    else if (*p == 'E') lon = 'E';
    else if (*p == ',') lon = 0;
    else return false;

    // speed
    speed = strchr(p, ',')+1;
    //fspeed = atof(p);
    p = strchr(speed, ',');
    *p++ = '\0';    

    // angle
    angle = p;
    //angle = strchr(p, ',')+1;
    //fangle = atof(p);
    p = strchr(angle, ',');
    *p++ = '\0';

    date = p;  // find start of date
    p = strchr(date, ',');  // find end of date
    *p = '\0';   // terminate date with null
    
//    p = strchr(p, ',')+1;
//    uint32_t fulldate = atof(p);
//    day = fulldate / 10000;
//    month = (fulldate % 10000) / 100;
//    year = (fulldate % 100);

    // we dont parse the remaining, yet!
    return true;
  }

  // not a NMEA sentence we recognize (not GGA or RMC)
  return false;
}
