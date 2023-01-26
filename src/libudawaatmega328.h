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
    void coMCUGetInfo(StaticJsonDocument<DOCSIZE> &doc);
    void serialHandler(StaticJsonDocument<DOCSIZE> &doc);
    ConfigCoMCU configcomcu;

  private:
    bool _toBoolean(String &value);
    void _serialCommandHandler(HardwareSerial &serial);
};



libudawaatmega328::libudawaatmega328()
{
  RGBLed led(configcomcu.pinLedR, configcomcu.pinLedG, configcomcu.pinLedB, RGBLed::COMMON_CATHODE);
}

void libudawaatmega328::begin()
{
  Serial.begin(115200);
  Serial.setTimeout(30000);
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  EasyBuzzer.setPin(configcomcu.pinBuzzer);
}

void libudawaatmega328::execute()
{
  EasyBuzzer.update();
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
        setConfigCoMCU(doc);
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
      else if(strcmp(method, (const char*) "sPnc") == 0)
      {
        setPanic(doc);
      }
      //serializeJsonPretty(doc, Serial);
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

  EasyBuzzer.setPin(configcomcu.pinBuzzer);
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


}

void libudawaatmega328::setBuzzer(StaticJsonDocument<DOCSIZE>& doc)
{
  if(doc["st"].as<uint8_t>() == 1){
    EasyBuzzer.beep(
      configcomcu.bfreq,		// Frequency in hertz(HZ).
      doc["on"].as<uint32_t>(), 	// On Duration in milliseconds(ms).
      doc["of"].as<uint32_t>(), 	// Off Duration in milliseconds(ms).
      doc["bp"].as<uint32_t>(), 	// The number of beeps per cycle.
      doc["ps"].as<uint32_t>(), 	// Pause duration.
      doc["cy"].as<uint32_t>() 		// The number of cycle.
    );
  }else{
    EasyBuzzer.stopBeep();
  }
}


void libudawaatmega328::setPanic(StaticJsonDocument<DOCSIZE> &doc)
{
  configcomcu.fPanic = doc["params"]["state"].as<bool>();
  if(configcomcu.fPanic){
    EasyBuzzer.beep(
      configcomcu.bfreq,		// Frequency in hertz(HZ).
      100, 	// On Duration in milliseconds(ms).
      100, 	// Off Duration in milliseconds(ms).
      2, 			// The number of beeps per cycle.
      100, 	// Pause duration.
      0 		// The number of cycle.
    );
  }else{
    EasyBuzzer.stopBeep();
  }
}

#endif
