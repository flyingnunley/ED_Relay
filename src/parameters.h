#ifndef PARAMETERS_H
#define PARAMETERS_H
#define CFGVER "R1.1"
#define PROGMEM_LATE __attribute__ (( __section__(".fini1") ))

typedef struct strSched 
{
  int onHour;
  int onMin;
  int offHour;
  int offMin;
  bool wdays[7];
  bool onatsunset;
  bool offatsunrise;
} Sched;

struct strConfig {
  boolean dhcp;                         // 1 Byte - EEPROM 16
  boolean isDayLightSaving;             // 1 Byte - EEPROM 17
  long Update_Time_Via_NTP_Every;       // 4 Byte - EEPROM 18
  long timeZone;                        // 4 Byte - EEPROM 22
  byte  IP[4];                          // 4 Byte - EEPROM 32
  byte  Netmask[4];                     // 4 Byte - EEPROM 36
  byte  Gateway[4];                     // 4 Byte - EEPROM 40
  String ssid;                          // up to 32 Byte - EEPROM 64
  String password;                      // up to 32 Byte - EEPROM 96
  String ntpServerName;                 // up to 32 Byte - EEPROM 128
  String DeviceName;                    // up to 32 Byte - EEPROM 160
  String OTApwd;                         // up to 32 Byte - EEPROM 192
  String MQTTServer;                    // up to 32 Byte - EEPROM 224
  long MQTTPort;                        // 4 Byte - EEPROM 256
  long HeartbeatEvery;                  // 4 Byte - EEPROM 260
  boolean ReportTime;                   // 1 Byte - EEPROM 264
  // Application Settings here... from EEPROM 263 up to 511 (0 - 511)
  boolean R1OffAtSunrise;               // 1 byte - EEPROM 265
  boolean R1OnAtSunset;                   // 1 Byte - EEPROM 266
  boolean R2OffAtSunrise;               // 1 Byte - EEPROM 267
  boolean R2OnAtSunset;                 // 1 Byte - EEPROM 268
  String Relay1Name;                 // up to 16 Byte - EEPROM 349
  String Relay2Name;                 // up to 16 Byte - EEPROM 365
  Sched RSchedule[2][10];                     // 40 Bytes EEPROM 269
} config;

long packSched(Sched shed)
{
  long packed = shed.onHour;
  packed = (((((packed << 6) + shed.onMin) << 5) + shed.offHour) << 6) + shed.offMin;
  packed = packed << 9 | ((shed.onatsunset<<8)+(shed.offatsunrise<<7)+(shed.wdays[6]<<6)+(shed.wdays[0]<<5)+(shed.wdays[1]<<4)+(shed.wdays[2]<<3)+(shed.wdays[3]<<2)+(shed.wdays[4]<<1)+(shed.wdays[5]));
  Serial.println(String(packed));
  return packed;
}

