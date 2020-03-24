
//Master --> Sensors

#include <ESP8266WiFi.h>
extern "C" {
  #include <espnow.h>
}
#include <FirebaseArduino.h>
#include <Ticker.h>
#include "env.h" // Definition of enviromnent variables
// Make sure you have a env.h, following the env_sample.h
#define LDR_PIN A0
#define CHANNEL 1 // CHANNEL Used on communication

// Send Data  -- It could be a struct
struct PackData{
   uint16_t luminosity = 0;
};



uint8_t macSlaves[] =  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // MAC ADDRES OF SLAVE

/*************************************/
/*          PROTOTYPES               */
/**************************************/
void   initFirebase();
void   initTicker();
void   initESPNow();
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
Ticker ticker; //Set interval of publishing to Firebase
bool publishNewState = true;
/**************************************/
/*                SETUP               */
/**************************************/
//this sets the ground pin to LOW and the input voltage pin to high
void setup() {
    Serial.begin(9600);
    //    initWiFi();


    //Init ESP_NOW
    initESPNow();
    Serial.print("Mac Address in Station: ");
    Serial.println(WiFi.macAddress());

    initTicker();

    Serial.println("Lum");

    //0=OCIOSO, 1=MAESTRO, 2=ESCLAVO y 3=MAESTRO+ESCLAVO
    esp_now_set_self_role(1);
uint8_t mac_addr[6] = {0xA2, 0x20, 0xA6, 0x17, 0x37, 0x46}; 
       
    uint8_t role=2;  
    uint8_t channel= CHANNEL;
    uint8_t key[0]={};   //no hay clave 
    //uint8_t key[16] = {0,255,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
    uint8_t key_len=sizeof(key);
    
    esp_now_add_peer(mac_addr,role,channel,key,key_len);
}

/**************************************/
/*                LOOP                */
/**************************************/
void loop(){
   // String sensorLDRData = String(readLum());
   
    PackData PD;
    PD.luminosity = readLum();
    Serial.print("Luminosity: ");Serial.println(PD.luminosity);
    uint8_t da[6] = {0xA2, 0x20, 0xA6, 0x17, 0x37, 0x46};  // Slave
    //uint8_t da[6] = {0x5C, 0xCF, 0x7F, 0x16, 0x50, 0xB9};
    
    uint8_t data[sizeof(PD)] ; memcpy(data, &PD, sizeof(PD));
    int len = sizeof(data);
    esp_now_send(da, data, len);
    delay(400);

      //***VERIFICACIÃ“N DE LA RECEPCIÃ“N CORRECTA DE LOS DATOS POR EL ESCLAVO***//
  esp_now_register_send_cb([](uint8_t* mac, uint8_t status) {
    char MACslave[6];
    sprintf(MACslave,"%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    Serial.print(". Sending to ESP MAC: "); Serial.print(MACslave);
    Serial.print(". Success (0=0K - 1=ERROR): "); Serial.println(status);
  });
    

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


void initESPNow() {
 if (esp_now_init()!=0) {
    Serial.println("*** ESP_Now init failed");
    ESP.restart();
    delay(1);
  }
}
void initTicker(){
    ticker.attach_ms(PUBLISH_INTERVAL, publish);
}

void publish(){
    publishNewState = true;
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
    JsonObject& dataObject = jsonBuffer.createObject();
    char jsonChar[500];
    if(srcData != ""){ // If srcData is not an empty json struct, parse srcData

        jsonBuffer.clear();
        JsonObject& dataObject = jsonBuffer.parseObject(srcData);
    }
    // If sensorType already included on the srcData, just include another sensorID on the nested object
    if(dataObject.containsKey(sensorType)){

        dataObject[sensorType][sensorID] = sensorData;
    } else {

        JsonObject& sensorObject = dataObject.createNestedObject(sensorType);
        sensorObject[sensorID] =  sensorData ;
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
