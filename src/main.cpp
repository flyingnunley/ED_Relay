#include <Arduino.h>
#include <PubSubClient.h>
#include "ESPBASE.h"
#include "relay.h"
#include "statusIndicator.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);
const byte mqttDebug = 1;
#define RELAY1_PIN 12
#define RELAY2_PIN 13
#define STATUS_LED 16
bool statusledon = false;
relay* Relay[2];
String sChipID;
long lastReconnectAttempt = 0;         // used for a millisecond timer to reconnect to mqtt broker if disconnected
unsigned long lastschedulecheck = 0;  // used for a millisecond timer to check the schedule for relay changes every 60 seconds
String RelayTopic;
String StatusTopic;
unsigned long reporttimemilli = 600000;
unsigned long lastreporttime = 0;
String ScheduleTopic;
int loopcount = 0;
ESPBASE Esp;
void sendStatus();
unsigned int statusmode = 0;
statusIndicator* statflash;

void setup() {
  Serial.begin(115200);
  statflash = new statusIndicator(STATUS_LED);
  statflash->setStatus(2);
  char cChipID[10];
  sprintf(cChipID,"%08X",ESP.getChipId());
  sChipID = String(cChipID);

  Esp.initialize();
  pinMode(STATUS_LED,OUTPUT);
  RelayTopic = String(DEVICE_TYPE) + "/" + config.DeviceName + "/command";
  StatusTopic = String(DEVICE_TYPE) + "/" + config.DeviceName + "/status";
  ScheduleTopic = String(DEVICE_TYPE) + "/" + config.DeviceName + "/schedule";
  Relay[0] = new relay(RELAY1_PIN,config.Relay1Name,config.AutoOffSeconds);
  Relay[1] = new relay(RELAY2_PIN,config.Relay2Name,config.AutoOffSeconds);
  Esp.setupMQTTClient();
  customWatchdog = millis();

  Serial.println("Done with setup");
  Serial.println(config.ssid);
  if(!Esp.WIFI_connected)
    statflash->setStatus(2);
  else
    statflash->setStatus(1);
}

void setSchedule(int relay)
{
  if(suntime.valid && UnixTimestamp > 100000)
  {
    long tmnow = (DateTime.hour*60+DateTime.minute) * 60;
    Serial.println("now " + String(DateTime.hour) + ":" + String(DateTime.minute) + " tz " + config.timeZone);
    long nexton = 86401;
    int noshed = -1;
  //  Serial.println(String(DateTime.hour) + ":"+String(DateTime.minute));
    for(int i=0;i<10;i++)
    {
      if(i < 2)
      {
// for each of the scheduled times, set the on and off time to sunset/rise as configured.  
// only the first two schedules can be set this way.
        if(config.RSchedule[relay][i].onatsunset)
        {
          config.RSchedule[relay][i].onHour = suntime.setHour;
          config.RSchedule[relay][i].onMin = suntime.setMin;
        }  
        if(config.RSchedule[relay][i].offatsunrise)
        {
          config.RSchedule[relay][i].offHour = suntime.riseHour;
          config.RSchedule[relay][i].offMin = suntime.riseMin;
        }
      }  
      if(config.RSchedule[relay][i].wdays[DateTime.wday-1]) // if the weekday is selected in the schedule
      {
        if(Relay[relay]->getonTime() == 0 && Relay[relay]->getoffTime() == 0)  // if the relay is not set to a schedule (last scheduled time complete)
        {
          long tmschedon = (config.RSchedule[relay][i].onHour*60+config.RSchedule[relay][i].onMin) * 60; // get the on time for this schedule
          if(tmschedon > tmnow) // if its a future time
          {
            tmschedon = tmschedon - tmnow; // figure out now many seconds from now
            if(tmschedon < nexton) // if this schedule is more current than the other ones checked
            {
              Serial.println("relay " + String(relay) + " schedule " + String(i) + " " + String(config.RSchedule[relay][i].onHour) + ":" + String(config.RSchedule[relay][i].onMin) + " " + String(tmschedon));
              nexton = tmschedon; // set the next on time
              noshed = i; // keep track of the current schedule
            }
          }
        } 
      }
    } // once we get here, nexton is set to the closest on time in all the schedules
    if(nexton < 86401) // if its within the day
    {
      Relay[relay]->setonTime(nexton);  // set the next on time counter in the relay
      // now get the off time in the current schedule
      long offseconds = (config.RSchedule[relay][noshed].offHour*60+config.RSchedule[relay][noshed].offMin) * 60; 
      Serial.println(String(config.RSchedule[relay][noshed].offHour) + ":" + String(config.RSchedule[relay][noshed].offMin) + " " + String(offseconds));
      offseconds = offseconds - tmnow;
      Serial.println(String(offseconds));
      if(offseconds < nexton) // if the off seconds is less than the on seconds, its assumed that the off must be after midnight.
        offseconds = offseconds + 86400;  // add 24 hours
      Serial.println(offseconds);
      Relay[relay]->setoffTime(offseconds);
      Serial.println("Set relay " + String(relay) + " schedule to on " + String(Relay[relay]->getonTime()) + " off " + String(Relay[relay]->getoffTime()));
    }
  }
//  return false;
}

