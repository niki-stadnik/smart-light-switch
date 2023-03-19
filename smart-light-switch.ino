#include <ArduinoJson.h>
#include <WiFi.h>
#include <Adafruit_AHT10.h>


#include "config.h"
const char* ssid = WIFI;
const char* password =  PASS;
const char * hostpi = HOSTPI;
const char * hosttest = HOSTTEST;
const char * host;
const uint16_t port = PORT;
char * key = KEY;

#include "ACS712.h"
ACS712  ACS(36, 3.0, 4095, 66);
float weight = 0.2;
int loops = 50;
int balance = 0;
float avrOld[8] = {0, 0, 0, 0, 0, 0, 0, 0};

//const int currentPin = 36;
//int ARDUINO_REF = 3300;

float adcValue = 0.0f;
float mV = 0.0f;
float eCurrent = 0.0f;


const int currentSwitchPin[8] = {12, 14, 27, 25, 33, 26, 23, 32};
float currentResults[8];

const int RelayPin[9] = {2, 15, 16, 13, 17, 5, 18, 19, 4};


unsigned long sendtimeing = 0;
int countNoIdea = 0;

//const int TestMode = 23;


WiFiClient client;

Adafruit_AHT10 aht;

boolean relay = false;




void setup() {
  //GPIO setup
  for (int i = 0; i < 8; i++) {
    pinMode(currentSwitchPin[i], OUTPUT);
    digitalWrite(currentSwitchPin[i], LOW);
    pinMode(RelayPin[i], OUTPUT);
    digitalWrite(RelayPin[i], LOW);
    currentResults[i] = 0.0f;
  }
  for (int i = 0; i < 9; i++) {
    pinMode(RelayPin[i], OUTPUT);
    digitalWrite(RelayPin[i], LOW);
  }

  //pinMode(TestMode, INPUT_PULLUP);

 
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
      //ESP.restart();
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

  //setting up current sensors
  digitalWrite(currentSwitchPin[0], HIGH);
  ACS.autoMidPoint();

  Serial.print("MidPoint: ");
  Serial.println(ACS.getMidPoint());
  Serial.print("Noise mV: ");
  Serial.println(ACS.getNoisemV());
  Serial.print("Amp/Step: ");
  Serial.println(ACS.getAmperePerStep(), 4);

  auto0();
  
  digitalWrite(currentSwitchPin[0], LOW);
}

void loop() {
  
  if(!client.connected()){
    if (!client.connect(hostpi, port)) {            //hostpi    //hosttest
    Serial.println("Connection to host failed");
    delay(1000);
    return;
    }
    Serial.println("Connected to server successful!");
  }


  if(millis() >= sendtimeing + 500){

    readCurrents();

    //sendData();

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
    getCurrent(i);
    digitalWrite(currentSwitchPin[i], LOW);
    Serial.println();
  }
}



void getCurrent(int i){
  adcValue = getValue();
  if ((adcValue - avrOld[i]) > 1 || (avrOld[i] - adcValue) > 1)
      avrOld[i] = adcValue;
  else
      adcValue = avrOld[i];
  //The ESP's ADC when running at 3.3V has a resolution of (3.3/4095) 0.81mV per LSB.
  //That means that 0.81/66 gives you 0.012A or 12mA per LSB.
  //Therefore 10W lightbulb would be seen as 10W/220V=0.045A or 3-4 in analog.
  currentResults[i] = adcValue * 12;
  Serial.print(" avr: ");
  Serial.print(adcValue, 0);
  Serial.println();
  Serial.println(currentResults[i]);
}

int getValue(){
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

void auto0(){
  float value = ACS.mA_AC();  // get good initial value
  int summ = 0;
  for (int i = 0; i<10; i++){
    summ += getValue();
  }
  balance = summ / 10;
}

//todo if not all sensors are calibrated the same -> make balance an array

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
  dat["fuseBoxFan"] = relay;
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

  int id = doc["ID"]; //2
  if (id != 2){
    Serial.println("message is brodcast");
    return;
  }
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


  //atm turns all the lights in sequence not at the same time (cool effect?) 
  for (int i = 1; i < 9; i++){
    if(pulse[i]){
      digitalWrite(RelayPin[i], HIGH);
      delay(200);
      digitalWrite(RelayPin[i], LOW);
    }
  }

  if (pulse[0] == true && relay == false){
    digitalWrite(RelayPin[0], HIGH);
    relay = true;
  }
  else if (pulse[0] == false && relay == true){
    digitalWrite(RelayPin[0], LOW);
    relay = false;
  }

  sendData();
}