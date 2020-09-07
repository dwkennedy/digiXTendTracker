
// GPS telemetry and SD recording with the digi XTend 915 MHz data radio
//
//  Record GPS & other data on SD card
//  Transmit reduced GPS and other data to ground
//  Respond to cut-down command from ground
//  TODO:  add recording of 4800 baud serial data from optical receiver 
//
// This code is intended for use with Arduino Leonardo and other ATmega32U4-based Arduinos

// decoded debug output on Serial
#define DEBUG true

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
//#define GPSECHO  false
#define GPSECHO true

// Set XTENDECHO to 'false' to turn off echoing the commands received from the ground station.
// Set to 'true' if you want to debug and listen to the commands
//#define XTENDECHO false
#define XTENDECHO true

// unique letter id, A-Z/0-9 identifying tracker, along with our proprietary NMEA-like data type

#define UNIT_ID "$PTRK,F,"

#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <avr/sleep.h>
#include "NMEA_serial.h"
#include "GPS_NMEA.h"
#include "XTend_NMEA.h"

//   Connect the GPS TX (transmit) pin to Digital 8
//   Connect the GPS RX (receive) pin to Digital 7
//   Connect the GPS PPS pin to Digital 2
// 
//   Connect the XTend TX (transmit) pin to Arduino RX1 (Digital 0)
//   Connect the XTend RX (receive) pin to matching TX1 (Digital 1)
//
//   Connect the optical RX (receive) pin to Digital 9


// set up serial port for GPS, set up to read one sentence at a time (NMEA_serial)
SoftwareSerial gpsSerial(8, 7);
NMEA_serial gpsNMEA(&gpsSerial);
GPS_NMEA GPS;

// If using hardware serial, comment
// out the above two lines and enable these two lines instead:
//NMEA_serial gpsNMEA(&Serial1);
//HardwareSerial gpsSerial = Serial1;

// Handle sentences to/from the digi XTend.  UART pins 0/1
HardwareSerial radioSerial = Serial1;
NMEA_serial xtendNMEA(&Serial1);
XTend_NMEA XTend;


// analog pins
const int batteryPin = 5; // v3 hardware is 0, and voltage divider is 3:1!!!
const int boardTempPin = 1;
const int externalTempPin = 2;
const int rssiPin = 3;

const int cutdownPin = 6;  // activate cutter with 15-60 sec HIGH
const int shutdownPin = 5; // active LOW; leave HIGH for operation
const int sleepPin = 4;  // optional, to enable pin sleep mode controll
const int gpsPPS = 2; // pulse per second, only when valid fix
const int ledPin = 13; // blinky error light
const int chipSelect = 10; // SD card SPI chip select

const char initMessage[] PROGMEM = {"DOUG TRACKER V1.1"};
const char PMTK_SET_NMEA_OUTPUT_RMCGGA[] PROGMEM = {"$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"};
const char PMTK_SET_NMEA_UPDATE_1HZ[] PROGMEM = {"$PMTK220,1000*1F"};

boolean blink = false;  // blinky light
boolean pulseDetected = false;  // semaphone for ISR
boolean cutdownOn = false;  // semaphone for cutter
int errorCount = 0; // keep track of how many NMEA cksum errors occur

char outputBuffer[120]; // space to hold output message string we're building
char batteryVoltageString[10]; // text equiv. of battery voltage
char checksumString[5]; // ascii checksum

File logfile;  // file to hold all the fascinating data on the SD card

// interrupt service routine to monitor PPS signal and set flag
void pulseISR() {
  pulseDetected = true;
  //Serial.println("PPS");
}

