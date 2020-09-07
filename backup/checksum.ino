// utility functions to compute/verify NMEA XOR checksums

boolean verifyChecksum(char *nmea) {
  // check that the checksum as transmitted equals the calculated value

  // first look if we even have one
  if (nmea[strlen(nmea)-4] == '*') {
    uint8_t sum = parseHex(nmea[strlen(nmea)-3]) * 16;  // upper 4 bits of transmitted checksum
    sum += parseHex(nmea[strlen(nmea)-2]);               // lower 4 bits of transmitted checksum
    
    // check transmitted checksum vs. calculated checksum
    if (sum != calculateChecksum(nmea, strlen(nmea)-4 )) {
      // bad checksum :(
      return false;
    } else {
      // checksum checks out
      return true;
    }
  }
  Serial.println("Didn't see '*' in sentence");
  return false;  // '*' not found
}

// read a Hex value and return the decimal equivalent
uint8_t parseHex(char c) {
    if (c < '0')
      return 0;
    if (c <= '9')
      return c - '0';
    if (c < 'A')
       return 0;
    if (c <= 'F')
       return (c - 'A')+10;
}


uint8_t calculateChecksum(char *nmea, uint8_t length) {
  // calculates the checksum value for a referenced string, skips the first character!
  uint8_t sum = 0;
  
  for (uint8_t i=1; i < length; i++) {
    sum ^= nmea[i];
  }
  
  return (sum);
}

