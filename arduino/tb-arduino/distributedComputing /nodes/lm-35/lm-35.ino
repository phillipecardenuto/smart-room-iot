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
 *             ____  _____ _____           __    ____  _____ 
 *            |    \|  |  |_   _|   ___   |  |  |    \| __  |
 *            |  |  |     | | |    |___|  |  |__|  |  |    -|
 *            |____/|__|__| |_|           |_____|____/|__|__|                                               
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
#define LM35_PIN A0
#define SENSOR_LM35_ID "LM35-C2M"
#define SENSOR_LM35 "temperature"
//============================//
//         GLOBAL VAR         //
//============================//
struct PackData{
   float sensorData;
   char  sensorType[21];
   char  sensorID[21];
};
uint8_t MASTER_MAC_ADDRESS[6] = {0xA2, 0x20, 0xA6, 0x17, 0x37, 0x46}; // Master sends data to Firebase
bool LM35_OK_2_SEND = false;
int TRY_TIMES = 0; // avoid congestioning the system
//============================//
//         PROTOTYPES         //
//============================//
PackData* createPackageData( float sensorData,  char* sensorType, char* sensorID);
void   initESPNow();
void OnDataSentLM35(uint8_t* mac, uint8_t status);
float readLM35();
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
  uint8_t data_dht[sizeof(PackData)];
  float luminosity;
  float temperature;
  PackData* PD;
  
  if ((LM35_OK_2_SEND ) || TRY_TIMES > 80){ // Sleep if master is down or the system is overheaded
//      ESP.deepSleep(1 * 60000000); // 1 minute sleep
        ESP.deepSleep(1 * 10000000); 
   }
    
    /*Send another package data to master*/
    if (!LM35_OK_2_SEND ){
      temperature = readLM35();
       if( 0 <= temperature <= 80){
        PD = createPackageData(temperature, SENSOR_LM35, SENSOR_LM35_ID);
        memcpy(data_dht, PD, sizeof(PackData));  len = sizeof(data_dht);
        esp_now_register_send_cb(OnDataSentLM35);
        esp_now_send(MASTER_MAC_ADDRESS, data_dht, len);
        free(PD);
        delay(1000);
       }
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

void OnDataSentLM35(uint8_t* mac, uint8_t status){
  char MacNode[6];
  sprintf(MacNode,"%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  Serial.print(". Sending to ESP MAC: "); Serial.print(MacNode);
  Serial.print(". Success (0=0K - 1=ERROR): "); Serial.println(status);
  // If success, allow deep slep
  if (status == 0){
    LM35_OK_2_SEND = true;
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
  PD->sensorData =  sensorData;
  sprintf(PD->sensorType,"%s\0",sensorType);
  sprintf(PD->sensorID, "%s\0",sensorID);
  
  // Display Package info
  Serial.print(String(PD->sensorType)+ String(" ")); Serial.print(String(PD->sensorID) +": ");Serial.println(PD->sensorData);
  
  return PD;
}

//============================//
//      Sensors Function      //
//============================//

float readLM35(){
   return (analogRead(LM35_PIN) * 0.0048828125 * 100);
}
