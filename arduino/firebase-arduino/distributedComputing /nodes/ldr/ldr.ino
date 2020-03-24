/*
 *                                                                  
 *   _____ _____ _____ _____ _____    _____ __    _____ _____ _____ 
 *  |   __|     |  _  | __  |_   _|  |     |  |  |  _  |   __|   __|
 *  |__   | | | |     |    -| | |    |   --|  |__|     |__   |__   |
 *  |_____|_|_|_|__|__|__|__| |_|    |_____|_____|__|__|_____|_____|
 *                                                                  
 *                                                     Version 0.0.4
 *                       _____ _____ ____  _____ 
 *                      |   | |     |    \|   __|
 *                      | | | |  |  |  |  |   __|
 *                      |_|___|_____|____/|_____|
 *                                               
 *  
 *  ================================================================*  
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
extern "C" {
  #include <espnow.h>
}
#include "DHT.h"
#define LDR_PIN A0
#define SENSOR_LDR_ID "LDR-C2W"
#define SENSOR_LDR "luminosity"
#define SENSOR_DHT11_ID "DHT-C2W"
#define SENSOR_DHT11 "temperature"
#define DHT_PIN D1
#define DHTTYPE DHT11
//============================//
//         GLOBAL VAR         //
//============================//
struct PackData{
   float sensorData;
   char sensorType[21];
   char sensorID[21];
};
uint8_t MASTER_MAC_ADDRESS[6] = {0xA2, 0x20, 0xA6, 0x17, 0x37, 0x46}; // Master sends data to Firebase
bool LDR_OK_2_SEND = false;
bool DHT_OK_2_SEND = false;
int TRY_TIMES = 0; // avoid congestioning the system
DHT dht(DHT_PIN, DHTTYPE);
//============================//
//         PROTOTYPES         //
//============================//
PackData* createPackageData( float sensorData,  char* sensorType, char* sensorID);
void   initESPNow();
float  readLum();
void OnDataSent(uint8_t* mac, uint8_t status);
//============================//
//           SETUP            //
//============================//
//this sets the ground pin to LOW and the input voltage pin to high
void setup() {
    Serial.begin(9600);
    //Init ESP_NOW
    initESPNow();
    Serial.print("Mac Address in Station: ");
    Serial.println(WiFi.macAddress());

  // 0=idle, 1=master, 2=slave y 3=master+slave 
    esp_now_set_self_role(3);
 /*Check if master received the data*/


    //Add the master as a peer to receive some a cmd broadcast by it
//    uint8_t role=2;  
//    uint8_t channel= CHANNEL;
//    uint8_t key[0]={};   //no hay clave 
//    //uint8_t key[16] = {0,255,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
//    uint8_t key_len=sizeof(key);
//    esp_now_add_peer(MASTER_MAC_ADDRESS,role,channel,key,key_len);
}

//============================//
//           Loop             //
//============================//
void loop(){
  int len;
  uint8_t data[sizeof(PackData)];
  PackData* PD;
  
  if ((DHT_OK_2_SEND && LDR_OK_2_SEND) || TRY_TIMES > 80){ // Sleep if master is down or the system is overheaded
      ESP.deepSleep(5 * 60000000); // 5 minute sleep
   }
    
    // Create Package with data sensor  
    if (!LDR_OK_2_SEND){
      PD = createPackageData(readLum(),SENSOR_LDR,SENSOR_LDR_ID);
      // Set up info to send
      memcpy(data, PD, sizeof(PackData)); len = sizeof(data);
      // Sending
      esp_now_register_send_cb(OnDataSentLDR);
      esp_now_send(MASTER_MAC_ADDRESS, data, len);
      free(PD);
    }
    /*Send another package data to master*/
    if (LDR_OK_2_SEND && !DHT_OK_2_SEND ){
        PD = createPackageData(readDHTTemp(),SENSOR_DHT11,SENSOR_DHT11_ID);
        memcpy(data, PD, sizeof(PackData));  len = sizeof(data);
        esp_now_register_send_cb(OnDataSentDHT);
        esp_now_send(MASTER_MAC_ADDRESS, data, len);
        free(PD);
    }
    TRY_TIMES += 1;
    delay(400);

   
        
}
//============================//
//  Initial setup functions   //
//============================//
void initESPNow() {
 if (esp_now_init()!=0) {
    Serial.println("*** ESP_Now init failed");
    ESP.restart();
    delay(1);
  }
}
void OnDataSentLDR(uint8_t* mac, uint8_t status){
  char MacNode[6];
  sprintf(MacNode,"%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  Serial.print(". Sending to ESP MAC: "); Serial.print(MacNode);
  Serial.print(". Success (0=0K - 1=ERROR): "); Serial.println(status);
  // If success, allow deep slep
  if (status == 0){
    LDR_OK_2_SEND = true;
  }
}
void OnDataSentDHT(uint8_t* mac, uint8_t status){
  char MacNode[6];
  sprintf(MacNode,"%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  Serial.print(". Sending to ESP MAC: "); Serial.print(MacNode);
  Serial.print(". Success (0=0K - 1=ERROR): "); Serial.println(status);
  // If success, allow deep slep
  if (status == 0){
    DHT_OK_2_SEND = true;
  }
}

//============================//
//  Data Manipulation/Setup   //
//============================//
PackData* createPackageData( float sensorData,  char* sensorType, char* sensorID){
  /* Creates package to send through WIFI master intranet. 
  *  You have to free the allocated memory after using this function.
  *  sensorData <float> Data capture by a sensor
  *  sensortype <char*> A max of 20 chars with the name of type of sensor. Eg. (temperature,luminosity)
  *  sensorID <char*> Unique ID of a sensor inside a room.
  */
  PackData* PD;
  
  PD = (PackData*) malloc(sizeof(PackData));
  PD->sensorData = sensorData;
  sprintf(PD->sensorType,"%s\0",sensorType);
  sprintf(PD->sensorID, "%s\0",sensorID);
  
  // Display Package info
  Serial.print(String(PD->sensorType)+ String(" ")); Serial.print(String(PD->sensorID) +": ");Serial.println(PD->sensorData);
  
  return PD;
}

//============================//
//      Sensors Function      //
//============================//

float readLum(){
    float analogValue = (float) analogRead(LDR_PIN);
    return analogValue;
}
float readDHTTemp(){
    return dht.readTemperature();/* Get temperature value */
}
