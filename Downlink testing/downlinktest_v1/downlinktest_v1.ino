// Include necessary libraries
#include <Arduino.h>
#include <elapsedMillis.h>
#include <avr/pgmspace.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

/**
 * How often should data be sent?
 */
#define UpdateInterval 0.5     // Update Every 1 mins

// LoRaWAN Config
// Device Address
devaddr_t DevAddr = 0x03ff0002;

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

// Track if the current message has finished sending
bool dataSent = false;


// Random number generator (Setting the random number generator outside of sendVrms();)
int randomNum;



/**
 * Device Start Up
 */
void setup() {
    // Startup delay for Serial interface to be ready
    Serial.begin(115200);

    pinMode(13, OUTPUT);
     
    delay(3000);

    // Debug message
    Serial.println("Starting...");

    // Setup LoRaWAN state
    initLoRaWAN();

    // Shutdown the radio
    os_radio(RADIO_RST);

    // Debug message
    Serial.println("Startup Complete");
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
    Serial.print("\nBeginning to send data with seqnoUp ==  ");
    Serial.println(LMIC.seqnoUp);

    sendVRms();


    for(int i = 0; i < 64; i++)
    {
      Serial.print(LMIC.frame[i],HEX);
      Serial.print(" ");
    }
    Serial.println();
    if(LMIC.dataLen >= 1) {  
      LMIC_clrTxData ();        
      Serial.println("*** LMIC_clrTxData");
 // set LED state if exactly one byte is received
 Serial.println("***************************************** RX DATA ********************         ");
 //Serial.println(LMIC.frame[LMIC.dataBeg]);
 //LMIC.reportEvent(EV_RXCOMPLETE);
 //LMIC.seqnoUp = LMIC.seqnoUp - 1;
    }

    //LMIC.seqnoUp = LMIC.seqnoUp - 1;
    

    /******************************* DOWNLINK EVENT ******************************/

    
    /********************************** DOWNLINK END *****************************/


    
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
        // Something already in the queue
        return;
    }

    // Convert to a string
    char buffer[4];
    //itoa(random(114, 126),buffer,10); //(integer, buffer, base)
    itoa(randomNum, buffer, 10);
    

    // Put together the data to send
    char packet[20] = "V: ";
    strcat(packet, buffer);
    
    // Debug message
    Serial.print("  seqno ");
    Serial.print(LMIC.seqnoUp);
    Serial.print(": ");
    Serial.println(packet);

    // Add to the queque
    dataSent = false;
    uint8_t lmic_packet[20];
    strcpy((char *)lmic_packet, packet);

    //digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
    
    LMIC_setTxData2(1, lmic_packet, strlen((char *)lmic_packet), 0);

    // Wait for the data to send or timeout after 15s
    elapsedMillis sinceSend = 0;
    while (!dataSent && sinceSend < 15000) {
        os_runloop_once();
        delay(1);
    }
    os_runloop_once();

    //digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
}



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
        //Serial.println("************TX COMPLETE EVENT************");
        Serial.print("Data length: ");Serial.println(LMIC.dataLen);
        Serial.print("Data frame: "); Serial.println(LMIC.frame[0]);
        Serial.print("Data Beg: ");   Serial.println(LMIC.dataBeg);
        //Serial.println("************** DEBUG END *****************");

        randomNum = random(114, 126);
        
        //if(LMIC.dataLen >= 1) {          
 // set LED state if exactly one byte is received
 //Serial.println("***************************************** RX DATA ********************         ");
 //Serial.println(LMIC.frame[LMIC.dataBeg]);
 //LMIC.reportEvent(EV_RXCOMPLETE);
   // }
}
        
    
    if (ev == EV_LINK_DEAD) {
        initLoRaWAN();
    }

    //if(ev == EV_RXCOMPLETE){
     // Serial.println("********************************RX EVENT*********************");
  //log frame data
 //debug_buf(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
// if(LMIC.dataLen == 1) {
  //set LED state if exactly one byte is received
 //Serial.println(LMIC.frame[LMIC.dataBeg]);
 //}
    //}
}
