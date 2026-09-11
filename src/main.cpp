#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#define SCK   4 //OK
#define CS    5 //OK
#define MISO  6 //ok


#define TEMPERATURE_BUFFER_SIZE 1023



//#define SSID "Mega-2.4G-32AD"
//#define PASSWORD "f3mXcWDz7C"


#define SSID "iPhone"
#define PASSWORD "alex12345"


unsigned long previousDataSampleMilis = 0;
const long dataSampleInterval = 15000; //USed in milis


unsigned long previousPublishedMilis = 0;
const long publishInterval = 5000; //USed in milis

bool wifiConnectedPreviously = false;


unsigned long previousWifiCheck = 0;
const long wifiCheckInterval = 5000; //USed in milis



uint16_t bufferCount = 0;


const char* mqtt_url = "bdd53fed217747629eb941f979434002.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;

const char* mqtt_user = "esp32_s3";
const char* mqtt_password = "passwordjeje";

WiFiClientSecure espClient;
PubSubClient    client(espClient);




float temperatureBuffer[TEMPERATURE_BUFFER_SIZE];




void SPI_BIT_BANGING(uint8_t ClockCycles, uint16_t period);
bool connectMQTT();



void taskReadSensor();


void taskPublishData();


void taskCheckWiFi();

uint8_t flag = 0;
uint16_t dataStored = 0;
float temperatura = 0.0;

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


  WiFi.begin(SSID, PASSWORD);
  while(WiFi.status()!= WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  wifiConnectedPreviously = true;





  espClient.setInsecure(); //CERTIFICATE TO VALODAITE SERVER AUTHENTICITY
  client.setServer(mqtt_url, mqtt_port); //COnnect to the mqtt server


  
}

void loop() {

  unsigned long currentMilis = millis();

//Read temperature

  if(currentMilis - previousDataSampleMilis >= dataSampleInterval){
    previousDataSampleMilis = currentMilis;
    taskReadSensor();
  }

//Publish Data

  if(currentMilis - previousPublishedMilis >= publishInterval){
    previousPublishedMilis = currentMilis;
    taskPublishData();
  }


  //Check wifi

  if(currentMilis - previousWifiCheck >= wifiCheckInterval){
    previousWifiCheck = currentMilis;
    taskCheckWiFi();
  }


      
}




void taskReadSensor(){

  SPI_BIT_BANGING(16,100);

  Serial.println(dataStored, BIN);

  if(bufferCount < TEMPERATURE_BUFFER_SIZE){

    temperatureBuffer[bufferCount] = ((dataStored >> 3) & 0xFFF) * 0.25; 
    bufferCount++;
  }
  
  Serial.println(temperatureBuffer[bufferCount -1]);


}


void taskPublishData(){
  
  if(!client.connected()){
    connectMQTT();
    return;
  }
  client.loop();


  if(bufferCount > 0){

  String payload = String(temperatureBuffer[0]);

  // LA RUTA CREA EL TOPIC DONDE SE PUBLICARÁ, EL PARAMETRO TRUE INDICA QUE SSE UTILIZARÁ EL MESSAGE RETAINED
  //EL MESSAGE RETAINED SIRVE PARA QUE EN CUANTO UN NUEVO CLIENTE SE SUSCRIBA AL TOPIC, LEA EL UTLIMO MENSAJE
  if(client.publish("sensor/temperatura", payload.c_str(), true)){
        Serial.println("Temperatura publicada con éxito");

        for(int i = 0; i<bufferCount - 1; i++){
          temperatureBuffer[i] = temperatureBuffer[i+1];
        }
        bufferCount--;

  }else{
        Serial.println("Error al publicar la temperatura");
  }
  
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

bool connectMQTT(){
  if(!client.connected()){
    Serial.println("Conectando a HIVEMQ..");
    String clientId = "ESP32S3-Client-";
    clientId += String(random(0xffff), HEX);

    if(client.connect(clientId.c_str(), mqtt_user, mqtt_password)){
      Serial.println("Conectado!");
      return true;
    }else{
      Serial.print(" fallo, rc=");
      Serial.print(client.state());
      Serial.println("Intentando de nuevo en 5 segundos");
      return false;

    }
  }
  return true;

}


void taskCheckWiFi(){


  if(WiFi.status() != WL_CONNECTED){
      if(wifiConnectedPreviously){
        Serial.println("Wifi disconnected, retrying");
        WiFi.begin(SSID, PASSWORD);
        wifiConnectedPreviously = false;
      }

  }else{
    if(!wifiConnectedPreviously){
      Serial.print("IP ADDRESS: ");
      Serial.println(WiFi.localIP());
      wifiConnectedPreviously = true;
    } 
  }

  if((time(nullptr)<1000000000)){
    configTime(0,0,"pool.ntp.org", "time.nist.gov"); //NEED TO GET THE CURRENT TIME SO THE CERTIFICATE WONT INVALIDATE THE CONNECTION
    Serial.println("Sincronizando hora");
  }

}