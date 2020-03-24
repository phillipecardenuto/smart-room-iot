#define PIN_SENSOR D4

int number_of_activation =0;
void setup() {
  // put your setup code here, to run once:
  pinMode(PIN_SENSOR, INPUT);
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(PIN_SENSOR), PIRActivated, RISING);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(number_of_activation); 
//    if(sinal == HIGH ){
//    //aciona o Buzzer
//    Serial.print("ON: ");Serial.println(sinal);
//  }
//  else{
//   Serial.print("OFF");Serial.println(sinal);
//    }
  delay(1000);
  
}
void PIRActivated(){
  number_of_activation+=1;
}
