
#ifndef _CHECKSUM_H
#define _CHECKSUM_H


  // utility functions to parse and check hexadecimal checksum
  uint8_t parseHex(char c);
  boolean calculateChecksum(char *nmea, uint8_t length);
  boolean verifyChecksum(char *nmea);

#endif
