/**
 * UDAWA - Universal Digital Agriculture Watering Assistant
 * Function helper library for ATMega328 based Co-MCU firmware development
 * Licensed under aGPLv3
 * Researched and developed by PRITA Research Group & Narin Laboratory
 * prita.undiknas.ac.id | narin.co.id
**/
#ifndef libudawaatmega328_h
#define libudawaatmega328_h

#include "Arduino.h"
#include <ArduinoJson.h>
#include <ArduinoLog.h>

#define DOCSIZE 128
#define countof(a) (sizeof(a) / sizeof(a[0]))
#define COMPILED __DATE__ " " __TIME__
#define BOARD "nanoatmega328new"

struct Led
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
  bool isBlink;
  uint16_t blinkCount;
  uint16_t blinkDelay;
  uint32_t lastRun;
  uint8_t lastState = 1;
  uint8_t off = 255;
  uint8_t on = 0;
};

struct Buzzer
{
  uint16_t beepCount;
  uint16_t beepDelay;
  uint32_t lastRun;
  bool lastState;
};

struct ConfigCoMCU
{
  bool fPanic = false;
  uint16_t bfreq = 1600;
  bool fBuzz = true;
  uint8_t pinBuzzer = 2;
  uint8_t pinLedR = 3;
  uint8_t pinLedG = 5;
  uint8_t pinLedB = 6;
};

class libudawaatmega328
{
  public:
    libudawaatmega328();
    void begin();
    void execute();
    int getFreeHeap(uint16_t min,uint16_t max);
    int getFreeHeap();
    void setConfigCoMCU(StaticJsonDocument<DOCSIZE> &doc);
    void setPin(StaticJsonDocument<DOCSIZE> &doc);
    int getPin(StaticJsonDocument<DOCSIZE> &doc);
    void setRgbLed(StaticJsonDocument<DOCSIZE> &doc);
    void setBuzzer(StaticJsonDocument<DOCSIZE> &doc);
    void serialWriteToESP32(StaticJsonDocument<DOCSIZE> &doc);
    StaticJsonDocument<DOCSIZE> serialReadFromESP32();
    void setPanic(StaticJsonDocument<DOCSIZE> &doc);
    void runRgbLed();
    void runBuzzer();
    void runPanic();
    void coMCUGetInfo(StaticJsonDocument<DOCSIZE> &doc);
    void serialHandler(StaticJsonDocument<DOCSIZE> &doc);
    ConfigCoMCU configcomcu;
  private:
    bool _toBoolean(String &value);
    void _serialCommandHandler(HardwareSerial &serial);
    Led _rgb;
    Buzzer _buzzer;
};



libudawaatmega328::libudawaatmega328()
{
  _buzzer.lastState = 1;
}

void libudawaatmega328::begin()
{
  Serial.begin(115200);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);

}

void libudawaatmega328::execute()
{
  runRgbLed();
  runBuzzer();
  runPanic();
}

void libudawaatmega328::coMCUGetInfo(StaticJsonDocument<DOCSIZE> &doc)
{
  doc["method"] = "coMCUSetInfo";
  doc["board"] = BOARD;
  doc["fwv"] = COMPILED;
  doc["heap"] = getFreeHeap();
  serialWriteToESP32(doc);
}

int libudawaatmega328::getFreeHeap(uint16_t min,uint16_t max)
{
  if (min==max-1)
  {
    return min;
  }
  int size=max;
  int lastSize=size;
  byte *buf;
  while ((buf = (byte *) malloc(size)) == NULL)
  {
    lastSize=size;
    size-=(max-min)/2;
  };

  free(buf);
  return getFreeHeap(size,lastSize);
}

int libudawaatmega328::getFreeHeap()
{
    return getFreeHeap(0,4096);
}

void libudawaatmega328::serialWriteToESP32(StaticJsonDocument<DOCSIZE> &doc)
{
  serializeJson(doc, Serial);
}

StaticJsonDocument<DOCSIZE> libudawaatmega328::serialReadFromESP32()
{
  StaticJsonDocument<DOCSIZE> doc;

  if (Serial.available())
  {
    DeserializationError err = deserializeJson(doc, Serial);

    if (err == DeserializationError::Ok)
    {
      const char* method = doc["method"].as<const char*>();
      if(strcmp(method, (const char*) "coMCUGetInfo") == 0)
      {
        coMCUGetInfo(doc);
      }
      else if(strcmp(method, (const char*) "setConfigCoMCU") == 0)
      {
        setConfigCoMCU(doc);
      }
      else if(strcmp(method, (const char*) "setRgbLed") == 0)
      {
        setRgbLed(doc);
      }
      else if(strcmp(method, (const char*) "setBuzzer") == 0)
      {
        setBuzzer(doc);
      }
      else if(strcmp(method, (const char*) "setPin") == 0)
      {
        setPin(doc);
      }
      else if(strcmp(method, (const char*) "getPin") == 0)
      {
        getPin(doc);
      }
      else if(strcmp(method, (const char*) "setPanic") == 0)
      {
        setPanic(doc);
      }
      //serializeJson(doc, Serial);
    }
    else
    {
      doc["err"] = 1;
      //Log.error(F("SerialCoMCU DeserializeJson() returned: %s" CR), err.c_str());
    }
  }
  return doc;
}

