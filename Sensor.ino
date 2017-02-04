String decodeSN(byte *data)
{
  byte uuid[8];
  String lookupTable[32] = 
  { 
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", 
    "A", "C", "D", "E", "F", "G", "H", "J", "K", "L", 
    "M", "N", "P", "Q", "R", "T", "U", "V", "W", "X", 
    "Y", "Z" 
    };            
    byte uuidShort[8];                        
    for (int i = 2; i < 8; i++)  uuidShort[i-2] = data[i];                                 
    uuidShort[6] = 0x00;
    uuidShort[7] = 0x00;
    String binary="";
    String binS="";
    for (int i=0; i<8; i++)  
    {
      binS = String(uuidShort[i], BIN); 
      int l = binS.length();
      if (l == 1) binS = "0000000"+binS;
      else if (l == 6) binS = "00"+binS;
      else if (l == 7) binS = "0"+binS;
      binary+=binS;
    }
    String v = "0";            
    char pozS[5];
    for (int i=0; i<10; i++)
    {
      for (int k=0; k<5;k++) pozS[k] = binary[(5*i)+k];                               
      int value = (pozS[0]-'0')*16 + (pozS[1]-'0')*8 + (pozS[2]-'0')*4 + (pozS[3]-'0')*2 + (pozS[4]-'0')*1;
      v += lookupTable[value];
    }
    return v;
}
void decodeSensor()
{
  for (int i = 0; i < 24; i++) sensorDataHeader[i] = dataBuffer[i];
  for (int i = 24; i < 320; i++) sensorDataBody[i - 24] = dataBuffer[i];
  for (int i = 320; i < 344; i++) sensorDataFooter[i - 320] = dataBuffer[i];
  decodeSensorHeader();
  decodeSensorBody();
  decodeSensorFooter();
  displaySensorData();
}

void decodeSensorHeader()
{
  sensorData.sensorStatusByte = sensorDataHeader[4];
  switch (sensorData.sensorStatusByte)
  {
    case 0x01:
      sensorData.sensorStatusString = "not yet started";
      break;
    case 0x02:
      sensorData.sensorStatusString = "starting";
      break;
    case 0x03:
      sensorData.sensorStatusString = "ready";
      break;
    case 0x04:
      sensorData.sensorStatusString = "expired";
      break;
    case 0x05:
      sensorData.sensorStatusString = "shutdown";
      break;
    case 0x06:
      sensorData.sensorStatusString = "failure";
      break;
    default:
      sensorData.sensorStatusString = "unknown state";
      break;
  }
}

void decodeSensorBody()
{
  sensorData.nextTrend = sensorDataBody[2];
  sensorData.nextHistory = sensorDataBody[3];
  byte minut[2];
  minut[0] = sensorDataBody[293];
  minut[1] = sensorDataBody[292];
  #ifdef DEBUGM
    Serial.printf("Minutes: %x %x\r\n", minut[0], minut[1]);
  #endif  
  sensorData.minutesSinceStart = minut[0]*256 + minut[1]; 
  sensorData.minutesHistoryOffset =(sensorData.minutesSinceStart-3) % 15 +3 ;   
  int index = 0;
  for (int i = 0; i < 16; i++)                                // 16 bloków co 1 minutę
  {
    index = 4 + (sensorData.nextTrend - 1 - i) * 6;
    if (index < 4) index = index + 96;
    byte pomiar[6];
    for (int k = index; k < index + 6; k++) pomiar[k - index] = sensorDataBody[k];    
    sensorData.trend[i] = ((pomiar[1] << 8) & 0x0F00) + pomiar[0];
//    Int16 rawTmp = Convert.ToInt16(((Convert.ToInt16(pomiar[1]) << 8) & 0x0F00) + Convert.ToInt16(pomiar[0]));
//    int rawBGtemp = ((Convert.ToInt16(pomiar[5]) << 8) & 0x0F) + Convert.ToInt16(pomiar[4]);
//    int rawBGhugo = Convert.ToInt16(pomiar[3]);
  }
  index = 0;
  for (int i = 0; i < 32; i++)                                // 32 bloki co 15 minut
  {
    index = 100 + (sensorData.nextHistory - 1 - i) * 6;
    if (index < 100) index = index + 192;
    byte pomiar[6];
    for (int k = index; k < index + 6; k++) pomiar[k - index] = sensorDataBody[k];    
    sensorData.history[i] = ((pomiar[1] << 8) & 0x0F00) + pomiar[0];
//    Int16 rawTmp = Convert.ToInt16(((Convert.ToInt16(pomiar[1]) << 8) & 0x0F00) + Convert.ToInt16(pomiar[0]));
//    int rawBGtemp = ((Convert.ToInt16(pomiar[5]) << 8) & 0x0F) + Convert.ToInt16(pomiar[4]);
//    int rawBGhugo = Convert.ToInt16(pomiar[3]);
  }                        
}
void decodeSensorFooter()
{
  
}

void displaySensorData()
{
  #ifdef DEBUGM
    if (!sensorData.sensorDataOK) Serial.println("Sensor data error");
    else 
    {
      Serial.println("Sensor data OK.");
      Serial.print("Sensor s/n: ");
      Serial.println(sensorData.sensorSN);
      Serial.print("Sensor status: ");
      Serial.print(sensorData.sensorStatusByte, HEX);
      Serial.print(" - ");
      Serial.println(sensorData.sensorStatusString);
      Serial.print("Next trend position: ");
      Serial.println(sensorData.nextTrend);
      Serial.print("Next history position: ");
      Serial.println(sensorData.nextHistory);
      Serial.print("Minutes since sensor start: ");
      Serial.println(sensorData.minutesSinceStart);
      Serial.print("Minutes trend to history offset: ");
      Serial.println(sensorData.minutesHistoryOffset);
      Serial.print("BG trend: ");
      for (int i=0; i<16; i++) Serial.printf(" %f",(float)(sensorData.trend[i]/10.0f));
      Serial.println("");
      Serial.print("BG history: ");
      for (int i=0; i<32; i++) Serial.printf(" %f",(float)(sensorData.history[i]/10.0f));
      Serial.println("");
    }   
  #endif
}