Sched unpackSched(long packed)
{
  Sched shed;

  Serial.println(String(packed));
  int days = packed & 0x01ff;
  shed.onatsunset = (days >> 8) & 1;
  shed.offatsunrise = (days >> 7) & 1;
  shed.wdays[6] = (days >> 6) & 1;
  shed.wdays[0] = (days >> 5) & 1;
  shed.wdays[1] = (days >> 4) & 1;
  shed.wdays[2] = (days >> 3) & 1;
  shed.wdays[3] = (days >> 2) & 1;
  shed.wdays[4] = (days >> 1) & 1;
  shed.wdays[5] = days & 1;
  shed.onHour = (packed >> 26) & 0x1f;
  shed.onMin = (packed >> 20) & 0x3f;
  shed.offHour = (packed >> 15) & 0x1f;
  shed.offMin = (packed >> 9) & 0x3f;
  return shed;
}

  //  Auxiliar function to handle EEPROM
  // EEPROM-parameters
  void EEPROMWriteint(int address, int value){
    byte two = (value & 0xFF);
    byte one = ((value >> 8) & 0xFF);

    //Write the 4 bytes into the eeprom memory.
    EEPROM.write(address, two);
    EEPROM.write(address + 1, one);
  }

  void EEPROMWritelong(int address, long value){
    byte four = (value & 0xFF);
    byte three = ((value >> 8) & 0xFF);
    byte two = ((value >> 16) & 0xFF);
    byte one = ((value >> 24) & 0xFF);

    //Write the 4 bytes into the eeprom memory.
    EEPROM.write(address, four);
    EEPROM.write(address + 1, three);
    EEPROM.write(address + 2, two);
    EEPROM.write(address + 3, one);
  }

  int EEPROMReadint(long address){
    //Read the 4 bytes from the eeprom memory.
    long two = EEPROM.read(address);
    long one = EEPROM.read(address + 1);

    //Return the recomposed long by using bitshift.
    return ((two << 0) & 0xFF) + ((one << 8) & 0xFFFF);
  }

  long EEPROMReadlong(long address){
    //Read the 4 bytes from the eeprom memory.
    long four = EEPROM.read(address);
    long three = EEPROM.read(address + 1);
    long two = EEPROM.read(address + 2);
    long one = EEPROM.read(address + 3);

    //Return the recomposed long by using bitshift.
    return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
  }

  void WriteStringToEEPROM(int beginaddress, String string){
    char  charBuf[string.length() + 1];
    string.toCharArray(charBuf, string.length() + 1);
    for (int t =  0; t < sizeof(charBuf); t++)
    {
      EEPROM.write(beginaddress + t, charBuf[t]);
    }
  }

  String  ReadStringFromEEPROM(int beginaddress){
    volatile byte counter = 0;
    char rChar;
    String retString = "";
    while (1)
    {
      rChar = EEPROM.read(beginaddress + counter);
      if (rChar == 0) break;
      if (counter > 31) break;
      counter++;
      retString.concat(rChar);

    }
    return retString;
  }

  void WriteConfig(){
//    Serial.println("Saving config");
    String cfgver = CFGVER;
    char ccfgver[5];
    cfgver.toCharArray(ccfgver,5);

    Serial.print("Writing Config ");
    Serial.println(ccfgver);
    EEPROM.write(0, ccfgver[0]);
    EEPROM.write(1, ccfgver[1]);
    EEPROM.write(2, ccfgver[2]);
    EEPROM.write(3, ccfgver[3]);

    EEPROM.write(16, config.dhcp);
    EEPROM.write(17, config.isDayLightSaving);

    EEPROMWritelong(18, config.Update_Time_Via_NTP_Every); // 4 Byte
    EEPROMWritelong(22, config.timeZone); // 4 Byte

    EEPROM.write(32, config.IP[0]);
    EEPROM.write(33, config.IP[1]);
    EEPROM.write(34, config.IP[2]);
    EEPROM.write(35, config.IP[3]);

    EEPROM.write(36, config.Netmask[0]);
    EEPROM.write(37, config.Netmask[1]);
    EEPROM.write(38, config.Netmask[2]);
    EEPROM.write(39, config.Netmask[3]);

    EEPROM.write(40, config.Gateway[0]);
    EEPROM.write(41, config.Gateway[1]);
    EEPROM.write(42, config.Gateway[2]);
    EEPROM.write(43, config.Gateway[3]);

    WriteStringToEEPROM(64, config.ssid);
    WriteStringToEEPROM(96, config.password);
    WriteStringToEEPROM(128, config.ntpServerName);
    WriteStringToEEPROM(160, config.DeviceName);
    WriteStringToEEPROM(192, config.OTApwd);
    WriteStringToEEPROM(224, config.MQTTServer);
    EEPROMWritelong(256, config.MQTTPort);
    EEPROMWritelong(260, config.HeartbeatEvery);
    EEPROM.write(264,config.ReportTime);    
    EEPROM.write(265, config.R1OffAtSunrise);
    EEPROM.write(266,config.R1OnAtSunset);         
    EEPROM.write(267, config.R2OffAtSunrise);
    EEPROM.write(268,config.R2OnAtSunset);         
    for(int i=0;i<10;i++)
    {
      EEPROMWritelong(269+(i*4),packSched(config.RSchedule[0][i]));
      EEPROMWritelong(309+(i*4),packSched(config.RSchedule[1][i]));
    }
    WriteStringToEEPROM(349, config.Relay1Name);                 // up to 16 Byte - EEPROM 292
    WriteStringToEEPROM(365, config.Relay2Name);                 // up to 16 Byte - EEPROM 308

      // Application Settings here... from EEPROM 392 up to 511 (0 - 511)

    EEPROM.commit();
  }

  boolean ReadConfig(){
    Serial.println("Reading Configuration");
    String cfgver = CFGVER;
    char ccfgver[5];
    cfgver.toCharArray(ccfgver,5);
    if (EEPROM.read(0) == cfgver[0] && EEPROM.read(1) == cfgver[1] && EEPROM.read(2) == cfgver[2] && EEPROM.read(3) == cfgver[3] )
    {
      Serial.println("Configurarion Found!");
      config.dhcp = 	EEPROM.read(16);
      config.isDayLightSaving = EEPROM.read(17);
      config.Update_Time_Via_NTP_Every = EEPROMReadlong(18); // 4 Byte
      config.timeZone = EEPROMReadlong(22); // 4 Byte
      config.IP[0] = EEPROM.read(32);
      config.IP[1] = EEPROM.read(33);
      config.IP[2] = EEPROM.read(34);
      config.IP[3] = EEPROM.read(35);
      config.Netmask[0] = EEPROM.read(36);
      config.Netmask[1] = EEPROM.read(37);
      config.Netmask[2] = EEPROM.read(38);
      config.Netmask[3] = EEPROM.read(39);
      config.Gateway[0] = EEPROM.read(40);
      config.Gateway[1] = EEPROM.read(41);
      config.Gateway[2] = EEPROM.read(42);
      config.Gateway[3] = EEPROM.read(43);
      config.ssid = ReadStringFromEEPROM(64);
      config.password = ReadStringFromEEPROM(96);
      config.ntpServerName = ReadStringFromEEPROM(128);
      config.DeviceName = ReadStringFromEEPROM(160);
      config.OTApwd = ReadStringFromEEPROM(192);
      config.MQTTServer = ReadStringFromEEPROM(224);
      config.MQTTPort = EEPROMReadlong(256);
      config.HeartbeatEvery = EEPROMReadlong(260);
      config.ReportTime = EEPROM.read(264);                   // 1 Byte - EEPROM 262
      config.ReportTime = false;
      config.R1OffAtSunrise = EEPROM.read(265);
      config.R1OnAtSunset = EEPROM.read(266);                   // 1 Byte - EEPROM 261
      config.R2OffAtSunrise = EEPROM.read(267);
      config.R2OnAtSunset = EEPROM.read(268);                   // 1 Byte - EEPROM 261
      for(int i=0;i<10;i++)
      {
        config.RSchedule[0][i] = unpackSched(EEPROMReadlong(269+(i*4)));
        config.RSchedule[1][i] = unpackSched(EEPROMReadlong(309+(i*4)));
      }
      config.Relay1Name = ReadStringFromEEPROM(349);                 // up to 16 Byte - EEPROM 292
      config.Relay2Name = ReadStringFromEEPROM(365);                 // up to 16 Byte - EEPROM 308
      // Application parameters here ... from EEPROM 456 to 511

      return true;

    }
    else
    {
      Serial.println("Configurarion NOT FOUND!!!!");
      return false;
    }
  }

