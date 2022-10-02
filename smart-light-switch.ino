#include <ArduinoJson.h>
#include <WiFi.h>
#include <Adafruit_AHT10.h>

#include <VirtualWire.h>

#include "config.h"
const char* ssid = WIFI;
const char* password =  PASS;
const char * hostpi = HOSTPI;
const char * hosttest = HOSTTEST;
const char * host;
const uint16_t port = PORT;
char * key = KEY;

/*
#include "ACS712.h"

//  Arduino UNO has 5.0 volt with a max ADC value of 1023 steps
//  ACS712 5A  uses 185 mV per A
//  ACS712 20A uses 100 mV per A
//  ACS712 30A uses  66 mV per A
//ACS712  ACS(A0, 5.0, 1023, 100);
ACS712  ACS0(36, 5.0, 4095, 66);  //tr
ACS712  ACS1(39, 5.0, 4095, 66);  //t2r
ACS712  ACS2(34, 5.0, 4095, 66);  //mr
ACS712  ACS3(35, 5.0, 4095, 66);  //b2r
ACS712  ACS4(32, 5.0, 4095, 66);  //br
ACS712  ACS5(33, 5.0, 4095, 66);  //bl
ACS712  ACS6(25, 5.0, 4095, 66);
ACS712  ACS7(26, 5.0, 4095, 66);
ACS712  ACS8(27, 5.0, 4095, 66);
ACS712  ACS9(14, 5.0, 4095, 66);
//  ESP 32 example (might requires resistors to step down the logic voltage)
//  ACS712  ACS(25, 3.3, 4095, 185);
*/

float sensitivity = 66.0f; // Uncomment this for 30A board, Output sensitivity 66mV/A
int adcValue= 0;
const int currentPin = 36;
float adcVoltage = 0.0f;
float currentValue = 0.0f;
int ARDUINO_REF = 3300;
int offsetVoltage = ARDUINO_REF / 2;
float errorCorrectionAmp = 0.0f; // Set this to use a correcting offset on readings, e.g. -0.15f;
const float alpha = 0.1f; // Can vary this value between 0.1 and 0.9, changes filter impact



const int currentSwitchPin[8] = {12, 14, 27, 25, 33, 26, 23, 32};
float currentResults[8];

const int RelayPin[9] = {2, 15, 16, 13, 17, 5, 18, 19, 4};

/*
const int RelayPin0 = 2;
const int RelayPin1 = 15;
const int RelayPin2 = 16;
const int RelayPin3 = 13;
const int RelayPin4 = 17;
const int RelayPin5 = 5;
const int RelayPin6 = 18;
const int RelayPin7 = 19;
const int RelayPin8 = 4;
//const int RelayPin9 = 0;
*/

unsigned long sendtimeing = 0;
int countNoIdea = 0;

//const int TestMode = 23;


WiFiClient client;

Adafruit_AHT10 aht;

//boolean relay = false;




void setup() {

  pinMode(currentPin, INPUT);

  for (int i = 0; i < 8; i++) {
    pinMode(currentSwitchPin[i], OUTPUT);
    digitalWrite(currentSwitchPin[i], LOW);
    pinMode(RelayPin[i], OUTPUT);
    digitalWrite(RelayPin[i], LOW);
  }
  for (int i = 0; i < 9; i++) {
    pinMode(RelayPin[i], OUTPUT);
    digitalWrite(RelayPin[i], LOW);
  }

  //pinMode(TestMode, INPUT_PULLUP);

/*
  pinMode(RelayPin0, OUTPUT);
  pinMode(RelayPin1, OUTPUT); 
  pinMode(RelayPin2, OUTPUT); 
  pinMode(RelayPin3, OUTPUT); 
  pinMode(RelayPin4, OUTPUT); 
  pinMode(RelayPin5, OUTPUT); 
  pinMode(RelayPin6, OUTPUT); 
  pinMode(RelayPin7, OUTPUT); 
  pinMode(RelayPin8, OUTPUT); 
  //pinMode(RelayPin9, OUTPUT); 

  digitalWrite(RelayPin0, LOW);
  digitalWrite(RelayPin1, LOW);
  digitalWrite(RelayPin2, LOW);
  digitalWrite(RelayPin3, LOW);
  digitalWrite(RelayPin4, LOW);
  digitalWrite(RelayPin5, LOW);
  digitalWrite(RelayPin6, LOW);
  digitalWrite(RelayPin7, LOW);
  digitalWrite(RelayPin8, LOW);
  //digitalWrite(RelayPin9, LOW);
*/
 
  //set up coms
  Serial.begin(115200);
 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("...");
  }
  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());


  //set up sensors
  Serial.println("AHT10 test");
  if (!aht.begin()) {
    Serial.println("Couldn't find sensor!");
    delay(500);
    if (!aht.begin()) {
      Serial.println("Couldn't find sensor! 2");
      ESP.restart();
    }
  }
  Serial.println("AHT10 found");

/*
  if(digitalRead(TestMode) == HIGH){
    host = hostpi;
    Serial.println("normal mode");
  }else{
    host = hosttest;
    Serial.println("test mode");
  }
  */

/*
  ACS0.autoMidPoint();
  ACS1.autoMidPoint();
  ACS2.autoMidPoint();
  ACS3.autoMidPoint();
  ACS4.autoMidPoint();
  ACS5.autoMidPoint();
  ACS6.autoMidPoint();
  ACS7.autoMidPoint();
  ACS8.autoMidPoint();
  ACS9.autoMidPoint();
*/

}

