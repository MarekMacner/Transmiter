void configSPI()
{
  pinMode(PIN_SPI_SS, OUTPUT);
  pinMode(PIN_IRQ, OUTPUT); 
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  SPI.setFrequency(1000); 
}

void configWDT()
{
  NRF_WDT->CONFIG = (WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos) | (WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos); 
  NRF_WDT->CRV = 32768 * 60 * 11;         // 11 minut
  NRF_WDT->RREN |= WDT_RREN_RR0_Msk;                      
  NRF_WDT->TASKS_START = 1; 
}

void restartWDT()
{
   NRF_WDT->RR[0] = WDT_RR_RR_Reload;
}

void RfduinoData() 
{
  analogReference(VBG);                
  analogSelection(VDD_1_3_PS);         
  int sensorValue = analogRead(1); 
  rfduinoData.voltage = sensorValue * (360 / 1023.0) * 10; 
  rfduinoData.voltagePercent = map(rfduinoData.voltage, 3000, 3600, 0, 100);  
  rfduinoData.temperatureC = RFduino_temperature(CELSIUS); 
  rfduinoData.temperatureF = RFduino_temperature(FAHRENHEIT);  
  if (rfduinoData.voltage < 3000) BatteryOK = false;
  else BatteryOK = true;
  
  #ifdef DEBUGM
    Serial.println("RFDuino data:");
    Serial.print(" - Voltage [mv]: ");
    Serial.println(rfduinoData.voltage);
    Serial.print(" - Voltage [%]: ");
    Serial.println(rfduinoData.voltagePercent);
    Serial.print(" - Temperature [C]: ");
    Serial.println(rfduinoData.temperatureC);
    Serial.print(" - Temperature [F]: ");
    Serial.println(rfduinoData.temperatureF);
  #endif
}

void readAllData()
{  
  NFC_wakeUP();
  NFC_CheckWakeUpEventRegister();
  NFCReady = 0;
  SetNFCprotocolCommand();
  
  runSystemInformationCommandUntilNoError(10);
  systemInformationData = systemInformationDataFromGetSystemInformationResponse();
  printSystemInformationData(systemInformationData);
  sensorData.sensorDataOK = readSensorData();
  decodeSensor();
  RfduinoData();

  sendNFC_ToHibernate();
}

void setupInitData()
{
  if (p->marker == 'T')
  {
    protocolType = p->protocolType;
    runPeriod = p->runPeriod;
    #ifdef DEBUG
      Serial.print("Init data present at page ");
      Serial.println(MY_FLASH_PAGE);
      Serial.print("  protocolType = ");
      Serial.println(p->protocolType);
      Serial.print("  runPeriod = ");
      Serial.println(p->runPeriod);
      Serial.print("  firmware = ");
      Serial.println(p->firmware);
    #endif
  }
  else
  {
    eraseData();
    valueSetup.marker = 'T';
    valueSetup.protocolType = 1;                  // 1 - LimiTTer, 2 - Transmiter, 3 - LibreCGM, 4 - Transmiter II
    valueSetup.runPeriod = 5; 
    valueSetup.firmware = 0x02;  
    writeData();
    protocolType = p->protocolType;
    runPeriod = p->runPeriod;
    #ifdef DEBUG
      Serial.print("New init data stored at page ");
      Serial.println(MY_FLASH_PAGE);
      Serial.print("  protocolType = ");
      Serial.println(p->protocolType);
      Serial.print("  runPeriod = ");
      Serial.println(p->runPeriod, HEX);
      Serial.print("  firmware = ");
      Serial.println(p->firmware, HEX);
    #endif
  }    
}

void eraseData()
{
  int rc;
  #ifdef DEBUGM
    Serial.print("Attempting to erase flash page ");
    Serial.print(MY_FLASH_PAGE);
  #endif  
  rc = flashPageErase(PAGE_FROM_ADDRESS(p));
  #ifdef DEBUGM
    if (rc == 0)
      Serial.println(" -> Success");
    else if (rc == 1)
      Serial.println(" -> Error - the flash page is reserved");
    else if (rc == 2)
      Serial.println(" -> Error - the flash page is used by the sketch");
  #endif
}

void writeData()
{
  int rc;
  #ifdef DEBUGM
    Serial.print("Attempting to write data to flash page ");
    Serial.print(MY_FLASH_PAGE);
  #endif   
  valueSetup.marker = 'T';
  rc = flashWriteBlock(p, &valueSetup, sizeof(valueSetup));
  #ifdef DEBUG
    if (rc == 0)
      Serial.println(" -> Success");
    else if (rc == 1)
      Serial.println(" -> Error - the flash page is reserved");
    else if (rc == 2)
      Serial.println(" -> Error - the flash page is used by the sketch");
  #endif 
}

void displayData()
{
  #ifdef DEBUGM
    Serial.print("The data stored in flash page ");
    Serial.print(MY_FLASH_PAGE);
    Serial.println(" contains: ");
    Serial.print("  protocolType = ");
    Serial.println(p->protocolType);
    Serial.print("  runPeriod = ");
    Serial.println(p->runPeriod);
    Serial.print("  firmware = ");
    Serial.println(p->firmware, HEX);
  #endif 
}

void RFduinoBLE_onReceive(char *data, int len) 
{
  if (data[0] == 'V')
  {
    String v = "v ";
    v += String(p->protocolType) + " ";
    v += String(p->runPeriod) + " ";
    v += String(p->firmware, HEX) + " v";
    #ifdef DEBUG
      Serial.println("V-command received.");
      Serial.println(v);
    #endif
    RFduinoBLE.send(v.c_str(), v.length());   
    displayData();
  }
  else if (data[0] == 'M')
  {    
    valueSetup.protocolType = (byte) (data[1]- '0');  
    valueSetup.runPeriod = (byte) (data[2]- '0');
    valueSetup.firmware = p->firmware; 
    #ifdef DEBUG
      Serial.println("M-command received.");
    #endif    
    while(!RFduinoBLE.radioActive){} 
    delay(6);
    eraseData();
    writeData();
    displayData();  
    protocolType = p->protocolType;
    runPeriod = p->runPeriod;
  }
  else if (data[0] == 'R')
  {
    #ifdef DEBUG
      Serial.println("R-command received.");
    #endif  
    readAllData();
    dataTransferBLE();
  }
  else
  {
    #ifdef DEBUG
      Serial.println("Wrong command received.");
    #endif
  }    
}

void RFduinoBLE_onDisconnect() 
{
    BTconnected = false;
    #ifdef DEBUG
      Serial.println("BT disconnected.");
    #endif
}

void RFduinoBLE_onConnect() 
{
    BTconnected = true;
    #ifdef DEBUG
      Serial.println("BT connected.");
    #endif
}

void RFduinoBLE_onRSSI(int rssi)
{
  rfduinoData.rssi = rssi;
}

