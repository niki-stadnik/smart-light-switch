#include "CurrentSensor.h"

CurrentSensor::CurrentSensor(int pin) : ACS(36, 3.0, 4095, 66){
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);

  ACS.autoMidPoint();

  Serial.print("MidPoint: ");
  Serial.println(ACS.getMidPoint());
  Serial.print("Noise mV: ");
  Serial.println(ACS.getNoisemV());
  Serial.print("Amp/Step: ");
  Serial.println(ACS.getAmperePerStep(), 4);

  auto0();
  digitalWrite(pin, LOW);
  _pin = pin;
  weight = 0.2;
  loops = 50;
  balance = 0;
  avrOld = 0;
  adcValue = 0.0f;
  returnCurrent = 0.0f;
}

float CurrentSensor::getCurrent(){
  adcValue = getValue();
  if ((adcValue - avrOld) > 1 || (avrOld - adcValue) > 1)
      avrOld = adcValue;
  else
      adcValue = avrOld;
  //The ESP's ADC when running at 3.3V has a resolution of (3.3/4095) 0.81mV per LSB.
  //That means that 0.81/66 gives you 0.012A or 12mA per LSB.
  //Therefore 10W lightbulb would be seen as 10W/220V=0.045A or 3-4 in analog.
  returnCurrent = adcValue * 12;
  Serial.print(" avr: ");
  Serial.print(adcValue, 0);
  Serial.println();
  Serial.println(returnCurrent);
  return returnCurrent;
}

int CurrentSensor::getValue(){
  int sum = 0;
  float value  = 0;
  for (int i = 0; i < 3; i++){
    float dummy = ACS.mA_AC_sampling(); //some dummy reads
  }
  for (int i = 0; i < loops; i++){
    float mA = ACS.mA_AC_sampling();
    value += weight * (mA - value);  // low pass filtering
    sum += value;
  }
  int avr = sum / loops;
  avr -= balance;
  Serial.print(" value: ");
  Serial.print(value);
  return avr;
}

void CurrentSensor::auto0(){
  float value = ACS.mA_AC();  // get good initial value
  int summ = 0;
  for (int i = 0; i<10; i++){
    summ += getValue();
  }
  balance = summ / 10;
}