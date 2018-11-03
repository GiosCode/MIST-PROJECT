// CS5463 Power Monitoring Test Code

/* 
IMPORTANT:
B7  B6  B5  B4  B3  B2  B1  B0
0  W/R RA4 RA3 RA2 RA1 RA0  0

W = 1 
R = 0

RA[4:0] 
(Page 0) 24
00000 = Config
00110 = PulseRateE
01011 = RMS Current
01100 = RMS Voltage
10010 = Mode
(Page 1) 25
00000 = PulseWidth
00001 = No Load Threshold
(Page 3) 25

*/

// HAVE TO INCLUDE LIBRARIES!!!!!!!!!!!!!!!!!!!!!!!!!
//NEED TO DEFINE F_CPU
#define F_CPU 48000000UL
#include <delay.h>
#include <SPI.h>

//////////////////////////////////////////////////////////////

#define Config 0;
#define PulseRate 6;
#define Mode 18;
#define PulseWidth 0;
#define No_Load_Threshold 1;

//PINOUT Registers
const int CS = 15;
const int Reset_Pin = 16;
const int Mode_Pin = 17;

byte H_Byte;              //High Byte 
byte M_Byte;              //Middle Byte 
byte L_Byte;              //Low Byte

int Write = 64;
int Read = 0;


int READINGS [3][32];

//FEATHER INIT

void setup() 
{
  pinMode(CS, OUTPUT);
  pinMode(Reset_Pin, OUTPUT);
  pinMode(Mode_Pin, OUTPUT);

  digitalWrite(CS,HIGH);
  digitalWrite(Reset_Pin, HIGH);
  digitalWrite(Mode_Pin, LOW);

  delay(100);

  // INIT SERIAL COM
  Serial.begin(9600);

  //SPI LIBRARY
  SPI.begin();
  SPI.beginTransaction(SPISettings(4000000,MSBFIRST,SPI_MODE0));

  //Hardware Reset
  digitalWrite(CS, LOW);
  digitalWrite(Reset_Pin, LOW);
    delay (100);
  digitalWrite(Reset_Pin, HIGH);
    delay (100);

  digitalWrite(CS,HIGH);
  delay(100);
  
//------------------------------SET REGS-------------------------------//

  //Set Config register
  digitalWrite(CS, LOW);          //Chip select to low to initialise comms with CS5463 
  SPI.transfer( Write | (0<<1) );
  SPI.transfer(0x00);           //3 bytes of data to set 24bits of config register        
    SPI.transfer(0x00);     
    SPI.transfer(0x01);           // Set K value to 1 (clock devider for measuring cycles)
    digitalWrite(CS, HIGH);         //Chip select to HIGH to disable comms with CS5463

  //Set Control register
  digitalWrite(CS, LOW);          
  SPI.transfer( Write | (28<<1) );
  SPI.transfer(0x00);           //Disables CPUCLK 
  SPI.transfer(0x00);     
  SPI.transfer(0x4); 
  digitalWrite(CS, HIGH);

  
  // Set offsets to default value (no offset)
  digitalWrite(CS, LOW);
   
  SPI.transfer( Write | (1 << 1 ));     
  SPI.transfer(0x00); 
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer( Write | (3  << 1 ));    
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer(0x00); 
  SPI.transfer( Write | (2   << 1 ));    
  SPI.transfer(0x40);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer( Write | (4  << 1 ));    
  SPI.transfer(0x40);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
    digitalWrite(CS, HIGH);
    

}

void loop() 
{
  digitalWrite(CS, LOW);//Chip select
  //begin reading
  
  SPI.transfer( Read | ( 12<<1) );
  H_Byte = SPI.transfer(255);
  M_Byte = SPI.transfer(255);
  L_Byte = SPI.transfer(255);
  //delay(1000);
  
  READINGS[0][12]=  H_Byte  ;     //store recieved data in 0-2 collumn of the array 
  READINGS[1][12]=  M_Byte  ;
  READINGS[2][12]=  L_Byte  ;

  
  
  //Serial.print("REAL_VOLTAGE_RMSasasas: ");
  //Serial.println(READINGS[2][12]);
  float REAL_VOLTAGE_RMS =  (READINGS[0][12]);
  delay (1000);
  digitalWrite(CS,HIGH);
  
  Serial.print("REAL_VOLTAGE_RMS: ");
  Serial.println(REAL_VOLTAGE_RMS);
  
}
