// Include necessary libraries
#include <Arduino.h>
#include <elapsedMillis.h>
#include <avr/pgmspace.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include "dtostrf.h"
#include <ArduinoJson.h>
#include <delay.h>

/**
 * How often should data be sent?
 */
#define UpdateInterval 0.5     // Update Every 1 mins


// LoRaWAN Config
// Device Address
devaddr_t DevAddr = 0x03ff0002;
//char deviceBuffer[16];

// Network Session Key DE-B1-DA-D2-FA-D3-BA-D4-FA-B5-FE-D6-AB-BA-DB-BA 00-00-a3-0b-00-1b-61-8c
// unsigned char NwkSkey[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char NwkSkey[16] = { 0xDE, 0xB1, 0xDA, 0xD2, 0xFA, 0xD3, 0xBA, 0xD4, 0xFA, 0xB5, 0xFE, 0xD6, 0xAB, 0xBA, 0xDB, 0xBA };

// Application Session Key
// unsigned char AppSkey[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char AppSkey[16] = { 0xDE, 0xB1, 0xDA, 0xD2, 0xFA, 0xD3, 0xBA, 0xD4, 0xFA, 0xB5, 0xFE, 0xD6, 0xAB, 0xBA, 0xDB, 0xBA };

/*****
 * --- Nothing needs to be changed below here ---
 *****/
// Feather M0 RFM9x pin mappings
lmic_pinmap pins = {
  .nss = 8,                // Internal connected
  .rxen = 0,               // Not used for RFM92/RFM95
  .txen = 0,               // Not used for RFM92/RFM95
  .rst = 4,                // Internal connected
  .dio = {3, 5, 6},    // Connect "i01" to "5"
                    // Connect "D2" to "6"
};

/********************************* Power IC Chip ************************/


enum CS5463_register_t {
  
  //Register Page 0
  CONFIG              =    0,     // Configuration
  CURRENT_DC_OFFSET       =    1,     // Current DC Offset
  CURRENT_GAIN          =    2,     // Current Gain
  VOLTAGE_DC_OFFSET       =    3,     // Voltage DC Offset
  VOLTAGE_GAIN          =    4,     // Voltage Gain
  CYCLE_COUNT           =    5,     // Number of A/D conversions used in one computation cycle (N)).
  PULSE_RATE_E          =    6,     // Sets the E1, E2 and E3 energy-to-frequency output pulse rate.
  CURRENT             =  7,     // Instantaneous Current
  VOLTAGE             =    8,     // Instantaneous Voltage
  POWER             =  9,     // Instantaneous Power
  POWER_ACTIVE          =    10,  // Active (Real) Power
  CURRENT_RMS           =    11,  // RMS Current
  VOLTAGE_RMS           =    12,  // RMS Voltage
  EPSILON             =  13,  // Ratio of line frequency to output word rate (OWR)
  POWER_OFFSET          =    14,  // Power Offset
  STATUS              =    15,  // Status
  CURRENT_AC_OFFSET       =    16,  // Current AC (RMS) Offset
  VOLTAGE_AC_OFFSET       =    17,  // Voltage AC (RMS) Offset
  MODE              =    18,  // Operation Mode
  TEMPERATURE           =    19,  // Temperature
  POWER_REACTIVE_AVERAGE      =    20,  // Average Reactive Power
  POWER_REACTIVE          =    21,  // Instantaneous Reactive Power
  CURRENT_PEAK          =    22,  // Peak Current
  VOLTAGE_PEAK          =    23,  // Peak Voltage
  POWER_REACTIVE_TRIANGLE     =    24,  // Reactive Power calculated from Power Triangle
  POWERFACTOR           =    25,  // Power Factor
  MASK_INTERUPT         =    26,  // Interrupt Mask
  POWER_APPARENT          =    27,  // Apparent Power
  CONTROL             =    28,  // Control
  POWER_ACTIVE_HARMONIC     =    29,  // Harmonic Active Power
  POWER_ACTIVE_FUNDAMENTAL    =    30,  // Fundamental Active Power
  POWER_REACTIVE_FUNDAMENTAL    =    31,  // Fundamental Reactive Power / Page
  
    
  //Register Page 1
  PULSE_WIDTH           = 0,
  LOAD_MIN            = 1,
  TEMPERATURE_GAIN        = 2,
  TEMPERATURE_OFFSET        = 3,
  
  //Register Page 3
  VOLTAGE_SAG_DURATION      = 6,
  VOLTAGE_SAG_LEVEL       = 7,
  CURRENT_SAG_DURATION      = 10,
  CURRENT_SAG_LEVEL       = 11,
  // COMMANDS
  Read              =  0,
  Write             =  64,
  SYNC0             =  254, //SYNC 0 Command: Last byte of a serial port re-initialization sequence
  SYNC1             =  255, //SYNC 1 Command: Used during reads and serial port initialization.
  SELECT_PAGE           =  0x1F,
  START_CONTINOUS         =  0xE8,
  VOLTAGE_DC_OFFSET_CALIBRATION   =   0b11011101,
  
}
;
// FEATHER VARIABLES
const int CS = 15;
const int RESET_PIN =16;
const int MODE_PIN = 14;
float Full_Scale_V = 237.71;   //full scale Value
byte H_Byte;
byte M_Byte;
byte L_Byte;
int READINGS [3][32];
int v; // voltage reading in sendVRms();
int i; // current reading in sendVRms();

