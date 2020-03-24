/*
 *                                                                         
 *   _____ _____ _____ _____ _____    _____ __    _____ _____ _____        
 *  |   __|     |  _  | __  |_   _|  |     |  |  |  _  |   __|   __|  
 *  |__   | | | |     |    -| | |    |   --|  |__|     |__   |__   |  
 *  |_____|_|_|_|__|__|__|__| |_|    |_____|_____|__|__|_____|_____|       
 *                                                                         
 *                                                            Version 0.0.4
 *                _____ _____ _____ _____ _____ _____ 
 *               |     |  _  |   __|_   _|   __| __  |
 *               | | | |     |__   | | | |   __|    -|
*                |_|_|_|__|__|_____| |_| |_____|__|__|
 *                                      
 *  
 *  ================================================================
 *  
 *  Copyright (C) [2019] [Joao Phillipe Cardenuto]
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *  
 *  ================================================================
 */

//============================//
//         INCLUDE            //
//============================//
#include <ESP8266WiFi.h>
// Make sure you have a env.h, following the env_sample.h
#include "env.h"
#include <FirebaseArduino.h>
#include <Ticker.h>
extern "C" {
  #include <espnow.h>
}
#define CHANNEL 1 // Channel of Communication using nodemcu

//============================//
//         GLOBAL VAR         //
//============================//

//uint8_t macSlaves[] =  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast an info
String MasterData = "";
bool publishTime = false;
Ticker ticker; //Set interval of publishing to Firebase
struct PackData{   //Struct used to receive and send data between the nodes
   float sensorData;
   char  sensorType[21];
   char  sensorID[21];
}; 

//============================//
//         PROTOTYPES         //
//============================//
void initFirebase();
void initESPNow();
void initWiFi();
void reconnectWiFi();
void initTicker();
void publish(); 
String aggregateSensorData(String sensorData, String srcData, String sensorType, String sensorID);

//============================//
//           SETUP            //
//============================//
void setup() {
  Serial.begin(9600);
  //Initial settings
   initFirebase();
   initWiFi();
   initESPNow();
   initTicker();
  /*MacAddres of Node Master*/
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress()); 
  delay(1000);
   // initialize digital pin LED_BUILTIN as an output.
  pinMode(D1,OUTPUT); // If success turn on external LED
  digitalWrite(D1, HIGH);    // turn the LED off by making the voltage LOW
  
  // 0=idle, 1=master, 2=slave y 3=master+slave 
  esp_now_set_self_role(3);   
}

//============================//
//           Loop             //
//============================//
void loop() {

    esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {
        PackData PD;
        memcpy(&PD, data, sizeof(PD));
    
        char macMaster[18];
        sprintf(macMaster, "%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
        Serial.print("Receiving data from ESP MAC: "); Serial.print(macMaster);Serial.println();
        Serial.print(PD.sensorType);Serial.print(" "+String(PD.sensorID)+": "); Serial.println(PD.sensorData);
        MasterData =  aggregateSensorData(String(PD.sensorData), MasterData, String(PD.sensorType), String(PD.sensorID));
        Serial.println("MasterData: "+MasterData);
        delay(30);
        });

        if (publishTime){
          Serial.println("--->  Send2Firebase: "+MasterData);
          send2Firebase(MasterData);
          MasterData = "";
          WiFi.forceSleepBegin();
          delay(5000);
//          ESP.deepSleep(DEEP_SLEEP * 60000000); // 1 minute sleep
          ESP.deepSleep(DEEP_SLEEP * 600000); // seconds sleep  --> Used to reset esp
        }
}

//============================//
//  Initial setup functions   //
//============================//
void initWiFi() {
    delay(10);
    Serial.println("Connecting Wifi: " + String(WLAN_SSID));
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(WLAN_SSID, WLAN_PASSWORD);
   
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("Connected to " + String(WLAN_SSID) + " | IP => ");
    Serial.println(WiFi.localIP());
}
void initESPNow() {
 if (esp_now_init()!=0) {
    Serial.println("*** ESP_Now init failed");
    ESP.restart();
    delay(1);
  }
}
void reconnectWiFi() {
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
}
void initFirebase(){
    Serial.println("Setting up Firebase");
    Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);
}

void initTicker(){
    ticker.attach_ms(PUBLISH_INTERVAL, publish);
}

void publish(){
      publishTime = true;
}

//============================//
//  Data Manipulation/Setup   //
//============================//
String aggregateSensorData(String sensorData, String srcData, String sensorType, String sensorID){
  /*
   * Function aggregateSensorData
   * Join a sensor data to a source Json struct.
   * Param:
   * sensorData: <String> data from a sensor
   * srcData: <String> source json struct that will receive the data. E.g {"temperature":{"DHT11":"24.00","DHT12":"24.00"},"luminosity":{"LDR":"1"}}
   * sensorTyoe: <String> Sensor type. E.g "temperature", "luminosity"...
   * sensorID: <String> Unique sensor identification 
   * Return:
   * <String> Json with the sensorData included
   */
    DynamicJsonBuffer jsonBuffer;
    // We have to initialize this variable to avoid errors
    JsonObject& dataObject = jsonBuffer.createObject();
    
  
    char jsonChar[500];
    if(srcData != ""){ // If srcData is not an empty json struct, parse srcData
      
       jsonBuffer.clear();
       JsonObject& dataObject = jsonBuffer.parseObject(srcData);
     
    }
    // If sensorType already included on the srcData, just include another sensorID on the nested object
    if(dataObject.containsKey(sensorType)){
      Serial.println("Contains " + sensorType);
         dataObject[sensorType]["measurementValues"][sensorID] = sensorData;  
    } else {
      
      JsonObject& sType = dataObject.createNestedObject(sensorType);
      sType["measurementType"] = sensorType;
      JsonObject& measurementValues = sType.createNestedObject("measurementValues");
      measurementValues[sensorID] =  sensorData ;
    }
    
    dataObject.printTo((char*)jsonChar, dataObject.measureLength() + 1);

    return String(jsonChar);
}

void send2Firebase(String dataObject){
    esp_now_deinit();
    DynamicJsonBuffer jsonBuffer;
    if( dataObject == "") {
      return;
    }
    
    // Insert metadata in sendObject
    JsonObject& sendObject = jsonBuffer.createObject();
    JsonObject& measurementTypes = sendObject.createNestedObject("measurementTypes");
    JsonObject& timestampObject = sendObject.createNestedObject("timestamp");
    timestampObject[".sv"] = "timestamp";

    JsonObject& dataObjectJson = jsonBuffer.parseObject(dataObject);
    sendObject["measurementTypes"] =  dataObjectJson;
    
      /*Debug SendJson*/
//    char jsonChar[300];
//    sendObject.printTo((char*)jsonChar, sendObject.measureLength() + 1);
//    Serial.println("JSONCHAR: "+ String(jsonChar));

    // Push to Firebase
    Serial.print("Send to Firebase: ");
    Firebase.push(ABS_PATH_JSON, sendObject); 
    // Assert success with some code
    if (Firebase.success()){
     Serial.println(" OK!");
    }
    else{
      Serial.println(" Fail!");
      Serial.println(Firebase.error());
    }  
    
}
