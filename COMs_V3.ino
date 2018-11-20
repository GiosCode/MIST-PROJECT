/* Wiring Notes
 * ARDUINO UNO:
 * MISO(12) <- SDO(6) : CS5463
 * MOSI(11) -> SDI(23)
 * SCK(13) -> SCLK(5)
 *
 * FEATHER M0:
 *(SCK) -> SCLK(5) : CS5463
 *(MOS) -> SDI(23)
 *(MIS) <- SDO(6)
 */

/*
 *   BREADBOARD Wiring
 *   LHS:
 *   A -> MODE (4)
 *   B -> CS   (3)
 *   C -> SDO (12)
 *   D -> SCLK (13)
 *
 *   RHS:
 *   G -> SDI (11)
 *   H -> INT TBD
 *   I -> RESET (2)
 *
 */

/*
   FEATHER Wiring
   MIS <- SDO(6)
   MOS -> SDI(23)
   SCK -> SCLK(5)
   MODE (A1/15) -> MODE SELECT(8)
   RESET (A2/16) -> RESET (19)
   SS (A3/17) -> CS(7)
 */

#include <SPI.h>
#include <util/delay.h>

//POWER MONITOR ADDRESSES
#define WRITE         64
#define READ          0
#define CONFIG_ADD    0x00
#define MODE_ADD      18
#define STATUS_ADD    15
#define TEMP_ADD      19
#define VOLT_ADD 8
#define SYNC1         0xFF
#define SYNC0         0XFE
#define INSTANTVOLT 8
#define V_GAIN 4
#define I_GAIN 2
#define I_ADD 7
#define POW 9
//uC PINS///////////////////////////////////////////
/* MUST COMMENT OUT ONE*/
//Arduino Uno
//
#define MODE          4
#define RESET         2
#define SS            3
//
//FEATHER
/*
 #define MODE 15
 #define RESET 16
 #define SS 17
 //*/
////////////////////////////////////////////////////

//Variables
byte H_BYTE;
byte M_BYTE;
byte L_BYTE;
byte STATUS_L;
byte VOLTAGE_H;
byte CONFIG_H;
byte CONFIG_M;
byte CONFIG_L;
byte TMP;


void setup()
{
        //CONFIGURATION
        //uC SETUP
        pinMode(RESET, OUTPUT);
        pinMode(SS, OUTPUT);
        pinMode(MODE, OUTPUT);
        digitalWrite(RESET, HIGH);
        digitalWrite(SS, HIGH);
        digitalWrite(MODE, LOW);


        //SPI
        SPI.begin();
        SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0)); //2.5Mhz TEST
        //Serial
        Serial.begin(9600);

        //HARDWARE RESET
        digitalWrite(RESET, LOW);
        delayMicroseconds(100);
        digitalWrite(RESET, HIGH);

        //SOFTWARE RESET
        digitalWrite(SS, LOW); //Slave Select Driven Low
        //delayMicroseconds(10); //Time to drive SS Low
        SPI.transfer(0x80); //Software Reset Command
        //delayMicroseconds(10);
        digitalWrite(SS, HIGH);

        delayMicroseconds(10);

        // INITIALIZING SPI
        digitalWrite(SS, LOW);
        SPI.transfer(SYNC1);
        SPI.transfer(SYNC1);
        SPI.transfer(SYNC1);
        SPI.transfer(SYNC1);
        SPI.transfer(SYNC0);
        digitalWrite(SS, HIGH);

        //++++++++++++++++++++++++++++++++++++++++++++++//

        /* The following lines of code are included for testing the chip with a 16Mhz crystal clock
         * comment out of code when using the 4Mhz clock
         */

        delayMicroseconds(10);
        digitalWrite(SS, LOW);
        SPI.transfer(WRITE | CONFIG_ADD << 1);
        SPI.transfer(0x00);
        SPI.transfer(0x00);
        SPI.transfer(0x06); //Setting the K value to 4 to divide the 16Mhz clk to a 4Mhz clk signal
        digitalWrite(SS, HIGH);

        delayMicroseconds(10);

        digitalWrite(SS, LOW); //Code to read the Config Reg
        //delayMicroseconds(10);
        SPI.transfer(READ | CONFIG_ADD << 1);
        CONFIG_H = SPI.transfer(SYNC1);
        CONFIG_M = SPI.transfer(SYNC1);
        CONFIG_L = SPI.transfer(SYNC1);
        SPI.transfer(READ | STATUS_ADD << 1);
        CONFIG_H = SPI.transfer(SYNC1);
        CONFIG_M = SPI.transfer(SYNC1);
        STATUS_L = SPI.transfer(SYNC1);
        digitalWrite(SS, HIGH);
        //*********************************************//

        delayMicroseconds(10);

        delayMicroseconds(10);

        Serial.print("CONFIG: ");
        Serial.print(CONFIG_L);
        Serial.print('\n');
}


