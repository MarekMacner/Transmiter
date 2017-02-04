//============================================================================================================
void NFC_wakeUP() 
{    
  #ifdef DEBUGM
    Serial.println("Send wake up pulse to CR95HF and configure to use SPI...");
  #endif    
  digitalWrite(PIN_IRQ, HIGH);
  delay(10);
  digitalWrite(PIN_IRQ, LOW);
  delayMicroseconds(100);
  digitalWrite(PIN_IRQ, HIGH);
  delay(10);
  #ifdef DEBUGM
    Serial.println("... done sending wake up pulse to CR95HF and configuring to use SPI.");
  #endif
}
void NFC_CheckWakeUpEventRegister() 
{
    int length = 5;
    byte command[length];
    command[ 0] = 0x08;   
    command[ 1] = 0x03;   
    command[ 2] = 0x62;  
    command[ 3] = 0x01;  
    command[ 4] = 0x00;  
    send_NFC_PollReceive(command, sizeof(command)); 
    print_NFC_WakeUpRegisterResponse();
}
void send_NFC_PollReceive(byte *command, int commandLength) 
{    
  send_NFC_Command(command, commandLength);    
  poll_NFC_UntilResponsIsReady();    
  receive_NFC_Response();
}
//============================================================================================================
void send_NFC_Command(byte *commandArray, int length) 
{
  digitalWrite(PIN_SPI_SS, LOW);    
  SPI.transfer(0x00); 
  for (int i=0; i<length; i++) 
  {
    SPI.transfer(commandArray[i]);
  }
  digitalWrite(PIN_SPI_SS, HIGH);
  delay(1);
}
void poll_NFC_UntilResponsIsReady() 
{   
  digitalWrite(PIN_SPI_SS , LOW);    
  while(resultBuffer[0] != 8) 
  {
    resultBuffer[0] = SPI.transfer(0x03); 
    #ifdef DEBUGX
      Serial.printf("SPI polling response byte:%x\r\n", resultBuffer[0]);
    #endif
    resultBuffer[0] = resultBuffer[0] & 0x08;  
  }
  digitalWrite(PIN_SPI_SS, HIGH);
  delay(1);
}
void receive_NFC_Response() 
{    
  digitalWrite(PIN_SPI_SS, LOW);
  SPI.transfer(0x02);    
  resultBuffer[0] = SPI.transfer(0);  
  resultBuffer[1] = SPI.transfer(0);     
  for (byte i=0; i<resultBuffer[1]; i++) resultBuffer[i+2] = SPI.transfer(0);      
  digitalWrite(PIN_SPI_SS, HIGH);
  delay(1);
}
//============================================================================================================
void print_NFC_WakeUpRegisterResponse() 
{
  #ifdef DEBUGM
    Serial.printf("Printing result of read wake-up register command ...\r\n");
    Serial.printf("  Result code (byte 0): %x\r\n", resultBuffer[0]);    
    Serial.printf("Length of data (byte 1): %d\r\n", resultBuffer[1]);
    if (resultBuffer[1] > 0) 
    {
      for (int i=2; i<2+resultBuffer[1]; i++) 
      {
        Serial.printf("   Data byte %d: %x\r\n", i, resultBuffer[i]);
      }
    }
    Serial.println("... finished printing wake-up register result.");
  #endif
}
//============================================================================================================
void SetNFCprotocolCommand() 
{
  for (int t=0; t<9; t++)
  {
    int length = 4;
    byte command[length];
    command[ 0] = 0x02;   
    command[ 1] = 0x02;   
    command[ 2] = 0x01;  
    command[ 3] = 0x0F;  
//    command[ 3] = 0x0D; 
    send_NFC_PollReceive(command, sizeof(command));
    #ifdef DEBUGM
      Serial.print("resultBuffer: ");
      for (byte i = 0; i < 2; i++) 
      {
        Serial.print(resultBuffer[i], HEX);
        Serial.print(" ");
      }
    #endif
    if ((resultBuffer[0] == 0) & (resultBuffer[1] == 0)) 
    {
      #ifdef DEBUG
        Serial.print("Try=");
        Serial.print(t);
        Serial.println(" PROTOCOL SET - OK");  
      #endif
      NFCReady = 1;
      break; 
    } 
    else 
    {
      #ifdef DEBUG
        Serial.print("Try=");
        Serial.print(t);
        Serial.println(" BAD RESPONSE TO SET PROTOCOL");
      #endif
      NFCReady = 0; // NFC not ready
    }      
  }    
}
//============================================================================================================
void runIDNCommand(int maxTrials) 
{
  byte command[2];
  command[0] = 0x01;  
  command[1] = 0x00;   
  delay(10);
  #ifdef DEBUG
    Serial.printf("maxTrials: %d, RXBuffer[0]: %x \r\n", maxTrials, resultBuffer[0]);
  #endif
  runIDNCommandUntilNoError(command, sizeof(command), maxTrials);
}
void runIDNCommandUntilNoError(byte *command, int length, int maxTrials) 
{
  int count = 0;
  bool success;
  do 
  {
    #ifdef DEBUGM
      Serial.printf("Before: Count: %d, success: %b, resultBuffer[0]: %x \r\n", count, success, resultBuffer[0]);
    #endif
    count++;
    memset(resultBuffer, 0, sizeof(resultBuffer));
    send_NFC_PollReceive(command, sizeof(command));
    success = idnResponseHasNoError();
    #ifdef DEBUGM
      Serial.printf("After: Count: %d, success: %b, resultBuffer[0]: %x \r\n", count, success, resultBuffer[0]);
    #endif
  } while ( !success && (count < maxTrials));
  delay(10);
  #ifdef DEBUGM
    Serial.printf("Exiting at count: %d, resultBuffer[0]: %x \r\n", count, resultBuffer[0]);
  #endif
}
bool idnResponseHasNoError() 
{    
  #ifdef DEBUG
    Serial.printf("IDN response is resultBuffer[0]: %x \r\n", resultBuffer[0]);
  #endif
  if (resultBuffer[0] == 0x00)
  {
    return true;
  }
  return false;
}
IDNDataType idnDataFromIDNResponse() 
{
  idnData.resultCode = resultBuffer[0];
  for (int i = 0; i < 13; i++) 
  {
    idnData.deviceID[i] = resultBuffer[i + 2];
  }
  idnData.romCRC[0] = resultBuffer[13+2];
  idnData.romCRC[1] = resultBuffer[14+2];
  return idnData;
}
void printIDNData(IDNDataType idnData) 
{
  #ifdef DEBUGM
    String nfc="";
    Serial.println("Printing IDN data:");
    Serial.printf("Result code: %x\r\n", idnData.resultCode);
    Serial.printf("NFC Device ID  [hex]: ");
    Serial.printf("%x", idnData.deviceID[0]);
    nfc += (char) idnData.deviceID[0];
    for (int i = 1; i < 12; i++) 
    {
      Serial.printf(":%x", idnData.deviceID[i]);
      nfc += (char) idnData.deviceID[i];
    }
    Serial.println("");
    Serial.printf("NFC Device ID [char]: ");
    Serial.println(nfc);
    Serial.printf("NFC Device CRC  %x:%x\r\n", idnData.romCRC[0], idnData.romCRC[1] );
    Serial.println("");
  #endif
}
//============================================================================================================
void runSystemInformationCommandUntilNoError(int maxTrials) 
{
  memset(resultBuffer, 0, sizeof(resultBuffer));
  #ifdef DEBUGX
    Serial.printf("maxTrials: %d, resultBuffer[0]: %x \r\n", maxTrials, resultBuffer[0]);
  #endif
  byte command[4];
  command[0] = 0x04;   
  command[1] = 0x02;   
  command[2] = 0x03;   
  command[3] = 0x2B;   
  delay(10);
  #ifdef DEBUGX
    Serial.printf("maxTrials: %d, resultBuffer[0]: %x \r\n", maxTrials, resultBuffer[0]);
  #endif
  runNFCcommandUntilNoError(command, sizeof(command), maxTrials);
}
void runNFCcommandUntilNoError(byte *command, int length, int maxTrials) 
{
  int count = 0;
  bool success;
  do 
  {
    delay(1);
    #ifdef DEBUGX
      Serial.printf("Before: Count: %d, success: %b, resultBuffer[0]: %x \r\n", count, success, resultBuffer[0]);
    #endif
    count++;
    send_NFC_PollReceive(command, sizeof(command));
    success = responseHasNoError();
    #ifdef DEBUGX
      Serial.printf("After: Count: %d, success: %b, resultBuffer[0]: %x \r\n", count, success, resultBuffer[0]);
    #endif
  } while ( !success && (count < maxTrials));
  delay(1);
  #ifdef DEBUGX
    Serial.printf("Exiting at count: %d, resultBuffer[0]: %x \r\n", count, resultBuffer[0]);
  #endif
}
bool responseHasNoError() 
{   
  #ifdef DEBUGX
    Serial.printf("Response is resultBuffer[0]: %x, resultBuffer[2]: %x \r\n", resultBuffer[0], resultBuffer[2]);
  #endif
  if (resultBuffer[0] == 0x80) 
  {
    if ((resultBuffer[2] & 0x01) == 0) 
    {
      return true;
    }
  }
  return false;
}