void libudawaatmega328::setConfigCoMCU(StaticJsonDocument<DOCSIZE> &doc)
{
  if(doc["fPanic"] != nullptr){configcomcu.fPanic = doc["fPanic"].as<bool>();}

  if(doc["bfreq"] != nullptr){configcomcu.bfreq = doc["bfreq"].as<uint16_t>();}
  if(doc["fBuzz"] != nullptr){configcomcu.fBuzz = doc["fBuzz"].as<bool>();}

  if(doc["pinBuzzer"] != nullptr){configcomcu.pinBuzzer = doc["pinBuzzer"].as<uint8_t>();}
  if(doc["pinLedR"] != nullptr){configcomcu.pinLedR = doc["pinLedR"].as<uint8_t>();}
  if(doc["pinLedG"] != nullptr){configcomcu.pinLedG = doc["pinLedG"].as<uint8_t>();}
  if(doc["pinLedB"] != nullptr){configcomcu.pinLedB = doc["pinLedB"].as<uint8_t>();}
}

void libudawaatmega328::setPin(StaticJsonDocument<DOCSIZE> &doc)
{
  pinMode(doc["params"]["pin"].as<uint8_t>(), doc["params"]["mode"].as<uint8_t>());
  if(doc["params"]["op"].as<uint8_t>() == 0){
    analogWrite(doc["params"]["pin"].as<uint8_t>(), doc["params"]["aval"].as<uint16_t>());
  }
  else if(doc["params"]["op"].as<uint8_t>() == 1){
    digitalWrite(doc["params"]["pin"].as<uint8_t>(), doc["params"]["state"].as<uint8_t>());
  }
}

int libudawaatmega328::getPin(StaticJsonDocument<DOCSIZE> &doc)
{
  int state;
  if(doc["params"]["op"].as<uint8_t>() == 0)
  {
    state = analogRead(doc["params"]["pin"].as<uint8_t>());
    doc["params"]["state"] = state;
  }
  else if(doc["params"]["op"].as<uint8_t>() == 1)
  {
    state = digitalRead(doc["params"]["pin"].as<uint8_t>());
    doc["params"]["state"] = state;
  }
  serialWriteToESP32(doc);
  return state;
}

void libudawaatmega328::setRgbLed(StaticJsonDocument<DOCSIZE> &doc)
{
  _rgb.r = doc["params"]["r"].as<uint8_t>();
  _rgb.g = doc["params"]["g"].as<uint8_t>();
  _rgb.b = doc["params"]["b"].as<uint8_t>();
  _rgb.isBlink = doc["params"]["isBlink"].as<uint8_t>();
  _rgb.blinkCount = doc["params"]["blinkCount"].as<uint16_t>();
  _rgb.blinkDelay = doc["params"]["blinkDelay"].as<uint16_t>();
  _rgb.off = doc["params"]["off"].as<uint8_t>();
  _rgb.on = doc["params"]["on"].as<uint8_t>();
}

void libudawaatmega328::setBuzzer(StaticJsonDocument<DOCSIZE>& doc)
{
  _buzzer.beepCount = doc["params"]["beepCount"].as<uint16_t>();
  _buzzer.beepDelay = doc["params"]["beepDelay"].as<uint16_t>();
}

void libudawaatmega328::runRgbLed()
{
  uint32_t now = millis();
  if(!_rgb.isBlink)
  {
    analogWrite(configcomcu.pinLedR, _rgb.r);
    analogWrite(configcomcu.pinLedG, _rgb.g);
    analogWrite(configcomcu.pinLedB, _rgb.b);
  }
  else if(_rgb.blinkCount > 0 && now - _rgb.lastRun > _rgb.blinkDelay){
    if(_rgb.lastState == 1){
      analogWrite(configcomcu.pinLedR, _rgb.r);
      analogWrite(configcomcu.pinLedG, _rgb.g);
      analogWrite(configcomcu.pinLedB, _rgb.b);

      _rgb.blinkCount--;
    }
    else{
      analogWrite(configcomcu.pinLedR, _rgb.off);
      analogWrite(configcomcu.pinLedG, _rgb.off);
      analogWrite(configcomcu.pinLedB, _rgb.off);
    }

    _rgb.lastState = !_rgb.lastState;
    _rgb.lastRun = now;
  }
}

void libudawaatmega328::runBuzzer()
{
  uint32_t now = millis();
  if(_buzzer.beepCount > 0 && now - _buzzer.lastRun > _buzzer.beepDelay)
  {
    if(_buzzer.lastState == 1){
      tone(configcomcu.pinBuzzer, configcomcu.bfreq, _buzzer.beepDelay);
      _buzzer.beepCount--;
    }
    else{
      noTone(configcomcu.pinBuzzer);
    }

    _buzzer.lastState = !_buzzer.lastState;
    _buzzer.lastRun = now;
  }
}

void libudawaatmega328::runPanic()
{
  if(configcomcu.fPanic)
  {
    _rgb.r = 0;
    _rgb.g = 255;
    _rgb.b = 255;
    _rgb.isBlink = 1;
    _rgb.blinkCount = 10;
    _rgb.blinkDelay = 100;

    _buzzer.beepCount = 10;
    _buzzer.beepDelay = 100;
  }
}

void libudawaatmega328::setPanic(StaticJsonDocument<DOCSIZE> &doc)
{
  configcomcu.fPanic = doc["params"]["state"].as<bool>();
}

#endif
