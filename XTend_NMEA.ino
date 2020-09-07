/* parses NMEA-like commands that we send via the XTend radio */

#include "XTend_NMEA.h"

    XTend_NMEA::XTend_NMEA(void) {}

    boolean XTend_NMEA::cutdown(void) {
  
      if (cutdownFlag) {
      cutdownFlag = false;
      return true;
        } else {
      return false;
      }
    }

    boolean XTend_NMEA::parseNMEA(char *nmea) {
  
    // do checksum check

    // first look if we even have one
  
      if (!verifyChecksum(nmea)) {
        // bad checksum :(
        return false;
      }

      // look for a few common sentences
      if (strstr(nmea, "$PCUT")) {
        // found PCUT
        char *p = nmea;
   
        p = strchr(p, ',')+1;
    
        if (*p == '1') {
          cutdownFlag = true;
          return true;
        } 
        return false;  // invalid character X in $PCUT,X
      }

      // command not recognized
      return false;
  }