void loop()
{
  byte high=0;
  byte low=0;
  byte mid=0;

  byte I_HIGH=0;
  byte ACT_I=0;
  byte POWER=0;


        digitalWrite(SS, LOW);
        SPI.transfer(0xE8); //CONTINUOUS COMPUTATIONS
        //SPI.transfer(0xE0); // Single Computation Cycle
        digitalWrite(SS, HIGH);

        delay(50);

        digitalWrite(SS, LOW); //Code to read the Config Reg
        SPI.transfer(READ | STATUS_ADD << 1);
        CONFIG_H = SPI.transfer(SYNC1);
        CONFIG_M = SPI.transfer(SYNC1);
        STATUS_L = SPI.transfer(SYNC1);
        digitalWrite(SS, HIGH);

        Serial.print("STATUS: ");
        Serial.print(STATUS_L, BIN);
        Serial.print('\n');


        //READ FROM DESIRED REGISTER
        digitalWrite(SS, LOW);
        SPI.transfer(READ | TEMP_ADD << 1);
        H_BYTE = SPI.transfer(SYNC1); // Saving MSB TO H-BYTE Reg
        M_BYTE = SPI.transfer(SYNC1);     // Would be M-BYTE but we don't care
        L_BYTE = SPI.transfer(SYNC1);     // Would be L-BYTE but again we don't care
        SPI.transfer(READ | VOLT_ADD << 1);
        VOLTAGE_H = SPI.transfer(SYNC1); // Saving MSB TO H-BYTE Reg
        SPI.transfer(SYNC1);     // Would be M-BYTE but we don't care
        SPI.transfer(SYNC1);     // Would be L-BYTE but again we don't care
        SPI.transfer(READ | I_GAIN << 1);
        I_HIGH = SPI.transfer(SYNC1);
        SPI.transfer(SYNC1);
        SPI.transfer(SYNC1);
        SPI.transfer(READ | I_ADD << 1);
        ACT_I = SPI.transfer(SYNC1);
        SPI.transfer(SYNC1);
        SPI.transfer(SYNC1);
        SPI.transfer(READ | POW << 1);
        POWER = SPI.transfer(SYNC1);
        SPI.transfer(SYNC1);
        SPI.transfer(SYNC1);

        digitalWrite(SS, HIGH);

        delayMicroseconds(10);

        digitalWrite(SS, LOW);
        SPI.transfer(READ | V_GAIN << 1);
        high = SPI.transfer(SYNC1); // Saving MSB TO H-BYTE Reg
        mid = SPI.transfer(SYNC1);     // Would be M-BYTE but we don't care
        low = SPI.transfer(SYNC1);     // Would be L-BYTE but again we don't care
        digitalWrite(SS, HIGH);

        Serial.print("\n V_GAIN: ");
        Serial.print(high,HEX);
        Serial.print("\n");
        Serial.print("\n I_GAIN: ");
        Serial.print(I_HIGH,HEX);
        Serial.print("\n");
        Serial.print("\n ACT_I: ");
        Serial.print(ACT_I);
        Serial.print("\n");
        Serial.print("Voltage H:");
        Serial.print(VOLTAGE_H);
        Serial.print("\n");
        Serial.print("POWER:");
        Serial.print(POWER);
        Serial.print("\n");

        delay(2500);

/*/////////////////////////////////////////////////////////////////////////////////////////////////////////////


         digitalWrite(SS, LOW);
        SPI.transfer(READ | INSTANTVOLT << 1);
        high = SPI.transfer(SYNC1); // Saving MSB TO H-BYTE Reg
        mid = SPI.transfer(SYNC1);     // Would be M-BYTE but we don't care
        low = SPI.transfer(SYNC1);     // Would be L-BYTE but again we don't care

        digitalWrite(SS, HIGH);

        delayMicroseconds(10);

       Serial.print("\n INSTANT VOLTAGE IS HIGH: ");
       Serial.print(high,HEX);
       Serial.print("\n THE MID: ");
       Serial.print(mid,HEX);
       Serial.print("\n THE LOW");
       Serial.print(low,HEX);
       Serial.print("\n");

        ///////////////////////////////////////////////////////////////////////// */



        /////////////////////////////////////////////////////////////////////////
        /*
        Serial.print("Temperature: ");

        Serial.print(H_BYTE, HEX);
        Serial.print("\n");
        Serial.print(M_BYTE, BIN);
        Serial.print("\n");
        Serial.print(L_BYTE, BIN);
        Serial.print("\n");


         */



/*
        H_BYTE = H_BYTE - 1;
        H_BYTE = H_BYTE ^ 0xFF;
        delay(500);
        Serial.print(H_BYTE, DEC);
        Serial.print("\n");
*/
       /* delayMicroseconds(10);

        Serial.print("STATUS_L: ");
        Serial.print(STATUS_L);
        Serial.print("\n");
          */

}
