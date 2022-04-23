#include <Arduino.h>

#define CS_INIT 0
#define CS_PAUSED 1
#define CS_ON 2
#define CS_OFF 3

class statusIndicator
{
    byte status=0;
    unsigned int pin;
    bool ledon = false;
    unsigned long onduration = 0;
    unsigned long offduration = 0;
    unsigned long pauseduration = 0;
    unsigned int numflashes = 0;
    unsigned long lastmillis = 0;
    unsigned int curstate = CS_INIT;
    unsigned int curflash = 0;
public:
    statusIndicator(int Pin);
    void setStatus(byte State); 
    void loop();
};

statusIndicator::statusIndicator(int Pin)
{
  pin=Pin;
  pinMode(pin,OUTPUT);
  digitalWrite(pin,LOW);
}

void statusIndicator::setStatus(byte State)
{
  status = State;
  if(status == 0)
  {
      onduration = 0;
      offduration = 0;
      pauseduration = 0;
      numflashes = 0;
  }  
  if(status == 1)
  {
      onduration = 500;
      offduration = 1500;
      pauseduration = 0;
      numflashes = 1;
  }
  if(status == 2)
  {
      onduration = 50;
      offduration = 100;
      pauseduration = 0;
      numflashes = 1;
  }
  if(status == 3)
  {
      onduration = 50;
      offduration = 50;
      pauseduration = 2000;
      numflashes = 3;
  }
  if(status == 4)
  {
      onduration = 50;
      offduration = 50;
      pauseduration = 2000;
      numflashes = 2;
  }
}

void statusIndicator::loop()
{
    if(status == 0)
        return;
    if(curstate == CS_INIT)
    {
        curstate = CS_ON;
        digitalWrite(pin,HIGH);
        lastmillis = millis();
    }
    else if(curstate == CS_ON)
    {
        if(millis() > lastmillis + onduration)
        {
            if(numflashes > 1 && curflash > numflashes)
            {
                curstate = CS_PAUSED;
            }
            else
            {
                curstate = CS_OFF;
            }
            digitalWrite(pin,LOW);
            curflash++;
            lastmillis = millis();
        }
    }
    else if(curstate == CS_OFF)
    {
        if(millis() > lastmillis + offduration)
        {
            curstate = CS_ON;
            digitalWrite(pin,HIGH);
            lastmillis = millis();
        }
    }
    else if(curstate == CS_PAUSED)
    {
        if(millis() > lastmillis + pauseduration)
        {
            curstate = CS_OFF;
            curflash = 0;
            lastmillis = millis();
        }
    }
}