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
#include "EasyBuzzer.h"

#ifndef DOCSIZE
  #define DOCSIZE 128
#endif
#define countof(a) (sizeof(a) / sizeof(a[0]))
#define COMPILED __DATE__ " " __TIME__
#define BOARD "nanoatmega328new"

struct Led
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
  bool isBlink;
  int32_t blinkCount;
  uint16_t blinkDelay;
  uint32_t lastRun;
  uint8_t lastState = 1;
};

struct Buzzer
{
  int32_t beepCount;
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
  uint8_t ledON = 255;
};

class libudawaatmega328
{
  public:
    libudawaatmega328();
    void begin();
    void execute();
    int getFreeHeap(uint16_t min,uint16_t max);
    int getFreeHeap();
    void setConfig(StaticJsonDocument<DOCSIZE> &doc);
    void setPin(StaticJsonDocument<DOCSIZE> &doc);
    int getPin(StaticJsonDocument<DOCSIZE> &doc);
    void setRgbLed(StaticJsonDocument<DOCSIZE> &doc);
    void setBuzzer(StaticJsonDocument<DOCSIZE> &doc);
    void serialWriteToESP32(StaticJsonDocument<DOCSIZE> &doc);
    StaticJsonDocument<DOCSIZE> serialReadFromESP32();
    void runRgbLed();
    void runBuzzer();
    void runPanic();
    void coMCUGetInfo(StaticJsonDocument<DOCSIZE> &doc);
    void serialHandler(StaticJsonDocument<DOCSIZE> &doc);
    ConfigCoMCU config;

  private:
    bool _toBoolean(String &value);
    void _serialCommandHandler(HardwareSerial &serial);
    Led _rgb;
    Buzzer _buzzer;
};



libudawaatmega328::libudawaatmega328()
{

}

void libudawaatmega328::begin()
{
  Serial.begin(115200);
  Serial.setTimeout(30000);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  EasyBuzzer.setPin(config.pinBuzzer);
}

void libudawaatmega328::execute()
{
  EasyBuzzer.update();
  runBuzzer();
  runRgbLed();
  runPanic();
}

void libudawaatmega328::coMCUGetInfo(StaticJsonDocument<DOCSIZE> &doc)
{
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
    const auto deser_err = deserializeJson(doc, Serial);;

    if (deser_err)
    {
      doc["err"] = 1;
      Log.error(F("SerialCoMCU DeserializeJson() returned: %s" CR), deser_err.c_str());
    }
    else
    {
      const char* method = doc["method"].as<const char*>();
      if(strcmp(method, (const char*) "gInf") == 0)
      {
        coMCUGetInfo(doc);
      }
      else if(strcmp(method, (const char*) "sCfg") == 0)
      {
        setConfig(doc);
      }
      else if(strcmp(method, (const char*) "sLed") == 0)
      {
        setRgbLed(doc);
      }
      else if(strcmp(method, (const char*) "sBuz") == 0)
      {
        setBuzzer(doc);
      }
      else if(strcmp(method, (const char*) "sPin") == 0)
      {
        setPin(doc);
      }
      else if(strcmp(method, (const char*) "gPin") == 0)
      {
        getPin(doc);
      }
      //serializeJsonPretty(doc, Serial);
    }
  }
  return doc;
}

void libudawaatmega328::setConfig(StaticJsonDocument<DOCSIZE> &doc)
{
  if(doc["fPanic"] != nullptr){config.fPanic = doc["fPanic"].as<uint8_t>();}

  if(doc["bfreq"] != nullptr){config.bfreq = doc["bfreq"].as<uint16_t>();}
  if(doc["fBuzz"] != nullptr){config.fBuzz = doc["fBuzz"].as<uint8_t>();}

  if(doc["pinBuzzer"] != nullptr){config.pinBuzzer = doc["pinBuzzer"].as<uint8_t>();}
  if(doc["pinLedR"] != nullptr){config.pinLedR = doc["pinLedR"].as<uint8_t>();}
  if(doc["pinLedG"] != nullptr){config.pinLedG = doc["pinLedG"].as<uint8_t>();}
  if(doc["pinLedB"] != nullptr){config.pinLedB = doc["pinLedB"].as<uint8_t>();}

  if(doc["ledON"] != nullptr){config.ledON = doc["ledON"].as<uint8_t>();}

  EasyBuzzer.setPin(config.pinBuzzer);
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
  _rgb.blinkCount = doc["params"]["blinkCount"].as<int32_t>();
  _rgb.blinkDelay = doc["params"]["blinkDelay"].as<uint16_t>();
}

void libudawaatmega328::setBuzzer(StaticJsonDocument<DOCSIZE>& doc)
{
  _buzzer.beepCount = doc["params"]["beepCount"].as<int32_t>();
  _buzzer.beepDelay = doc["params"]["beepDelay"].as<uint16_t>();
}

void libudawaatmega328::runRgbLed()
{
  uint32_t now = millis();
  if(!_rgb.isBlink)
  {
    analogWrite(config.pinLedR, _rgb.r);
    analogWrite(config.pinLedG, _rgb.g);
    analogWrite(config.pinLedB, _rgb.b);
  }
  else if(_rgb.blinkCount > 0 && now - _rgb.lastRun > _rgb.blinkDelay){
    if(_rgb.lastState == 1){
      analogWrite(config.pinLedR, _rgb.r);
      analogWrite(config.pinLedG, _rgb.g);
      analogWrite(config.pinLedB, _rgb.b);

      _rgb.blinkCount--;
    }
    else{
      analogWrite(config.pinLedR, config.ledON == 0 ? 255 : 0);
      analogWrite(config.pinLedG, config.ledON == 0 ? 255 : 0);
      analogWrite(config.pinLedB, config.ledON == 0 ? 255 : 0);
    }

    _rgb.lastState = !_rgb.lastState;
    _rgb.lastRun = now;
  }
  else if(_rgb.blinkCount == -1 && now - _rgb.lastRun > _rgb.blinkDelay){
    if(_rgb.lastState == 1){
      analogWrite(config.pinLedR, _rgb.r);
      analogWrite(config.pinLedG, _rgb.g);
      analogWrite(config.pinLedB, _rgb.b);
    }
    else{
      analogWrite(config.pinLedR, config.ledON == 0 ? 255 : 0);
      analogWrite(config.pinLedG, config.ledON == 0 ? 255 : 0);
      analogWrite(config.pinLedB, config.ledON == 0 ? 255 : 0);
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
      tone(config.pinBuzzer, config.bfreq, _buzzer.beepDelay);
      _buzzer.beepCount--;
    }
    else{
      noTone(config.pinBuzzer);
    }

    _buzzer.lastState = !_buzzer.lastState;
    _buzzer.lastRun = now;
  }
  else if(_buzzer.beepCount == -1 && now - _buzzer.lastRun > _buzzer.beepDelay)
  {
    if(_buzzer.lastState == 1){
      tone(config.pinBuzzer, config.bfreq, _buzzer.beepDelay);
    }
    else{
      noTone(config.pinBuzzer);
    }

    _buzzer.lastState = !_buzzer.lastState;
    _buzzer.lastRun = now;
  }
}

void libudawaatmega328::runPanic()
{
  if(config.fPanic)
  {
    _rgb.r = config.ledON;
    _rgb.g = config.ledON == 0 ? 255 : 0;
    _rgb.b = config.ledON == 0 ? 255 : 0;
    _rgb.isBlink = 1;
    _rgb.blinkCount = 10;
    _rgb.blinkDelay = 100;

    _buzzer.beepCount = 10;
    _buzzer.beepDelay = 100;
  }
}

#endif