SystemInformationDataType systemInformationDataFromGetSystemInformationResponse()
{
  SystemInformationDataType systemInformationData;
  systemInformationData.resultCode = resultBuffer[0];
  systemInformationData.responseFlags = resultBuffer[2];
  if (systemInformationData.resultCode == 0x80) 
  {
    if ((systemInformationData.responseFlags & 0x01) == 0) 
    {
      systemInformationData.infoFlags = resultBuffer[3];
      for (int i = 0; i < 8; i++) 
      {
        systemInformationData.uid[i] = resultBuffer[11 - i];
      }
      systemInformationData.errorCode = resultBuffer[resultBuffer[1] + 2 - 1];
    } 
    else 
    {
      systemInformationData.errorCode = resultBuffer[3];
    }
    systemInformationData.sensorSN = decodeSN(systemInformationData.uid);
    sensorData.sensorSN = systemInformationData.sensorSN;
  } 
  else 
  {
    clearBuffer(systemInformationData.uid);
    systemInformationData.errorCode = resultBuffer[3];
  }
  return systemInformationData;
}

void printSystemInformationData(SystemInformationDataType systemInformationData) 
{
  #ifdef DEBUG
    Serial.println("Printing system information data");
    Serial.printf("Result code: %x\r\n", systemInformationData.resultCode);
    Serial.printf("Response flags: %x\r\n", systemInformationData.responseFlags);
    #ifdef DEBUGX
      Serial.printf("uid: %x", systemInformationData.uid[0]);
      for (int i = 1; i < 8; i++) 
      {
        Serial.printf(":%x", systemInformationData.uid[i]);
      }
      Serial.println("");
    #endif
    Serial.print("Sensor SN:");
    Serial.println(systemInformationData.sensorSN);
    Serial.printf("Error code: %x\r\n", systemInformationData.errorCode);
  #endif
}

