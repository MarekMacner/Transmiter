void dataTransferBLE()
{  
  
  for (int i=0; i<6; i++)
  {
    if (BTconnected)
    {
      if (protocolType == 1) forLimiTTer();
      else if (protocolType == 2) forTransmiter1();
      else if (protocolType == 3) forTransmiter2(); 
      else if (protocolType == 4) forLibreCGM();
      #ifdef DEBUGM
        Serial.println("Conected - data transferred");
      #endif
      break;
    }
    else
    {     
      #ifdef DEBUGM
        Serial.print("Not conected - data not transferred -> try:");
        Serial.println(i);
      #endif
      delay(1000);
    }
  }  
  NFCReady = 1;
  
}

void setupBluetoothConnection() 
{
  if (protocolType == 1) RFduinoBLE.deviceName = "LimiTTer";
  else if (protocolType == 2) RFduinoBLE.deviceName = "LimiTTer";
  else if (protocolType == 3) RFduinoBLE.deviceName = "Transmiter";   
  RFduinoBLE.advertisementData = "data";
  RFduinoBLE.customUUID = "c97433f0-be8f-4dc8-b6f0-5343e6100eb4";
  RFduinoBLE.advertisementInterval = MILLISECONDS(300); 
  RFduinoBLE.txPowerLevel = 4; 
  RFduinoBLE.begin();    
  #ifdef DEBUGM
    Serial.println("... done seting up Bluetooth stack and starting connection.");
  #endif
}