/*********************************** END *********************************/

  // Memory pool for JSON object tree.
  //
  // Inside the brackets, 200 is the size of the pool in bytes.
  // Don't forget to change this value to match your JSON document.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonBuffer<64> jsonBuffer;

 // Create the root of the object tree.
  //
  // It's a reference to the JsonObject, the actual bytes are inside the
  // JsonBuffer with all the other nodes of the object tree.
  // Memory is freed when jsonBuffer goes out of scope.
  JsonObject& root = jsonBuffer.createObject();


// Track if the current message has finished sending
bool dataSent = false;

/**
 * Device Start Up
 */
void setup() {
    // Startup delay for Serial interface to be ready
    Serial.begin(115200);

    randomSeed(analogRead(0));
    pinMode(13, OUTPUT);
     
    delay(3000);

    // Debug message
    Serial.println("Starting...");

    // Some sensors require a delay on startup
    elapsedMillis sinceStart = 0;
    int sensorReady = 0;


    // Setup LoRaWAN state
    initLoRaWAN();

    // Wait for all the sensors to be ready
    if (sensorReady > sinceStart) {
        // A sensor still needs some time
        delay(max(0, (int)(sensorReady - sinceStart)));
    }

    // Shutdown the radio
    os_radio(RADIO_RST);

    // Debug message
    Serial.println("Startup Complete");

    /******************************** Power IC Chip **********************/


    pinMode(CS, OUTPUT);      //initalize the chip select pin;
    pinMode(RESET_PIN, OUTPUT); //initalize the RESET pin;
    pinMode(MODE_PIN, OUTPUT);  //initialize the MODE pin;
    digitalWrite(CS, HIGH);
    digitalWrite(RESET_PIN, HIGH);
    digitalWrite(MODE_PIN, LOW);
    delay (100);
    //Create a serial connection to display the data on the terminal.   
  Serial.begin(9600);
  //start the SPI library;
  SPI.begin();
  SPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE0));    //Set SPI to 1MHz, MSBFIRST and SPI_MODE0
  //Run a Hardware Reset of the CS5463
  digitalWrite(CS, LOW);
  
  digitalWrite(RESET_PIN, LOW);
    delay (100);
  digitalWrite(RESET_PIN, HIGH);
    delay (100);
//Run a Soft Reset of the CS5463    
  //Sync commands 
   SPI.transfer(SYNC1);    
   SPI.transfer(SYNC1);     
   SPI.transfer(SYNC1);    
   SPI.transfer(SYNC0); 
   
   digitalWrite(CS, HIGH);
  
  delay (100);
  //Set Config register
  digitalWrite(CS, LOW);          //Chip select to low to initialise comms with CS5463 
  SPI.transfer( Write | (CONFIG<<1) );
  SPI.transfer(0x00);           //3 bytes of data to set 24bits of config register        
    SPI.transfer(0x00);     
    SPI.transfer(0x01);           // Set K value to 1 (clock devider for measuring cycles)
    digitalWrite(CS, HIGH);         //Chip select to HIGH to disable comms with CS5463  
  //Set Mask register
  digitalWrite(CS, LOW);          
  SPI.transfer( Write | (MASK_INTERUPT<<1) );
  SPI.transfer(0x00);           //3 bytes of data to set 24bits of mask register (Set for no interrupts)  
  SPI.transfer(0x00);     
  SPI.transfer(0x00); 
  digitalWrite(CS, HIGH);         
  
  //Set Mode register
  digitalWrite(CS, LOW);          
  SPI.transfer( Write | (MODE<<1) );
  SPI.transfer(0x00);           //Sets High pass filters on Voltage and Current lines, sets automatic line frequency measurements     
  SPI.transfer(0x00);     
  SPI.transfer(0x61); 
  digitalWrite(CS, HIGH); 
  
  digitalWrite(CS, LOW);          
  SPI.transfer(START_CONTINOUS);
  digitalWrite(CS, HIGH);
  
  // Set offsets to default value (no offset)
  digitalWrite(CS, LOW);
   
  SPI.transfer( Write | (CURRENT_DC_OFFSET << 1 ));     
  SPI.transfer(0x00); 
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer( Write | (VOLTAGE_DC_OFFSET  << 1 ));    
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer(0x00); 
  SPI.transfer( Write | (CURRENT_GAIN   << 1 ));    
  SPI.transfer(0x40);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer( Write | (VOLTAGE_GAIN  << 1 ));    
  SPI.transfer(0x40);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  digitalWrite(CS, HIGH);

    /********************************** END ********************************/
}

