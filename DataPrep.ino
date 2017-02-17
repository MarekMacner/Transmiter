void forLimiTTer()
{
  noOfBuffersToTransmit = 1;
  TxBuffer1 = "";
  TxBuffer1 += String(sensorData.trend[0] * 100);
  TxBuffer1 += " ";
  TxBuffer1 += String(rfduinoData.voltage);
  TxBuffer1 += " ";
  TxBuffer1 += String(rfduinoData.voltagePercent);
  TxBuffer1 += " ";
  TxBuffer1 += String((int)(sensorData.minutesSinceStart/10));
  int  LL = TxBuffer1.length();  
  #ifdef DEBUG
    Serial.print("for LimiTTer >>");
    Serial.print(TxBuffer1);
    Serial.print("<< ");
    Serial.println(LL);
  #endif
  RFduinoBLE.send(TxBuffer1.c_str(), TxBuffer1.length());
}

void forTransmiter1()
{
  noOfBuffersToTransmit = 1;
  TxBuffer1 = "";
  TxBuffer1 += String(sensorData.trend[0] * 100);
  TxBuffer1 += " ";
  TxBuffer1 += String(rfduinoData.voltage);
  TxBuffer1 += " ";
  TxBuffer1 += String(rfduinoData.voltagePercent);
  TxBuffer1 += " ";
  TxBuffer1 += String((int)(rfduinoData.temperatureC*10));
  int  LL = TxBuffer1.length();  
  #ifdef DEBUG
    Serial.print("for Transmiter 1 >>");
    Serial.print(TxBuffer1);
    Serial.print("<< ");
    Serial.println(LL);
  #endif
  RFduinoBLE.send(TxBuffer1.c_str(), TxBuffer1.length());
}

void forTransmiter2()
{
  noOfBuffersToTransmit = 0;
  String allHardware = "";          // systemInformationData.resultCode, systemInformationData.responseFlags,
                                    // rfduinoData.voltage, rfduinoData.voltagePercent, rfduinoData.temperatureC,
                                    //  RSSI
  String allData = "";              // sensorSN, sensorStatusByte, minutesHistoryOffset, minutesSinceStart, 
                                    // all trend, all history                                    
  allHardware +=  String(systemInformationData.resultCode) + " ";
  allHardware +=  String(systemInformationData.responseFlags) + " ";
  allHardware +=  String(rfduinoData.voltage) + " ";
  allHardware +=  String(rfduinoData.voltagePercent) + " ";
  allHardware +=  String((int)(rfduinoData.temperatureC*10)) + " ";
  allHardware +=  String((int)(rfduinoData.rssi)) + " ";
  int  LLh = allHardware.length();  
  allData += sensorData.sensorSN + " ";   
  allData += String(sensorData.sensorStatusByte) + " ";
  allData += String(sensorData.minutesHistoryOffset) + " ";
  allData += String(sensorData.minutesSinceStart) + " ";
  for (int i=0; i<16; i++) allData += String(sensorData.trend[i]) + " ";
  for (int i=0; i<32; i++) allData += String(sensorData.history[i]) + " ";  
  int  LLd = allData.length();
  #ifdef DEBUG
    Serial.println("for Transmiter 2 - Hardware data");
    Serial.print(">>");
    Serial.print(allHardware);
    Serial.print("<< ");
    Serial.println(LLh);
    Serial.println("for Transmiter 2 - Sensor data");
    Serial.print(">>");
    Serial.print(allData);
    Serial.print("<< ");
    Serial.println(LLd);   
  #endif
  String toTransfer = "M "+allHardware + " " + allData + " M";
  int lengthToTransfer = toTransfer.length();
  byte toCRC[lengthToTransfer];
  for (int i=0; i< lengthToTransfer; i++) toCRC[i] = (byte) toTransfer[i]; 
  uint16_t CRC16toTransfer = computeCRC16(toCRC, lengthToTransfer);
  toTransfer += " *" + String(CRC16toTransfer, HEX) + "*";
  lengthToTransfer = toTransfer.length();
  for (int i=0; i< (1+((int)(lengthToTransfer))/20); i++)
  {
    TxBuffer1 = "";
    TxBuffer1 = toTransfer.substring(20*i, 20*(i+1));
    RFduinoBLE.send(TxBuffer1.c_str(), TxBuffer1.length());
    #ifdef DEBUG
      Serial.print("B-");
      Serial.print(i);
      Serial.print(" >>");
      Serial.print(TxBuffer1);
      Serial.println("<<");
    #endif
  } 
}

void forLibreCGM()
{
  #ifdef DEBUGM
    Serial.println("Start BT transmission ...");
    Serial.printf("IDN Sizeof ist : %d\n", sizeof(systemInformationData));
  #endif  
  bool ergo = pumpViaBluetooth(SYSTEM_INFORMATION_DATA, UBP_TxFlagIsRPC, (char *) &systemInformationData, sizeof(SystemInformationDataType));
  printSystemInformationData(systemInformationData);    
  #ifdef DEBUGM
    Serial.println("----about to send all data bytes packet");
  #endif
  for (int i = 0; i < sizeof(allBytes.allBytes); i++) 
  {
    allBytes.allBytes[i] = 0;
  }
  for (int i = 0; i < 344; i++) 
  {
    allBytes.allBytes[i] = dataBuffer[i];
  }
  #ifdef DEBUGM
    Serial.printf("All data Sizeof ist : %d\n", sizeof(allBytes));
  #endif
  bool success = UBP_queuePacketTransmission(ALL_BYTES, UBP_TxFlagIsRPC, (char *) &allBytes, sizeof(AllBytesDataType));
  #ifdef DEBUGM
    if (success) Serial.println("----all data bytes packet queued successfully");
    else Serial.println("----Failed to enqueue all data bytes packet");
  #endif
  while (UBP_isBusy() == true) UBP_pump();
  batteryData.voltage = (float) rfduinoData.voltage;
  batteryData.temperature = RFduino_temperature(CELSIUS);
  #ifdef DEBUGM
    Serial.printf("Battery voltage: %f\r\n", batteryData.voltage);
    Serial.printf("Bat Sizeof ist : %d\n", sizeof(batteryData));
  #endif
  success = UBP_queuePacketTransmission(BATTERY_DATA, UBP_TxFlagIsRPC, (char *) &batteryData, sizeof(BatteryDataType));
  #ifdef DEBUGM
    if (success) Serial.println("Battery data packet queued successfully");
    else Serial.println("Failed to enqueue battery data packet");
  #endif
  while (UBP_isBusy() == true) UBP_pump();
  #ifdef DEBUGM
    Serial.printf("Sent Battery voltage: %f\r\n", batteryData.voltage);
  #endif
  ergo = pumpViaBluetooth(IDN_DATA, UBP_TxFlagNone, (char *) &idnData, sizeof(IDNDataType));
  success = UBP_queuePacketTransmission(IDN_DATA, UBP_TxFlagIsRPC, (char *) &idnData, sizeof(IDNDataType));
  delay(10);
  #ifdef DEBUGM
    if (success) Serial.println("IDN data packet queued successfully");
    else Serial.println("Failed to enqueue IDN data packet");
  #endif
  while (UBP_isBusy() == true) UBP_pump();
}