String formatConfig(){
  String outstring = "";
  outstring = outstring + "DHCP:"+String(config.dhcp) + "|";
  outstring = outstring + "DayLight:" + String(config.isDayLightSaving) + "|";
  outstring = outstring + "NTP update every " + String(config.Update_Time_Via_NTP_Every) + " sec" + "|";
  outstring = outstring + "Timezone:" + String(config.timeZone) + "|";
  outstring = outstring + "IP:" + String(config.IP[0]) + "." + String(config.IP[1]) + "." + String(config.IP[2]) + "." + String(config.IP[3]) + "|";
  outstring = outstring + "Mask:" + String(config.Netmask[0]) + "." + String(config.Netmask[1]) + "." + String(config.Netmask[2]) + "." + String(config.Netmask[3]) + "|";
  outstring = outstring + "Gateway:" + String(config.Gateway[0]) + "." + String(config.Gateway[1]) + "." + String(config.Gateway[2]) + "." + String(config.Gateway[3]) + "|";
  outstring = outstring + "SSID:" + config.ssid + "|";
  outstring = outstring + "PWD:" + config.password + "|";
  outstring = outstring + "ntp ServerName:" + config.ntpServerName + "|";
  outstring = outstring + "Device Name:" + config.DeviceName + "|";
  outstring = outstring + "OTA password" + config.OTApwd + "|";
  outstring = outstring + "MQTT Server:" + config.MQTTServer + "|";
  outstring = outstring + "MQTT Port:" + String(config.MQTTPort) + "|";
  outstring = outstring + "Heartbeat every " + String(config.HeartbeatEvery);
  return outstring;
}

void configLoadDefaults(uint16_t ChipId){

  #ifdef ARDUINO_ESP32_DEV
    config.ssid = "ESP32-" + String(ChipId,HEX);       // SSID of access point
  #elif ARDUINO_ESP8266_ESP01 || ARDUINO_ESP8266_NODEMCU
    config.ssid = "ESP8266-" + String(ChipId,HEX);       // SSID of access point
  #endif
  config.password = "" ;   // password of access point
  config.dhcp = true;
  config.IP[0] = 192; config.IP[1] = 168; config.IP[2] = 1; config.IP[3] = 100;
  config.Netmask[0] = 255; config.Netmask[1] = 255; config.Netmask[2] = 255; config.Netmask[3] = 0;
  config.Gateway[0] = 192; config.Gateway[1] = 168; config.Gateway[2] = 1; config.Gateway[3] = 254;
  config.ntpServerName = "0.ubuntu.pool.ntp.org"; //"0.ch.pool.ntp.org"; // to be adjusted to PT ntp.ist.utl.pt
  config.Update_Time_Via_NTP_Every =  5;
  config.timeZone = 1;
  config.isDayLightSaving = true;
  config.DeviceName = "Not Named";
  config.OTApwd = "";
  config.MQTTServer = "";
  config.MQTTPort = 0;
  config.HeartbeatEvery = 0;

  return;
}


#endif