void setup()  
{
  // configure cutdown pin as write, and turn it off
  pinMode(cutdownPin, OUTPUT);
  digitalWrite(cutdownPin, false);
  cutdownOn = false;
  
  // set up Arduino Leo USB serial for debugging output
  Serial.begin(115200);
  delay(5000);
  Serial.println(initMessage);
  
  // initialize XTend shutdown pin
  pinMode(shutdownPin, OUTPUT);
  //digitalWrite(shutdownPin, HIGH);
  digitalWrite(shutdownPin, LOW);
    
  // SD card SPI select 
  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect, HIGH);
  // see if the card is present and can be initialized:
  Serial.println("before SD.begin");
  //if (!SD.begin(chipSelect, 11, 12, 13)) {
  if (!SD.begin(chipSelect)) {      // if you're using an UNO or some such newfangled device, you can use this line instead
    Serial.println("Card init. failed!");
    //error(2);
  }
 Serial.println("after SD.begin");
 
 // set filename and open log file for writing
 char filename[15];
  strcpy(filename, "GPSLOG00.TXT");
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i/10;
    filename[7] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    Serial.print("Trying ");
    Serial.println(filename);
    if (! SD.exists(filename)) {
      break;
    }
  }

  Serial.print(F("Opening "));
  Serial.println(filename);
  logfile = SD.open(filename, FILE_WRITE);
  if( ! logfile ) {
    Serial.print(F("Couldn't create ")); Serial.println(filename);
    //error(3);
  }
  Serial.print(F("Writing to ")); Serial.println(filename);
  
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS
  gpsNMEA.begin(9600);
  
  // turn on RMC (recommended minimum) and GGA (fix data) including altitude
  gpsNMEA.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time
  
  // Set the update rate
  gpsNMEA.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // Request updates on antenna status, comment out to keep quiet
  //gpsNMEA.sendCommand(PGCMD_ANTENNA);

  Serial.println(F("GPS initialized..."));
  
  // initialize Xtend radio here (deassert shutdown pin, set power level, RF settings, etc)
  // change these to xtendNMEA commands instead
  //xtendNMEA.begin(9600);
  Serial1.begin(9600);
  Serial1.println(F("HELLO"));
  Serial.println(F("Send hello to rxvr..."));
  // deassert shutdown pin to turn on radio
  //pinMode(shutdownPin, OUTPUT);
  digitalWrite(shutdownPin, HIGH);
  //digitalWrite(shutdownPin, LOW);
  delay(2250);  // wait for radio to wake up + BT guard time
  Serial.println(F("sending +++ to XTend..."));
  Serial1.print(F("+++"));
  //xtendNMEA.sendCommand("+++");
  readTimeout(1250);
  Serial1.print(F("+++"));
  //xtendNMEA.sendCommand("+++");
  readTimeout(1250);
  Serial1.print(F("+++"));
  //xtendNMEA.sendCommand("+++");
  readTimeout(1250);
  //delay(1250);  // AT guard time after
  Serial.println(F("entered command mode"));

  // *******************************************************
  //    really need to read OK response, and repeat command several times,
  //    then annoyingly flash LED to indicate bad GPS
  
  // write library routine to enter command mode
  // write routine to send command, look for OK, possibly timeout, then return condition code
  // write routines to generate AT statements based on arguments passed to routine
  //   a.k.a.  radio.enterCommandMode();
  //           radio.sendCommand("ATPL3");
  //           optionally: radio.setPowerLevel(3);  radio.setRFDataRate(9600);
  //
  // but for now we just send these strings and hope for the best :/
  
  Serial.print(F("sending ATPL4\r\n")); // set 10mw power
  //Serial1.print(F("ATPL1\r")); // set 10mw power
  Serial1.print(F("ATPL4\r")); // set 1000mw power
  // 4 = 1w, 3=500mw, 2=100mw, 1=10mw, 0=1mw

  readTimeout(2000);
  // delay(1000);  // wait for "OK"

  Serial.print(F("sending ATBR0\r\n")); // set 9600 baud RF data rate
  Serial1.print(F("ATBR0\r")); // set 9600 baud RF data rate

  readTimeout(2000);
  //delay(1000);
  
  Serial.print(F("sending ATSM7\r\n"));  // 8 second cyclic sleep mode
  Serial1.print(F("ATSM7\r"));  // 8 second cyclic sleep mode
  //delay(1000);
  readTimeout(2000);
  
  Serial.print(F("sending ATST30\r\n"));  // sleep after 30*100ms = 3 seconds of idle
  Serial1.print(F("ATST30\r"));  // sleep after 30*100ms = 3 seconds of idle
  readTimeout(2000);
  //delay(1000);
  
  Serial.print(F("sending ATCN\r\n"));  // exit command mode
  Serial1.print(F("ATCN\r"));  // exit command mode
  readTimeout(2000);
  //delay(1000);
  
  Serial.println(F("XTend initialized..."));
  
  // delay(1000);
  // Ask GPS for firmware version
  //gpsSerial.println(PMTK_Q_RELEASE);
  
  // Interrupt service routine to look for rising edge of PPS signal on int_1 (pin 2)
  attachInterrupt(1,pulseISR,RISING);

}

