#include <Arduino.h>
#include <WiFi.h>
#include "WebSocketsClient.h"
#include "StompClient.h"
#include "SudoJSON.h"
#include <Adafruit_AHT10.h>

//debug
#define DEBUG 0 //1 = debug messages ON; 0 = debug messages OFF

#if DEBUG == 1
#define debugStart(x) Serial.begin(x)
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debugStart(x)
#define debug(x)
#define debugln(x)
#endif


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
unsigned long keepAlive = 0;

const int powerSensorPin[8] = {12, 14, 27, 25, 33, 26, 23, 32};
boolean powerResults[8];
const int RelayPin[9] = {2, 15, 16, 13, 17, 5, 18, 19, 4};


unsigned long sendtimeing = 0;
int countNoIdea = 0;


Adafruit_AHT10 aht;

boolean relay = false;



void setup() {
  // setup serial
  debugStart(115200);
  // flush it - ESP Serial seems to start with rubbish
  debugln();

  // connect to WiFi
  debugln("Logging into WLAN: " + String(wlan_ssid));
  debug(" ...");
  WiFi.begin(wlan_ssid, wlan_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    debug(".");
  }
  debugln(" success.");
  debug("IP: "); debugln(WiFi.localIP());
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
  debugln("AHT10 test");
  if (!aht.begin()) {
    debugln("Couldn't find sensor!");
    delay(500);
    if (!aht.begin()) {
      debugln("Couldn't find sensor! 2");
      ESP.restart();
    }
  }
  debugln("AHT10 found");
}


// Once the Stomp connection has been made, subscribe to a topic

void subscribe(Stomp::StompCommand cmd) {
  debugln("Connected to STOMP broker");
  stomper.subscribe("/topic/lightSwitch", Stomp::CLIENT, handleMessage);    //this is the @MessageMapping("/test") anotation so /topic must be added
  stomper.subscribe("/topic/keepAlive", Stomp::CLIENT, handleKeepAlive);
}

Stomp::Stomp_Ack_t handleMessage(const Stomp::StompCommand cmd) {
  debugln(cmd.body);
  keepAlive = millis();
  getData(cmd.body);
  return Stomp::CONTINUE;
}
Stomp::Stomp_Ack_t handleKeepAlive(const Stomp::StompCommand cmd) {
  debugln(cmd.body);
  keepAlive = millis();
  return Stomp::CONTINUE;
}

void error(const Stomp::StompCommand cmd) {
  debugln("ERROR: " + cmd.body);
}



void loop() {
  if(millis() >= keepAlive + 60000){  //if no messages are recieved in 1min - restart esp
    ESP.restart();
    keepAlive = millis();
  }
  
  if(millis() >= sendtimeing + 250){

    for (int i=0; i<8; i++){
      debug("Status: ");
      debug(i);
      debug(" : ");
      powerResults[i] = digitalRead(powerSensorPin[i]);
      debugln(powerResults[i]);
    }

    sendData();

    sendtimeing = millis();
  }
  webSocket.loop();
}


void sendData(){
  sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
    debug("Temperature: "); debug(temp.temperature); debugln(" degrees C");
    debug("Humidity: "); debug(humidity.relative_humidity); debugln("% rH");
  
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
  SudoJSON json = SudoJSON(input);

  boolean pulse[10];
  pulse[0] = json.getPairB("pulse0");
  pulse[1] = json.getPairB("pulse1");
  pulse[2] = json.getPairB("pulse2");
  pulse[3] = json.getPairB("pulse3");
  pulse[4] = json.getPairB("pulse4");
  pulse[5] = json.getPairB("pulse5");
  pulse[6] = json.getPairB("pulse6");
  pulse[7] = json.getPairB("pulse7");
  pulse[8] = json.getPairB("pulse8");

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

  //sendData();
}