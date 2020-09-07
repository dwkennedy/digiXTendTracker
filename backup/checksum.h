
#ifndef _CHECKSUM_H
#define _CHECKSUM_H


  // utility functions to parse and check hexadecimal checksum
  boolean verifyChecksum(char *nmea);
  uint8_t parseHex(char c);
  uint8_t calculateChecksum(char *nmea, uint8_t length);
 

#endif
