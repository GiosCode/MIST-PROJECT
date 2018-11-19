//defines determine how code is compiled comment out as necessary
#define UNO //When defined code assumes Arduino is being used
#define 16CLK //When defined code assumes 16Mhz clk is being used
#define DEBUG //When defined Serial.print shows current values

#include <SPI.h>

#ifdef UNO
  #include <util/delay.h>
#else
  #include <delay.h>
#endif

#ifdef DEBUG
  #define Debug_print(x) Serial.print(x)
#else
  #define Debug_print(x)
#endif

#ifdef UNO
  #define MODE          4
  #define RESET         2
  #define SS            3
#else
  #define MODE          15       //A1
  #define RESET         16      //A2
  #define SS             17     //A3
#endif

//POWER MONITOR ADDRESSES
#define WRITE         64
#define READ          0
#define CONFIG_ADD    0x00
#define MODE_ADD      18
#define STATUS_ADD    15
#define TEMP_ADD      19
#define VOLT_ADD      8
#define SYNC1         0xFF
#define SYNC0         0XFE

//Variables
int8 H_BYTE;
int8 M_BYTE;
int8 L_BYTE;
long tmp;

long make24(int8 one, int8 two, int8 three)
{
  long rtn;
  rtn = (one << 16 | two << 8 | three);
}

void spi_write(int8 reg, int8 msb, int8 mid, int8 lsb)
{
  digitalWrite(SS,LOW);
  SPI.transfer(WRITE | reg << 1);
  SPI.transfer(msb);
  SPI.transfer(mid);
  SPI.transfer(lsb);
  digitalWrite(SS,HIGH);
}

long spi_read(int8 reg, int8 msb, int8 mid, int8 lsb)
{
  int8 msb,mid,lsb;
  long retval;

  digitalWrite(SS,LOW);
  SPI.transfer(READ | reg << 1);
  msb = SPI.transfer(SYNC1);
  mid = SPI.transfer(SYNC1);
  lsb = SPI.transfer(SYNC1);
  digitalWrite(SS,HIGH);

  retval = make24(msb,mid,lsb);

}

void hardware_reset()
{
  digitalWrite(RESET, LOW);
  delayMicroseconds(5);
  digitalWrite(RESET, HIGH);
  delayMicroseconds(100);
}

void software_reset()
{
  digitalWrite(SS, LOW); //Slave Select Driven Low
  SPI.transfer(0x80); //Software Reset Command
  digitalWrite(SS, HIGH);
}

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
        SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

        //Serial
        Serial.begin(9600);

        //HARDWARE RESET
        hardware_reset();
        //SOFTWARE RESET
        software_reset();

        delayMicroseconds(10);

        // INITIALIZING SPI ---------check in datasheet
        digitalWrite(SS, LOW);
        SPI.transfer(SYNC1);
        SPI.transfer(SYNC1);
        SPI.transfer(SYNC1);
        SPI.transfer(SYNC1);
        SPI.transfer(SYNC0);
        digitalWrite(SS, HIGH);

        //++++++++++++++++++++++++++++++++++++++++++++++//
        delayMicroseconds(10);

        //Setting the K value to 4 to divide the 16Mhz clk to a 4Mhz clk signal
        #ifdef 16CLK
        spi_write(CONFIG_ADD, 0x00,0x00,0x04);
        #endif

        delayMicroseconds(10);

        //Read the Config Reg
        tmp = spi_read(CONFIG,H_BYTE,M_BYTE,L_BYTE);
        Debug_print(tmp, BIN);

        //Read Status
        tmp = spi_read(STATUS_ADD,);
        Debug_print(tmp, BIN);

        delayMicroseconds(10);

        //CONTINUOUS COMPUTATIONS
        digitalWrite(SS, LOW);
        SPI.transfer(0xE8);
        digitalWrite(SS, HIGH);

        delayMicroseconds(10);
}

void loop()
{
        delay_ms(100);
        tmp = spi_read(STATUS_ADD,H_BYTE,M_BYTE,L_BYTE);

        Debug_print("STATUS: ");
        Debug_print(tmp);
        Debug_print("\n");

        //READ FROM DESIRED REGISTER
        tmp = spi_read(TEMP_ADD,H_BYTE,M_BYTE,L_BYTE);

        Debug_print("TEMP: ");
        Debug_print(tmp);
        Debug_print("\n");

        tmp = spi_read(VOLT_ADD, H_BYTE,M_BYTE,L_BYTE);

        Debug_print("Volt: ");
        Debug_print(tmp);
        Debug_print("\n");
        delay(2500);
}
