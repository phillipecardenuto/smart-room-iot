//Slave --> LED
#include <ESP8266WiFi.h>
extern "C" {
  #include <espnow.h>
}
#define CHANNEL 1
int led = D1;           // the PWM pin the LED is attached to
int brightness = 0;    // how bright the LED is
int fadeAmount = 5;    // how many points to fade the LED by

// Send Data  -- It could be a struct
struct PackData{
   float sensorData;
   char sensorType[21];
   char sensorID[21];
};

/*************************************/
/*          PROTOTYPES               */
/**************************************/
void initESPNow();

void initESPNow() {
 if (esp_now_init()!=0) {
    Serial.println("*** ESP_Now init failed");
    ESP.restart();
    delay(1);
  }
}


// the setup routine runs once when you press reset:
void setup() {
  Serial.begin(9600);

  //Init ESP_NOW
 initESPNow();
 
//***DATOS DE LAS MAC (Access Point y Station) del ESP***//
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress()); 

  pinMode(led, OUTPUT);
  digitalWrite(led,HIGH); // turn led OFF
  delay(1000);
  
  // 0=OCIOSO, 1=MAESTRO, 2=ESCLAVO y 3=MAESTRO+ESCLAVO 
  esp_now_set_self_role(2);   
}

// the loop routine runs over and over again forever:
void loop() {

    esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {
        PackData PD;
        memcpy(&PD, data, sizeof(PD));
    
        char macMaster[18];
        sprintf(macMaster, "%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
        Serial.print("Recepcion desde ESP MAC: "); Serial.print(macMaster);Serial.println();
        Serial.print(PD.sensorType);Serial.print(" "+String(PD.sensorID)+": "); Serial.println(PD.sensorData);
    //  analogWrite(led, int(PD.sensorData));
        delay(30);
        });
}
