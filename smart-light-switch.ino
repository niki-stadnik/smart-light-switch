#include <Arduino.h>
#include <WiFi.h>
#include "WebSocketsClient.h"
#include "StompClient.h"
#include "SudoJSON.h"
#include <Adafruit_AHT10.h>
#include <ArduinoJson.h>



#include "config.h"
const char* wlan_ssid = WIFI;
const char* wlan_password =  PASS;
const char * ws_host = HOSTPI;
const uint16_t ws_port = PORT;
const char* ws_baseurl = URL; 
bool useWSS = USEWSS;
const char * key = KEY;


// VARIABLES
WebSocketsClient webSocket;
Stomp::StompClient stomper(webSocket, ws_host, ws_port, ws_baseurl, true);

const int powerSensorPin[8] = {12, 14, 27, 25, 33, 26, 23, 32};
boolean powerResults[8];
const int RelayPin[9] = {2, 15, 16, 13, 17, 5, 18, 19, 4};


unsigned long sendtimeing = 0;
int countNoIdea = 0;


Adafruit_AHT10 aht;

boolean relay = false;



void setup() {
  // setup serial
  Serial.begin(115200);
  // flush it - ESP Serial seems to start with rubbish
  Serial.println();

  // connect to WiFi
  Serial.println("Logging into WLAN: " + String(wlan_ssid));
  Serial.print(" ...");
  WiFi.begin(wlan_ssid, wlan_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" success.");
  Serial.print("IP: "); Serial.println(WiFi.localIP());
  stomper.onConnect(subscribe);
  stomper.onError(error);


 // Start the StompClient

  if (useWSS) {
    stomper.beginSSL();
  } else {
    stomper.begin();
  }


  //GPIO setup
  for (int i = 0; i < 8; i++) {
    pinMode(powerSensorPin[i], INPUT_PULLDOWN);
    powerResults[i] = false;
  }
  for (int i = 0; i < 9; i++) {
    pinMode(RelayPin[i], OUTPUT);
    digitalWrite(RelayPin[i], LOW);
  }

  
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
}


// Once the Stomp connection has been made, subscribe to a topic

void subscribe(Stomp::StompCommand cmd) {
  Serial.println("Connected to STOMP broker");
  stomper.subscribe("/topic/lightSwitch", Stomp::CLIENT, handleMessage);    //this is the @MessageMapping("/test") anotation so /topic must be added
}

Stomp::Stomp_Ack_t handleMessage(const Stomp::StompCommand cmd) {
  Serial.println(cmd.body);
  getData(cmd.body);
  return Stomp::CONTINUE;
}

void error(const Stomp::StompCommand cmd) {
  Serial.println("ERROR: " + cmd.body);
}



void loop() {
  
  if(millis() >= sendtimeing + 500){

    for (int i=0; i<8; i++){
      Serial.print("Status: ");
      Serial.print(i);
      Serial.print(" : ");
      powerResults[i] = digitalRead(powerSensorPin[i]);
      Serial.println(powerResults[i]);
    }

    sendData();

    sendtimeing = millis();
  }
  webSocket.loop();
}


void sendData(){
  sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
    Serial.print("Temperature: "); Serial.print(temp.temperature); Serial.println(" degrees C");
    Serial.print("Humidity: "); Serial.print(humidity.relative_humidity); Serial.println("% rH");
  
  // Construct the STOMP message
  SudoJSON json;
  json.addPair("fuseBoxTemp", temp.temperature);
  json.addPair("fuseBoxHum", humidity.relative_humidity);
  json.addPair("fuseBoxFan", relay);
  json.addPair("light0", powerResults[0]);
  json.addPair("light1", powerResults[1]);
  json.addPair("light2", powerResults[2]);
  json.addPair("light3", powerResults[3]);
  json.addPair("light4", powerResults[4]);
  json.addPair("light5", powerResults[5]);
  json.addPair("light6", powerResults[6]);
  json.addPair("light7", powerResults[7]);
  // Send the message to the STOMP server
  stomper.sendMessage("/app/lightSwitch", json.retrive());   //this is the @SendTo anotation
}

void getData(String input){
  char string[input.length()+1];
  char out[input.length()+1];
  input.toCharArray(string, input.length()+1);
  int count = 0;
  for(int i =0; i < input.length(); i++ ) {
    if (string[i] != '\\'){
      out[i - count]=string[i];
    }else count++;
  }
  Serial.println(out);
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, out);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    Serial.print("in:");
    Serial.println(out);
    ESP.restart();
    return;
  }


  boolean pulse[10];
  pulse[0] = doc["pulse0"];
  pulse[1] = doc["pulse1"];
  pulse[2] = doc["pulse2"];
  pulse[3] = doc["pulse3"];
  pulse[4] = doc["pulse4"];
  pulse[5] = doc["pulse5"];
  pulse[6] = doc["pulse6"];
  pulse[7] = doc["pulse7"];
  pulse[8] = doc["pulse8"];


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