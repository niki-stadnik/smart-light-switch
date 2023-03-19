#ifndef CurrentSensor_h
#define CurrentSensor_h
#include <Arduino.h> 
#include "ACS712.h"

class CurrentSensor{

  public:
    CurrentSensor(int pin);
    float getCurrent();
    void auto0();
  private:
    ACS712  ACS;
    int _pin;
    
    float value;
    float weight;
    int loops;
    int balance;
    float avrOld;

    float adcValue;
    float returnCurrent;
    int getValue();
};
#endif