void clearBuffer(byte *tmpBuffer) 
{   
  memset(tmpBuffer, 0, sizeof(tmpBuffer));
}
//============================================================================================================
bool readSensorData()
{
  byte resultCode = 0;
  int trials = 0;
  int maxTrials = 10;
  clearBuffer(dataBuffer);
  for (int i = 0; i < 43; i++) 
  { 
    resultCode = ReadSingleBlockReturn(i);
    #ifdef DEBUGX
      printf("resultCode 0x%x - ", resultCode);
    #endif
    if (resultCode != 0x80 && trials < maxTrials) 
    {
      #ifdef DEBUGX        
        printf("Error 0x%x\n\r", resultCode);
      #endif
      i--;        // repeat same block if error occured, but
      trials++;   // not more than maxTrials times per block
    } 
    else if (trials >= maxTrials) 
    {
      break;
    } 
    else 
    {
      trials = 0;
      for (int j = 3; j < resultBuffer[1] + 3 - 4; j++) 
      {
        dataBuffer[i * 8 + j - 3] = resultBuffer[j];
        #ifdef DEBUGX
          Serial.print(resultBuffer[j], HEX);
          Serial.print(" ");
        #endif
      }
      #ifdef DEBUGX   
        Serial.println(" ");
      #endif     
    }    
  }  
  bool resultH = checkCRC16(dataBuffer, 0);
  bool resultB = checkCRC16(dataBuffer, 1);
  bool resultF = checkCRC16(dataBuffer, 2);
  bool crcResult = false;
  #ifdef DEBUGX
    Serial.println();
    Serial.print(" CRC-H check = ");
    Serial.println(resultH);  
    Serial.print(" CRC-B check = ");
    Serial.println(resultB);  
    Serial.print(" CRC-F check = ");
    Serial.println(resultF);
  #endif
  if (resultH && resultB && resultF) crcResult = true;
  else crcResult = false;
  #ifdef DEBUG
    Serial.print(" CRC check ");
    Serial.println(crcResult);
  #endif
  if (crcResult) NFCReady = 2;
  else NFCReady = 1;
  return crcResult;
}

byte ReadSingleBlockReturn(int blockNum) 
{
  int length = 5;
  byte command[length];
  command[0] = 0x04;
  command[1] = 0x03;               
  command[2] = 0x03;        
  command[3] = 0x20;          
  command[4] = blockNum; 
  send_NFC_Command(command, 5);
  poll_NFC_UntilResponsIsReady();    
  receive_NFC_Response();  
  delay(1);  
  #ifdef DEBUGX
    if (resultBuffer[0] == 128)  
    {
    
      Serial.printf("The block #%d:", blockNum);  
      for (byte i = 3; i < resultBuffer[1] + 3 - 4; i++) 
      {
        Serial.print(resultBuffer[i], HEX);
        Serial.print(" ");
      }
      Serial.println(" ");
    } 
    else 
    {
      Serial.print("NO Single block available - ");
      Serial.print("RESPONSE CODE: ");
      Serial.println(resultBuffer[0], HEX);
    } 
    Serial.println(" ");
  #endif
  return resultBuffer[0]; 
}
//============================================================================================================
void nfcInit()
{
  configSPI();
  NFC_wakeUP();
  NFC_CheckWakeUpEventRegister();
  NFCReady = 0;
  SetNFCprotocolCommand();
  runIDNCommand(10);
  idnData = idnDataFromIDNResponse();
  printIDNData(idnData);
}

void sendNFC_ToHibernate() 
{
    int length = 17;
    byte command[length];
    command[ 0] = 0x07;  
    command[ 1] = 0x0E;  
    command[ 2] = 0x08;   
    command[ 3] = 0x04;   
    command[ 4] = 0x00;  
    command[ 5] = 0x04;   
    command[ 6] = 0x00;  
    command[ 7] = 0x18;  
    command[ 8] = 0x00;   
    command[9 ] = 0x00;   
    command[10] = 0x00;  
    command[11] = 0x00;   
    command[13] = 0x00;   
    command[14] = 0x00;  
    command[15] = 0x00;  
    command[16] = 0x00;   
    send_NFC_Command(command, sizeof(command));
}