void loop() 
{
  loopcount++;
  Esp.loop();
  statflash->loop();
  if(!Esp.WIFI_connected)
    statflash->setStatus(2);
  else if(DateTime.year > 2050 || DateTime.year < 2018)
      statflash->setStatus(3);
    else if(!Esp.mqttClient->connected())
        statflash->setStatus(4);
        else
          statflash->setStatus(1);
  if(millis() > lastreporttime + reporttimemilli)
  {
    if(config.ReportTime)
    {
      Esp.mqttSend(String(DEVICE_TYPE) + "/" + config.DeviceName + "/time",String(DateTime.hour) + ":" + String(DateTime.minute) + ":" + String(DateTime.second),"");
      for(int i=0;i<2;i++)
      {
        String sched = "Relay " + String(i) + " on " + String(Relay[i]->getonTime()) + " off " + String(Relay[i]->getoffTime());
        Serial.println(sched);
        Esp.mqttSend(String(DEVICE_TYPE) + "/" + config.DeviceName + "/schedule",sched," now " + String((DateTime.hour*60+DateTime.minute)*60));
      }
    }
    lastreporttime = millis();
  }
  if(millis() > lastschedulecheck + 60000)
  {
    lastschedulecheck = millis();
    for(int i=0;i<2;i++)
    {
      setSchedule(i);
    }
  }
}

String getSignalString()
{
  String signalstring = "";
  byte available_networks = WiFi.scanNetworks();
  signalstring = signalstring + sChipID + ":";
 
  for (int network = 0; network < available_networks; network++) {
    String sSSID = WiFi.SSID(network);
    if(network > 0)
      signalstring = signalstring + ",";
    signalstring = signalstring + WiFi.SSID(network) + "=" + String(WiFi.RSSI(network));
  }
  return signalstring;    
}

void sendStatus()
{
  String message = "";
  for(int i=0;i<2;i++)
  {
    if(i==1)
      message = message + ":";
    message = message + Relay[i]->getName() + "," + String(i) + ",";
    if(Relay[i]->getState() == 1)
      message = message + "on";
    else
      message = message + "off";
  }
  Esp.mqttSend(StatusTopic,"",message);
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char c_payload[length];
  memcpy(c_payload, payload, length);
  c_payload[length] = '\0';
  int onseconds = 0;

  String s_topic = String(topic);
  String s_payload = String(c_payload);
  if(s_payload.indexOf(",") > 0)
  {
    String payload = getValue(s_payload,',',0);
    onseconds = getValue(s_payload,',',1).toInt();
  }

  if(s_topic == "SendStat")
  {
    sendStatus();
  }
  if (s_topic == RelayTopic || s_topic == "AllLights" || s_topic == "computer/timer/event") 
  {
    if(s_payload == "sendtime")
    {
      Esp.mqttSend(String(DEVICE_TYPE) + "/" + config.DeviceName + "/time",String(DateTime.hour) + ":" + String(DateTime.minute) + ":" + String(DateTime.second)," sunrise " + String(suntime.riseHour)+ ":" + String(suntime.riseMin) + " sunset " + String(suntime.setHour)+ ":" + String(suntime.setMin));
    }
    if(s_payload == "signal")
    {
      Esp.mqttSend(StatusTopic,sChipID," WiFi: " + getSignalString());
    }
    if(s_payload == "TOGGLE")
    {
      for(int i=0;i<2;i++)
      {
        Relay[i]->toggle();
      }
    }
    if(s_payload == "TOGGLE_1")
    {
      Relay[0]->toggle();
    }
    if(s_payload == "TOGGLE_2")
    {
      Relay[1]->toggle();
    }
    if(s_payload == "ON_1" || s_payload == "ON")
    {
      Relay[0]->setState(1);
      if(onseconds > 0)
      {
        if(Relay[0]->getoffTime() == 0)
        {
          Relay[0]->setoffTime(onseconds);
        }
      }
    }
    if(s_payload == "OFF_1" || s_payload == "OFF")
    {
      Relay[0]->setState(0);
    }
    if(s_payload == "ON_2" || s_payload == "ON")
    {
      Relay[1]->setState(1);
      if(onseconds > 0)
      {
        if(Relay[1]->getoffTime() == 0)
        {
          Relay[1]->setoffTime(onseconds);
        }
      }
    }
    if(s_payload == "OFF_2" || s_payload == "OFF")
    {
      Relay[1]->setState(0);
    }
  }
}

void mqttSubscribe()
{
    if (Esp.mqttClient->connected()) {
      if (Esp.mqttClient->subscribe(RelayTopic.c_str())) {
        Serial.println("Subscribed to " + RelayTopic);
        Esp.mqttSend(StatusTopic,"","Subscribed to " + RelayTopic);
        Esp.mqttSend(StatusTopic,verstr,","+Esp.MyIP()+" start");
      }
      if (Esp.mqttClient->subscribe("SendStat"))
      {
        Serial.println("Subscribed to SendStat");
      }
      if (Esp.mqttClient->subscribe("AllLights"))
      {
        Serial.println("Subscribed to AllLights");
      }
      if (Esp.mqttClient->subscribe("computer/timer/event"))
      {
        Serial.println("Subscribed to computer/timer/event");
      }
      char buff[100];
      ScheduleTopic.toCharArray(buff,100);
      if (Esp.mqttClient->subscribe((const char *)buff))
      {
        Serial.println("Subscribed to " + ScheduleTopic);
      }
    }
}

void mainTick()
{
  Relay[0]->tick();
  Relay[1]->tick();
  //if(statusledon)
  //{
  //  digitalWrite(STATUS_LED,LOW);
  //  statusledon = false;
  //}
  //else
  //{
  //  digitalWrite(STATUS_LED,HIGH);
  //  statusledon = true;
 // }
  //Serial.println(loopcount);
}
