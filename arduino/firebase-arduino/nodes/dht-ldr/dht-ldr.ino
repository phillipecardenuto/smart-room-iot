#include <ESP8266WiFi.h>
#include "DHT.h"
#include "env.h" // Definition of enviromnent variables
// Make sure you have a env.h, following the env_sample.h
#include <Ticker.h>
#include <FirebaseArduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#define DHT_PIN D1
#define DHTTYPE DHT11
#define LDR_PIN A0


/*************************************/
/*          PROTOTYPES               */
/**************************************/
void   initFirebase();
void   initTicker();
void   publish();
float  readDHTTemp();
int    readLum();
void   initWiFi();
void   reconnectWiFi();
String aggregateSensorData(String sensorData, String srcData, String sensorType, String sensorID);
void   sendFloat2Firebase(String path, float floatData);
void   sendStr2Firebase(String path, String floatData);
JsonObject& createJsonObject(String sensorLDRData);

/**************************************/
/*         GLOBAL VARIABLES           */
/**************************************/
/**************************************/
/*          DISPLAY PINS SETUP        */
/**************************************/
/* pin D8 - Serial clock out (SCLK)   */
/* pin D7- Serial data out (DIN)      */
/* pin D6 - Data/Command select (D/C) */
/* pin D5 - LCD chip select (CS)      */
/* pin D2 - LCD reset (RST)           */
/**************************************/
Adafruit_PCD8544 display = Adafruit_PCD8544(D8, D7, D6, D5, D2);

Ticker ticker; //Set interval of publishing to Firebase
bool publishNewState = true;
DHT dht(DHT_PIN, DHTTYPE);

/**************************************/
/*                SETUP               */
/**************************************/
//this sets the ground pin to LOW and the input voltage pin to high
void setup() {
    Serial.begin(9600);
    initWiFi();
    initTicker();
    initFirebase();
    dht.begin();
    Serial.println("MacAddress: "+WiFi.macAddress());
    Serial.print("LM35\t");
    Serial.println("DHT");
}

/**************************************/
/*                LOOP                */
/**************************************/
void loop(){

      String sensorDHTData = String(readDHTTemp());
      String sensorLDRData = String(readLum());
      Serial.print("Temperature: " +String(sensorDHTData));
      Serial.print(" Luminosity: " +String(sensorLDRData)); 
      Serial.println(" Publish: " + String(publishNewState));
    if (publishNewState){
        reconnectWiFi();
        String tempJson =  aggregateSensorData(sensorDHTData, String(""), String("temperature"), String("M14-DHT11"));
        Serial.println(tempJson);
       //Merge JsonObjects
        tempJson = aggregateSensorData(sensorLDRData, tempJson, String("luminosity"), String("M14-LDR"));
        // On or off option
        bool lightOn = false;
        if (sensorLDRData.toInt() > 600){
          lightOn = true;
        }
        tempJson = aggregateSensorData(String(lightOn), tempJson, String("lightOn"), String("M14-LDR"));
        // Just a test to include more than one sensor for a phenomenon
        tempJson = aggregateSensorData(String(lightOn), tempJson, String("lightOn"), String("M15-LDR"));

        Serial.println(tempJson);
        send2Firebase(tempJson);
        publishNewState = false;
    }

    //Sleep for 1 sec
    //ESP.deepSleep( 1000000, WAKE_RFCAL);
    delay(1000);
}

/**************************************/
/*     INTITIAL CONFIGURATIONS FUNC   */
/**************************************/
void initWiFi() {
    delay(10);
    Serial.println("Connecting Wifi: " + String(WLAN_SSID));

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
void initFirebase(){
    Serial.println("Setting up Firebase");
    Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);
}

void initTicker(){
    ticker.attach_ms(PUBLISH_INTERVAL, publish);
}

void publish(){
    publishNewState = true;
}

void send2Firebase(String dataObject){
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
    
    // Push to Firebase
    Serial.print("Send to Firebase: ");
    Firebase.push(ABS_PATH_JSON, sendObject); 
    // Assert success with some code
    if (Firebase.success()){
      Serial.println(" OK!");
    }
    else{
      Serial.println(" Fail!");
    }  
}
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

/**************************************/
/*          LUMINOSITY  SETUP         */
/**************************************/
int readLum(){
  int analogValue = analogRead(LDR_PIN);
  return analogValue;
}

///**************************************/
///*          TEMPERATURE  SETUP        */
///**************************************/
float readDHTTemp(){
    return dht.readTemperature();/* Get temperature value */
}