void initLoRaWAN() {
    // LMIC init
    os_init();

    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // by joining the network, precomputed session parameters are be provided.
    LMIC_setSession(0x1, DevAddr, (uint8_t*)NwkSkey, (uint8_t*)AppSkey);

    // Enabled data rate adaptation
    LMIC_setAdrMode(1);

    // Enable link check validation
    LMIC_setLinkCheckMode(0);

    // Set data rate and transmit power
    LMIC_setDrTxpow(DR_SF9, 14);
}

void loop() {
    // Start timing how long since starting to send data
    elapsedMillis sinceWake = 0;

    // Debug message
    Serial.println("\nBeginning to send data");

    sendVRms();
    delay(1000);

    // Shutdown the radio
    os_radio(RADIO_RST);

    // Output sleep time
    int sleepSeconds = 60 * UpdateInterval;
    sleepSeconds -= sinceWake/1000;
    Serial.print("Sleeping for ");
    Serial.print(sleepSeconds);
    Serial.println(" seconds");
    delay(500); // time for Serial send buffer to clear

    // Actually go to sleep
    signed long sleep = 60000 * UpdateInterval;
    sleep -= sinceWake;
    delay(constrain(sleep, 10000, 60000 * UpdateInterval));
}

/**
 * Send a message with the receptacle voltage
 */
void sendVRms() {
    // Ensure there is not a current TX/RX job running
    if (LMIC.opmode & (1 << 7)) {
        // Something already in the queque
        return;
    }

  /************************************** Power IC Chip **********************/

  // Read Voltage(v) and Current(i) from IC Chip
  digitalWrite(CS, LOW);

  // Read Voltage
  SPI.transfer( Read | (VOLTAGE<<1) );
  v = SPI.transfer(SYNC1);
  Serial.print("VOLTAGE: ");
  Serial.println(v);
  delayMicroseconds(10);

  // Read Current
  SPI.transfer( Read | (CURRENT<<1) );
  i = SPI.transfer(SYNC1);
  Serial.print("CURRENT: ");
  Serial.println(i);
  delayMicroseconds(10);

  digitalWrite(CS, HIGH);

  /*************************************** END ********************************/

    // Convert to a string
//    char buffer[8];
char packet[64];
root["v"] = v;
root["i"] = i;
root["d"] = "03ff0002";
root.printTo(packet);  

    // Put together the data to send
//    char packet[20] = "{\"v\":";
//    itoa(random(114, 126),buffer,10); //(integer, buffer, base)
//    strcat(packet, buffer);
//    strcat(packet, ", \"i\":");
//    itoa(random(0, 10),buffer,10); //(integer, buffer, base)
//    strcat(packet, buffer); 
//    strcat(packet, ", \"d\":1");
//    strcat(packet, deviceBuffer); // 0x03ff0001
//    strcat(packet, "}");

    // Debug message.
    Serial.print("  seqno ");
    Serial.print(LMIC.seqnoUp);
    Serial.print(": ");
    Serial.println(packet);

    // Add to the queque
    dataSent = false;
    uint8_t lmic_packet[20];
    strcpy((char *)lmic_packet, packet);

    digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
    
    LMIC_setTxData2(1, lmic_packet, strlen((char *)lmic_packet), 0);

    // Wait for the data to send or timeout after 15s
    elapsedMillis sinceSend = 0;
    while (!dataSent && sinceSend < 15000) {
        os_runloop_once();
        delay(1);
    }
    os_runloop_once();

    digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
}




/******************************************************** Downlink - Receive Code ********************/

if (ev == EV_TXCOMPLETE) {
    dataSent = true;
   
   if (LMIC.dataLen) {
        // data received in rx slot after tx
        Serial.print(F("Received "));
        Serial.print(LMIC.dataLen);
        Serial.print(F(" bytes of payload: 0x"));
        for (int i = 0; i < LMIC.dataLen; i++) {
          if (LMIC.frame[LMIC.dataBeg + i] < 0x10) {
            Serial.print(F("0"));
        }
        Serial.print(LMIC.frame[LMIC.dataBeg + i], HEX);
    }
    Serial.println();
/********************************************************** END *****************************************/






// ----------------------------------------------------------------------------
// LMIC CALLBACKS
// ----------------------------------------------------------------------------

// LoRaWAN Application identifier (AppEUI)
static const u1_t AppEui[8] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/**
 * Call back to get the AppEUI
 */
void os_getArtEui (u1_t* buf) {
    memcpy(buf, AppEui, 8);
}

/**
 * Call back to get the Network Session Key
 */
void os_getDevKey (u1_t* buf) {
    memcpy(buf, NwkSkey, 16);
}

/**
 * Callback after a LMIC event
 */
void onEvent (ev_t ev) {
    if (ev == EV_TXCOMPLETE) {
        dataSent = true;
    }
    if (ev == EV_LINK_DEAD) {
        initLoRaWAN();
    }
}