void loop() {
  
  if(!client.connected()){
    if (!client.connect(hosttest, port)) {            //hostpi    //hosttest
    Serial.println("Connection to host failed");
    delay(1000);
    return;
    }
    Serial.println("Connected to server successful!");
  }


  if(millis() >= sendtimeing + 500){

    readCurrents();

    sendData();

/*
    //---
    int mA = ACS0.mA_AC();
    Serial.print("mA: ");
    Serial.print(mA);
    Serial.print(". Form factor: ");
    Serial.println(ACS0.getFormFactor());

    Serial.println("start:");
    mA = ACS0.mA_DC();
    //mA = analogRead(36);
    Serial.print("mA0: ");
    Serial.println(mA);
    mA = ACS1.mA_DC();
    //mA = analogRead(39);
    Serial.print("mA1: ");
    Serial.println(mA);
    mA = ACS2.mA_DC();
    //mA = analogRead(34);
    Serial.print("mA2: ");
    Serial.println(mA);
    mA = ACS3.mA_DC();
    //mA = analogRead(35);
    Serial.print("mA3: ");
    Serial.println(mA);
    mA = ACS4.mA_DC();
    //mA = analogRead(32);
    Serial.print("mA4: ");
    Serial.println(mA);
    mA = ACS5.mA_DC();
    //mA = analogRead(33);
    Serial.print("mA5: ");
    Serial.println(mA);
    mA = ACS6.mA_DC();
    //mA = analogRead(25);
    Serial.print("mA6: ");
    Serial.println(mA);
    mA = ACS7.mA_DC();
    //mA = analogRead(26);
    Serial.print("mA7: ");
    Serial.println(mA);
    mA = ACS8.mA_DC();
    //mA = analogRead(27);
    Serial.print("mA8: ");
    Serial.println(mA);
    mA = ACS9.mA_DC();
    //mA = analogRead(14);
    Serial.print("mA9: ");
    Serial.println(mA);
    Serial.println("end");
    //---
*/
    sendtimeing = millis();
  }

  
  if (!client) {
    return;
  }
  String line = client.readStringUntil('\r');
  if(line != NULL && line.length() > 5){ // was > 5
    Serial.println(line.length());
    getData(line);
  }
  
}

void readCurrents(){
  for (int i=0; i<8; i++){
    Serial.println("Read: ");
    Serial.println(i);
    digitalWrite(currentSwitchPin[i], HIGH);
    currentResults[i] = doReadAlphaFilter();
    digitalWrite(currentSwitchPin[i], LOW);
    Serial.println();
  }
}

// Reading and apply software correction
float doReadAlphaFilter() {
   float average = 0.0f, previousValue = 0.0f;

   analogRead(currentPin); // Dummy read, doesn't help

   for(int i = 0; i < 1000; i++) {
      adcValue = analogRead(currentPin);
      adcVoltage = (adcValue / 4096.0f) * ARDUINO_REF;
      currentValue = (((float)(adcVoltage - offsetVoltage)) / sensitivity);

      if (i>0)
        currentValue = alpha * currentValue + (1-alpha) * previousValue;

      average += currentValue;
      previousValue = currentValue;
   }

   float result = int((average / 1000.0f + errorCorrectionAmp) * 100) / 100.0;
   Serial.println(result);
   
   return result; 
}

void sendData(){
  sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
    Serial.print("Temperature: "); Serial.print(temp.temperature); Serial.println(" degrees C");
    Serial.print("Humidity: "); Serial.print(humidity.relative_humidity); Serial.println("% rH");
  

  DynamicJsonDocument doc(1024);
  doc["ID"] = 2;
  JsonObject dat = doc.createNestedObject("data");
  dat["fuseBoxTemp"] = temp.temperature;
  dat["fuseBoxHum"] = humidity.relative_humidity;
  dat["light0"] = currentResults[0];
  dat["light1"] = currentResults[1];
  dat["light2"] = currentResults[2];
  dat["light3"] = currentResults[3];
  dat["light4"] = currentResults[4];
  dat["light5"] = currentResults[5];
  dat["light6"] = currentResults[6];
  dat["light7"] = currentResults[7];

  
  char json[1024];
  serializeJson(doc, json);

  //String out = encr(json);  //encripting the json char array
  //client.println(out);
  Serial.println(json);

  client.println(json);
}

void getData(String input){
  //String dec = decr(input);
  Serial.println("Decoded: ");
  //Serial.println(dec);
  String dec = input;

  Serial.println(dec);

  char firstChar = dec.charAt(0);
  //Serial.println(firstChar);
  if (firstChar != '{') {
    countNoIdea++;
    if (countNoIdea >= 5){
      Serial.println("5 unknown messages");
      ESP.restart();
    }
    return;   //ensures that the json is decoded correctly
  }
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, dec);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    Serial.print("in:");
    Serial.println(input);
    ESP.restart();
    return;
  }
  
  countNoIdea = 0;

  int id = doc["ID"]; // 1000
  boolean pulse[10];
  pulse[0] = doc["data"]["pulse0"];
  pulse[1] = doc["data"]["pulse1"];
  pulse[2] = doc["data"]["pulse2"];
  pulse[3] = doc["data"]["pulse3"];
  pulse[4] = doc["data"]["pulse4"];
  pulse[5] = doc["data"]["pulse5"];
  pulse[6] = doc["data"]["pulse6"];
  pulse[7] = doc["data"]["pulse7"];
  pulse[8] = doc["data"]["pulse8"];


  for (int i = 0; i < 9; i++){
    if(pulse[i]){
      digitalWrite(RelayPin[i], HIGH);
      delay(200);
      digitalWrite(RelayPin[i], LOW);
    }
  }

  sendData();
}