// how often to write telemetry to file & radio
// fileTimer is only used for backup to PPS signal
// cutTimer controls duration of the cut-through board signal
uint32_t fileTimer = millis();
uint32_t radioTimer = millis();
uint32_t cutTimer = 2^31;  

void loop()                     // run over and over again
{
  char c =  gpsNMEA.read();
  // if you want to debug, this is a good time to do it!
  if ((c) && (GPSECHO))
    Serial.write(c); 
    
  c = xtendNMEA.read();
  if ((c) && (XTENDECHO))
    Serial.write(c);

  if (xtendNMEA.newNMEAreceived()) {
    XTend.parseNMEA(xtendNMEA.lastNMEA());    
    
    if (XTend.cutdown()) {
      cutTimer = millis();
      digitalWrite(cutdownPin, true);
      cutdownOn = true;
      //Serial.println("cutter on");  // USB debugging message
      xtendNMEA.sendCommand("$PCUT,1");  // verify cutdown to downlink; need radio.println function; format as $XXXXX message
    }
  }
  
  if ( (cutdownOn == true) && (millis() - cutTimer) > 30000) { // if cutter has been on long enough, turn it off
    // It is slow to check the time, and set (an already off) digital output off, so check the semaphone
    digitalWrite(cutdownPin, false);
    cutdownOn = false;   
  }
    
  
  // if a sentence is received, we can check the checksum, parse it...
  if (gpsNMEA.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences! 
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    //Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false
  
    if (!GPS.parseNMEA(gpsNMEA.lastNMEA())) {   // this also sets the newNMEAreceived() flag to false
      errorCount++;
      if (DEBUG) {
        Serial.print(F("Chksum error: "));
        Serial.println(errorCount);
      }
      return;  // we can fail to parse a sentence in which case we should just wait for another
    }
  }
  
  // if millis() or timer wraps around, we'll just reset it; only happens every 50 days or so, 32 bit value
  //if (fileTimer > millis())  fileTimer = millis();

  // at every PPS, or at least every 3000 ms, print out the current stats
  if ( pulseDetected || ((millis() - fileTimer > 3000)) ) {
    
    fileTimer = millis(); // reset the timer
    blink = !blink;
    //digitalWrite(ledPin, blink);

    // analog input is Vbatt/2 (2:1 voltage divider), 3:1 on v. 3 board and newer
    //   convert from digital units to batt voltage        
    //float batteryVoltage = (float)analogRead(batteryPin)*0.009765625;
    
    // straight analog-to-digital value pretty close to battery voltage * 100!
    uint16_t batteryVoltage = analogRead(batteryPin);
    utoa(batteryVoltage, batteryVoltageString, 10); 
    
    // read/convert temperature and other sensors here
        
    if (DEBUG) {
   
      Serial.print("\nPulse: ");
      Serial.println(pulseDetected);  
      Serial.print("Time: ");
      Serial.print(GPS.time);
//    Serial.print(GPS.hour, DEC); Serial.print(':');
//    Serial.print(GPS.minute, DEC); Serial.print(':');
    // sometimes atof doesn't work as expected, leaving unreal & messy milliseconds part
//    Serial.print((int)((float)GPS.seconds + ((float)GPS.milliseconds/1000) + 0.49999), DEC); // Serial.print('.');
    //Serial.println(GPS.milliseconds);  
      Serial.print("\nDate: ");
      Serial.println(GPS.date);
//    Serial.print(GPS.month, DEC); Serial.print('/');
//    Serial.print(GPS.day, DEC); Serial.print("/20");
//    Serial.println(GPS.year, DEC);
      Serial.print("Fix: "); Serial.print((int)GPS.fix);
      Serial.print(" quality: "); Serial.println(GPS.fixquality); 

      if (GPS.fix) {
        Serial.print("Location: ");
        Serial.print(GPS.latitude);
        Serial.print(GPS.lat);
        Serial.print(", ");
        Serial.print(GPS.longitude);
        Serial.println(GPS.lon);      

        Serial.print("Speed (knots): "); Serial.println(GPS.speed);
        Serial.print("Angle: "); Serial.println(GPS.angle);
        Serial.print("Altitude: "); Serial.println(GPS.altitude);
        Serial.print("Geoidal Separation: "); Serial.println(GPS.geoidheight);
        Serial.print("Satellites: "); Serial.println(GPS.satellites);
      }

      Serial.print("Battery: ");
      // print two decimal places (default)
      Serial.print(batteryVoltageString);
      Serial.println("0 mV\n");
  }  
  
/* NSSL balloon telemetry protocol:
$PTRK,unit_id,HHMMSS,DDMMYY,latitude(NMEA),longitude(NMEA),altitude(meters MSL),
                     speed(knots),bearing(degrees true),satellites(from GPGGA),battery voltage(X.X),
                     internal_temp(C),external_temp(C),RSSI(%),*CHECKSUM\n

unit_id: string identifying unit (serial number)
HHMMSS:  hour/min/sec, UTC, 24hr clock
DDMMYY: day month year
latitude: DDMM.mmmmm, north is positive (35.xxx)
longitude: DDDMM.mmmmm, degrees and decimal minutes, west is positive (e.g. oklahoma is -97.xxx)
altitude:  meters, MSL
speed: knots over ground
bearing: degrees from true north
satellites: how many satellite are being tracked
battery voltage: in volts, nominally "9.0"
internal temp:  board temperature in C, room temp is about "25.0"
external temp:  optional external sensor, in deg. C
RSSI, received signal strength percentage, 0-100
..
other optional things, comma seperated
..
blank field,
*CHECKSUM
*/
    //strcpy(outputBuffer, "$PTRK,");
    strcpy(outputBuffer, UNIT_ID);
    //strcat(outputBuffer, ",");  
    strcat(outputBuffer, GPS.time);
    strcat(outputBuffer, ",");
    strcat(outputBuffer, GPS.date);
    strcat(outputBuffer, ",");
    strcat(outputBuffer, GPS.latitude);
    strcat(outputBuffer, ",");
    strcat(outputBuffer, GPS.longitude);
    strcat(outputBuffer, ",");
    strcat(outputBuffer, GPS.altitude);
    strcat(outputBuffer, ",");
    strcat(outputBuffer, GPS.speed);
    strcat(outputBuffer, ",");
    strcat(outputBuffer, GPS.angle);
    strcat(outputBuffer, ",");
    strcat(outputBuffer, GPS.satellites);
    strcat(outputBuffer, ",");
    strcat(outputBuffer, batteryVoltageString);
    //strcat(outputBuffer, ",0,0,0");
    
    uint16_t cksum = 0x100 + calculateChecksum(outputBuffer,strlen(outputBuffer));
    utoa(cksum, checksumString, 16);
    *checksumString = '*';  // overright leading '1' with * character
    strcat(outputBuffer, checksumString);
    
    //Serial.print(outputBuffer);
    
    // append string to log file
    
    uint8_t stringsize = strlen(outputBuffer);
    if (stringsize != logfile.write((uint8_t *)outputBuffer, stringsize))    //write the string to the SD file
      //error(4);
    //if (strstr(stringptr, "RMC"))
    //  logfile.flush();
    //Serial.println();

    //logfile.println(stringptr);
    
    // every X milliseconds, dump string to radio and flush SD card
    // if millis() or timer wraps around, we'll just reset it
    //if (radioTimer > millis())  radioTimer = millis();

    // approximately every 10 seconds or so, print out the current stats and flush
    if (millis() - radioTimer > 100) { 
      radioTimer = millis(); // reset the timer
      //digitalWrite(shutdownPin, HIGH); // turn on radio
      Serial1.println(outputBuffer);
      //digitalWrite(shutdownPin, LOW); // turn off radio
      logfile.flush();
      Serial.println(outputBuffer);
    }
    
    pulseDetected = 0;  // reset the pulse detector 
    
  }
}


// blink out an error code
void error(uint8_t errno) {

/*  if (SD.errorCode()) {
    Serial.print("SD error: ");
    Serial.print(logfile.errorCode(), HEX);
    Serial.print(',');
    Serial.println(logfile.errorData(), HEX);
  } */
  
  //while(1) {
    uint8_t i;
    for (i=0; i<errno; i++) {
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);
      delay(100);
    }
    for (i=errno; i<10; i++) {
      delay(200);
    }
  //}
}

void readTimeout(unsigned long timeout) {
  unsigned long foo;

  foo=millis();
  while ((millis()-foo)<timeout) {
    if(Serial1.available()) {
      Serial.write(Serial1.read());
    }
  }
  Serial.write("\r\n");
}
