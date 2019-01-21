#include <Arduino.h>
#include <PubSubClient.h>
#include "ESPBASE.h"
#include "relay.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);
const byte mqttDebug = 1;
//const int ESP_BUILTIN_LED = 1;
//#define RELAY1_PIN 12
//#define RELAY2_PIN 13
#define STATUS_LED 16
//byte relay1state = 0;
//byte relay2state = 0;
relay* Relay[2];
String sChipID;
long lastReconnectAttempt = 0;
unsigned long lastschedulecheck = 0;
int rbver = 0;
String RelayTopic;
String StatusTopic;
unsigned long reporttimemilli = 600000;
unsigned long lastreporttime = 0;
String ScheduleTopic;
bool scheduleon[2];

ESPBASE Esp;
void sendStatus();

void setup() {
  Serial.begin(115200);
  char cChipID[10];
  sprintf(cChipID,"%08X",ESP.getChipId());
  sChipID = String(cChipID);

  Esp.initialize();
  RelayTopic = String(DEVICE_TYPE) + "/" + config.DeviceName + "/command";
  StatusTopic = String(DEVICE_TYPE) + "/" + config.DeviceName + "/status";
  ScheduleTopic = String(DEVICE_TYPE) + "/" + config.DeviceName + "/schedule";
  Relay[0] = new relay(12,config.Relay1Name);
  Relay[1] = new relay(13,config.Relay2Name);
  Esp.setupMQTTClient();
  customWatchdog = millis();

  for(int r=0;r<2;r++)
    scheduleon[r] = false;

  Serial.println("Done with setup");
  Serial.println(config.ssid);
}

void setSchedule(int relay)
{
  if(suntime.valid && UnixTimestamp > 100000)
  {
    long tmnow = (DateTime.hour*60+DateTime.minute) * 60;
    Serial.println("now " + String(DateTime.hour) + ":" + String(DateTime.minute));
    long nexton = 86401;
    int noshed = -1;
  //  Serial.println(String(DateTime.hour) + ":"+String(DateTime.minute));
    for(int i=0;i<10;i++)
    {
      if(i < 2)
      {
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
      if(config.RSchedule[relay][i].wdays[DateTime.wday-1])
      {
        if(Relay[relay]->getonTime() == 0 && Relay[relay]->getoffTime() == 0)
        {
          long tmschedon = (config.RSchedule[relay][i].onHour*60+config.RSchedule[relay][i].onMin) * 60;
          if(tmschedon > tmnow)
          {
            tmschedon = tmschedon - tmnow;
            if(tmschedon < nexton)
            {
              Serial.println("relay " + String(relay) + " schedule " + String(i) + " " + String(config.RSchedule[relay][i].onHour) + ":" + String(config.RSchedule[relay][i].onMin) + " " + String(tmschedon));
              nexton = tmschedon;
              noshed = i;
            }
          }
        } 
      }
    }
    if(nexton < 86401)
    {
      Relay[relay]->setonTime(nexton);
      long offseconds = (config.RSchedule[relay][noshed].offHour*60+config.RSchedule[relay][noshed].offMin) * 60;
      Serial.println(String(config.RSchedule[relay][noshed].offHour) + ":" + String(config.RSchedule[relay][noshed].offMin) + " " + String(offseconds));
      offseconds = offseconds - tmnow;
      Serial.println(String(offseconds));
      if(offseconds < nexton)
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
  Esp.loop();
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
//      String sched = "Relay " + String(i) + " on " + String(Relay[i]->getonTime()) + " off " + String(Relay[i]->getoffTime());
//      Serial.println(sched);
//      Esp.mqttSend(String(DEVICE_TYPE) + "/" + config.DeviceName + "/schedule",sched," now " + String((DateTime.hour*60+DateTime.minute)*60));
//      if(scheduleShouldBeOn(i))
//      {
//        if(!scheduleon[i])
//        {
//          Relay[i]->setState(1);
//          scheduleon[i] = true;
//        }
//      }
//      else if(scheduleon[i])
//      {
//        Relay[i]->setState(0);
//        scheduleon[i] = false;
//      }
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

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char c_payload[length];
  memcpy(c_payload, payload, length);
  c_payload[length] = '\0';
  
  String s_topic = String(topic);
  String s_payload = String(c_payload);

  if(s_topic == "SendStat")
  {
    sendStatus();
  }
  if (s_topic == RelayTopic || s_topic == "AllLights" || s_topic == "computer/timer/event") 
  {
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
    }
    if(s_payload == "OFF_1" || s_payload == "OFF")
    {
      Relay[0]->setState(0);
    }
    if(s_payload == "ON_2" || s_payload == "ON")
    {
      Relay[1]->setState(1);
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
}
