#ifndef CurrentSensor_h
#define CurrentSensor_h
#include <Arduino.h> 
#include "ACS712.h"

class CurrentSensor{

  public:
    CurrentSensor(int pin);
    float getCurrent();
  private:
    ACS712  ACS;
    int _pin;

    float weight;
    int loops;
    int balance;
    float avrOld;

    float adcValue;
    float returnCurrent;
    int getValue();
    void auto0();
};
#endif