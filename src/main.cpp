#include <Arduino.h>


#define SCK   4 //OK
#define CS    5 //OK
#define MISO  6 //ok

void SPI_BIT_BANGING(uint8_t ClockCycles, uint16_t period);



uint8_t flag = 0;
uint16_t dataStored = 0;

void setup() {
  Serial.begin(115200);

  // Espera a que el puerto serial esté activo
  while (!Serial) {
    delay(10);
  }
  delay(1500); // Margen para que el monitor serie enganche

  Serial.println("LISTO");

  //ledcAttach(5,50, 14);
  //ledcWrite(5,8192);

  pinMode(MISO, INPUT);
  pinMode(SCK, OUTPUT);
  pinMode(CS, OUTPUT);
  



  digitalWrite(CS, HIGH);
  digitalWrite(SCK, LOW); 

  delay(3000);


}

void loop() {

  //CONFIRMAREMOS QUE EL GPIO 5 ESTÉ TOGGLEANDO

    if(!flag){

        SPI_BIT_BANGING(16,100);

        flag++;
        
        Serial.println(dataStored, BIN);
        Serial.println(((dataStored >> 3) & 0xFFF) * 0.25);

    }
  
}


void SPI_BIT_BANGING(uint8_t ClockCycles, uint16_t period){
  uint16_t buffer = 0;

  digitalWrite(CS, LOW);
  delay(1000);

  //GENERATE N AMOUNT OF CLOCK CYCLES
  for (int i = 0; i<ClockCycles; i++){

    //PULSO HIGH
    digitalWrite(SCK, HIGH);
    Serial.println("HIGH");
    delay(period/4);

    //DATA SAMPLE
    Serial.println("SAMPLE");


    dataStored <<= 1;
    dataStored |= digitalRead(MISO);
    
    delay(period/4);
    
    //LOW PULSE

    digitalWrite(SCK, LOW);
    Serial.println("LOW");
    delay(period/2);

  }

  delay(100);
  digitalWrite(CS, HIGH);

}