/*
 *
 *   _____ _____ _____ _____ _____    _____ __    _____ _____ _____
 *  |   __|     |  _  | __  |_   _|  |     |  |  |  _  |   __|   __|
 *  |__   | | | |     |    -| | |    |   --|  |__|     |__   |__   |
 *  |_____|_|_|_|__|__|__|__| |_|    |_____|_____|__|__|_____|_____|
 *
 *                                                            Version 0.1.4
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
#include<ArduinoJson.h> //Using library verstion 5
#include <PubSubClient.h>
//============================//
//         GLOBAL VAR         //
//============================//
WiFiClient wifiClient;
PubSubClient client(wifiClient);
int status = WL_IDLE_STATUS;
bool toggleButtom = false;
bool lockButtom = false;
//============================//
//         PROTOTYPES         //
//============================//
void initESPNow();
void initWiFi();
void reconnectWiFi();
void publish();
void reconnect();
void toggleLight();
void sendLightStatus(String fieldName, boolean lightStatus);
void initMQTT();
//============================//
//           SETUP            //
//============================//
void setup() {
    Serial.begin(9600);
    //Initial settings
    pinMode(D0,OUTPUT); // LED is ON if nodemcu is connect to ThingsBoard
    digitalWrite(D0,HIGH);
    pinMode(D1,INPUT_PULLUP);
    pinMode(D7,OUTPUT);
    digitalWrite(D7,HIGH);
    initWiFi();
    initMQTT();

    /*MacAddres of Node Master*/
    Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
    Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());
    delay(1000);
    attachInterrupt(digitalPinToInterrupt(D1), toggleLight, FALLING);
}

//============================//
//           Loop             //
//============================//
void loop() {

    if ( !client.connected() ) {
        reconnect();
    }
    client.loop();

}

/*Callback Function*/
void on_message(const char* topic, byte* payload, unsigned int length) {
    Serial.println("On message");

    char json[length + 1];
    strncpy (json, (char*)payload, length);
    json[length] = '\0';

    Serial.print("Topic: ");
    Serial.println(topic);
    Serial.print("Message: ");
    Serial.println(json);

    // Decode JSON request
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& data = jsonBuffer.parseObject((char*)json);

    if (!data.success())
    {
        Serial.println("parseObject() failed");
        return;
    }

    // Check request method
    String methodName = String((const char*)data["method"]);
    Serial.println(methodName);

    if (methodName.equals("setValue")) {
        // Set light status according TB
        Serial.println((const char*)data["params"]);
        bool setValue;
        if (String((const char*)data["params"]) == "true"){
            setValue = true;
        }
        else{
            setValue = false;
        }
        setLight(setValue);

    }else if (methodName.equals("getValue")) {
        // Send lightStatus to TB
        lockButtom = true;
        sendLightStatus("value", toggleButtom);
        lockButtom = false;
    }else if (methodName.equals("checkStatus")){
        // Send lightStatus to TB
        lockButtom = true;
        sendLightStatus("value", toggleButtom);
        lockButtom = false;
    }
}

//============================//
//      Action Functions      //
//============================//

void setLight(bool switchLight){
    if(!lockButtom){

        if(switchLight){
            digitalWrite(D7,LOW);
        }
        else{
            digitalWrite(D7,HIGH);
        }
        Serial.print("Light Status :");Serial.println(switchLight);
        toggleButtom = switchLight;
        lockButtom = true;
        sendLightStatus("value", toggleButtom);
        lockButtom = false;
    }
}

void toggleLight(){
    /*Set The light according the toogleButtom*/
    /*If the lockButtom is activated we don't touch on the light*/
    /*The lockButtom tries to make the light consitent with the Database*/
    if (!lockButtom){
        toggleButtom = !toggleButtom;
        sendLightStatus("value", toggleButtom);
        if (toggleButtom){
            digitalWrite(D7,LOW);
        }
        else{
            digitalWrite(D7,HIGH);
        }
        Serial.print("Light Status :");Serial.println(toggleButtom);
    }
}

//============================//
//  Initial setup functions   //
//============================//
void initMQTT(){
    client.setServer( BROKER_MQTT, BROKER_PORT );  // Set MQTT client
    client.setCallback( on_message );
}

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

void reconnectWiFi() {
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
}

//============================//
//  Data Manipulation/Setup   //
//============================//
void sendLightStatus(String fieldName, boolean lightStatus){
    StaticJsonBuffer<200> jsonBuffer;
    char payload[256];
    JsonObject& data = jsonBuffer.createObject();

    data[fieldName.c_str()] = lightStatus;
    data.printTo(payload, sizeof(payload));
    client.publish("v1/devices/me/attributes", payload);
}
void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        status = WiFi.status();
        if ( status != WL_CONNECTED) {
            WiFi.begin(WLAN_SSID, WLAN_PASSWORD);
            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                Serial.print(".");
            }
            Serial.println("Connected to AP");
        }
        Serial.print("Connecting to ThingsBoard node ...");

        // Attempt to connect (clientId, username, password)
        if ( client.connect("Esp8266", TB_TOKEN, NULL) ) {
            Serial.println( "[DONE]" );
            // Subscribing to receive RPC requests
            client.subscribe("v1/devices/me/rpc/request/+");
            digitalWrite(D0,LOW);
        } else {
            digitalWrite(D0,HIGH);
            Serial.print( "[FAILED] [ rc = " );
            Serial.println( " : retrying in 2 seconds]" );
            delay( 2000 );
        }
    }
}
