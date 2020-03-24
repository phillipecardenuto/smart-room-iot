//============================//
//         INCLUDE            //
//============================//
#define RELE_PORT D7
#define BUTTOM_PORT D1
#include <ESP8266WiFi.h>
//#include <FirebaseArduino.h>
#include "FirebaseESP8266.h" // Using different library to send data to Firebase
// Downloaded library from //https://github.com/mobizt/Firebase-ESP8266
#include "env.h"

//============================//
//         GLOBAL VAR         //
//============================//

bool toggleButtom = false;
bool lockButtom = false;
bool setFire = false;
FirebaseData firebaseData;

//============================//
//         PROTOTYPES         //
//============================//
void initFirebase();
void toggleLight();
void streamCallback(StreamData data);
void streamTimeoutCallback(bool timeout);
void initWiFi();


//============================//
//           SETUP            //
//============================//
void setup()
{
    // Debug console
    Serial.begin(9600);
    // Setup I/O Ports
    pinMode(D1,INPUT_PULLUP); // The Input_PULLUP is importa to avoid issue aliasing reading
    pinMode(D7,OUTPUT);
    digitalWrite(D7,HIGH);

    // connect to wifi.
    initWiFi();
    initFirebase();
    delay(500);
    if (!Firebase.beginStream(firebaseData, CLASS_LIGHT_PATH)){
        //Could not begin stream connection, then print out the error detail
        Serial.println(firebaseData.errorReason());
    }
    // After all setup is fineshed, attach a interuption
    attachInterrupt(digitalPinToInterrupt(D1), toggleLight, FALLING); //Interruption
}
//============================//
//           Loop             //
//============================//
void loop(){
    if(setFire){
        lockButtom = true;
        Firebase.setBool(firebaseData,CLASS_LIGHT_PATH,toggleButtom);
        setFire = false;
        delay(100);
        lockButtom = false;
    }
    
    if (!Firebase.readStream(firebaseData)){
        Serial.println(firebaseData.errorReason());
    }

    if (firebaseData.streamTimeout()){
        Serial.println("Stream timeout, resume streaming...");
        Serial.println();
    }
    // Receive stream data from firebase
    if (firebaseData.streamAvailable()){
      lockButtom = true;
        Serial.println("Stream Data...");
        if (firebaseData.dataType() == "boolean"){
            Serial.println(firebaseData.boolData() == 1 ? "true" : "false");
            if(firebaseData.boolData() != toggleButtom){
                lockButtom = false;
                toggleLight();
                lockButtom = true;
                setFire = false;
            }
        }
        delay(500);
        lockButtom = false;
    }
}


//============================//
//      Action Functions      //
//============================//

void toggleLight(){
  if (!lockButtom){
    toggleButtom = !toggleButtom;
    setFire = true;
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
void initFirebase(){
    Serial.println("Setting up Firebase");
    Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);
    Serial.println("...Done");
}

void initWiFi() {
    delay(100);
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
