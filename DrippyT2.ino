/* DrippyT2 by Marek Macner (c) 2017 */
#define DEBUG
//#define DEBUGM
//#define DEBUGX

#include <RFduinoBLE.h>
#include <SPI.h> 
#include <Stream.h>


#define PIN_SPI_SCK   4
#define PIN_SPI_MOSI  5
#define PIN_SPI_MISO  3
#define PIN_SPI_SS    6
#define PIN_IRQ       2 

typedef struct  __attribute__((packed)) 
{
  uint8_t resultCode;
  uint8_t deviceID[13];
  uint8_t romCRC[2];
} IDNDataType;

typedef struct  __attribute__((packed)) 
{
  uint8_t uid[8];
  uint8_t resultCode;
  uint8_t responseFlags;
  uint8_t infoFlags;
  uint8_t errorCode;
  String sensorSN;
} SystemInformationDataType;

typedef struct  __attribute__((packed)) 
{
  bool sensorDataOK;
  String sensorSN;
  byte   sensorStatusByte;
  String sensorStatusString;
  byte  nextTrend;
  byte  nextHistory;
  uint16_t minutesSinceStart;
  uint16_t minutesHistoryOffset;
  uint16_t trend[16];
  uint16_t history[32];
} SensorDataDataType;

typedef struct  __attribute__((packed)) 
{
  long voltage;
  int voltagePercent;
  double temperatureC;
  double temperatureF;
  double rssi;
} RFDuinoDataType;

typedef struct dataConfig 
{
  byte marker;
  byte protocolType;                  // 1 - LimiTTer, 2 - Transmiter, 3 - LibreCGM, 4 - Transmiter II
  byte runPeriod;                     // 0-9 main loop period in miutes, 0=on demenad 
  byte firmware;                      // firmware version starting 0x02
};


byte resultBuffer[40]; 
byte dataBuffer[400];
byte NFCReady = 0;            // 0 - not initialized, 1 - initialized, no data, 2 - initialized, data OK
bool sensorDataOK = false;

IDNDataType idnData;
SystemInformationDataType systemInformationData;
SensorDataDataType sensorData;
RFDuinoDataType rfduinoData;
struct dataConfig valueSetup;
dataConfig *p;

byte sensorDataHeader[24];
byte sensorDataBody[296];
byte sensorDataFooter[24];

byte noOfBuffersToTransmit = 1;
String TxBuffer[10];
String TxBuffer1 = "";

byte protocolType = 1;    // 1 - LimiTTer, 2 - Transmiter, 3 - Transmiter II
byte runPeriod = 1;       // czas w minutach - 0 = tylko na żądanie
byte  MY_FLASH_PAGE =  251;

bool BTconnected = false;
bool BatteryOK=false;

void setup() 
{
  p = (dataConfig*)ADDRESS_OF_PAGE(MY_FLASH_PAGE);
  Serial.begin(9600);
  #ifdef DEBUGM
    Serial.println("DrippyT2 - setup - start");
  #endif 
  RfduinoData();
  setupInitData();
  protocolType = p->protocolType;
  runPeriod = p->runPeriod;
  
  setupBluetoothConnection();
  nfcInit();
  configWDT();
  #ifdef DEBUG 
    Serial.print("NFCReady = ");
    Serial.println(NFCReady);
    Serial.print("Bat = ");
    Serial.println(BatteryOK);
    Serial.println("DrippyT2 - setup - end");
  #endif
}

void loop() 
{
  #ifdef DEBUG 
    Serial.println("========================================================");
    Serial.println("DrippyT2 - loop - start");
  #endif
  if (BatteryOK)
  {
    readAllData();
    if (NFCReady == 2) 
    {
      #ifdef DEBUG 
        Serial.print("After sensor read, NFCReady = ");
        Serial.println(NFCReady);
      #endif
      dataTransferBLE();
    }
    else
    {
      #ifdef DEBUG 
        Serial.print("No sensor data, ");
        Serial.print("NFCReady = ");
        Serial.println(NFCReady);
      #endif
    }
  }
  else
  {
    #ifdef DEBUG 
      Serial.println("low Battery - go sleep");
    #endif
  } 
  #ifdef DEBUG     
    Serial.print("DrippyT2 - loop - end, ");  
    Serial.print("NFCReady = ");
    Serial.println(NFCReady);  
  #endif
  RFduino_ULPDelay(1000 * 60 * runPeriod);
  restartWDT();
}


