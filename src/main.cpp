#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <secrets.h>

//General Macros
#define TEMPERATURE_BUFFER_SIZE 1023

//SPI Pin Number Macros
#define SCK   4 
#define CS    5 
#define MISO  6 

//Task intervals

  //Reading interval
  unsigned long previousDataSampleMilis = 0;
  const long dataSampleInterval = 15000; 

  //Publishing interval
  unsigned long previousPublishedMilis = 0;
  const long publishInterval = 5000; 

  //WiFi check interval
  unsigned long previousWifiCheck = 0;
  const long wifiCheckInterval = 5000; 

//MQTT Credentials
const char* mqtt_url = MQTT_URL;
const int mqtt_port = 8883;
const char* mqtt_user = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;


//MQTT client configuration
WiFiClientSecure espClient;
PubSubClient    client(espClient);

//Temperature purposes Global Vsariables
float temperatureBuffer[TEMPERATURE_BUFFER_SIZE];
uint16_t bufferCount = 0;
float temperatura = 0.0;
uint16_t dataStored = 0;

//Flags
bool wifiConnectedPreviously = false;

//Function Prototypes
void spiBitBanging(uint8_t ClockCycles, uint16_t period);
bool connectMQTT();
void taskReadSensor();
void taskPublishData();
void taskCheckWiFi();
void startupConnectionConfig();


void setup() {

  //Serial monitor initialization (only used for debug purposes)  
  Serial.begin(115200);
  delay(1500);
  Serial.println("Serial Ready");
  
  //SPI'S pin configuration
  pinMode(MISO, INPUT);
  pinMode(SCK, OUTPUT);
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  digitalWrite(SCK, LOW); 

  delay(3000);
  
  startupConnectionConfig();
  
}

void loop() {

  unsigned long currentMilis = millis();

  //Read temperature (Currently every 15 seconds)

  if(currentMilis - previousDataSampleMilis >= dataSampleInterval){
    previousDataSampleMilis = currentMilis;
    taskReadSensor();
  }

//Publish Data (Currently every 5 seconds)

  if(currentMilis - previousPublishedMilis >= publishInterval){
    previousPublishedMilis = currentMilis;
    taskPublishData();
  }
  //Check wifi (Currently every 5 seconds)

  if(currentMilis - previousWifiCheck >= wifiCheckInterval){
    previousWifiCheck = currentMilis;
    taskCheckWiFi();
  }

}

void taskReadSensor(){

  //Get raw value from sensor
  spiBitBanging(16,100);

  Serial.println(dataStored, BIN);

  //If there's space left on the buffer, we stored the data
  if(bufferCount < TEMPERATURE_BUFFER_SIZE){

    //Conversion from raw value to temperature in celcius
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

    //The path sensor/temperature creates the topic on which the client will publish the data
    //The true parameter activates the Message Retained option
    //Message Retained option allows new subscribers to receive the last message sent

    if(client.publish("sensor/temperatura", payload.c_str(), true)){
          Serial.println("Temperature successfully published");

          //FIFO structure
          for(int i = 0; i<bufferCount - 1; i++){
            //If data publish was successful
            //We shift the values by one 
            temperatureBuffer[i] = temperatureBuffer[i+1];
          }
          bufferCount--;

    }else{
          //In case of an error, data remains stored
          Serial.println("Error while publishing the temperature");
    }
    
  }
}

void spiBitBanging(uint8_t ClockCycles, uint16_t period){
  uint16_t buffer = 0;

  digitalWrite(CS, LOW);
  delay(1000);

  //Generate N amount of Clock Cycles
  for (int i = 0; i<ClockCycles; i++){

    //High pulse
    digitalWrite(SCK, HIGH);
    Serial.println("HIGH");
    delay(period/4);

    //Data sample time
    Serial.println("SAMPLE");
    dataStored <<= 1;
    dataStored |= digitalRead(MISO);
    delay(period/4);
    
    //High pulse
    digitalWrite(SCK, LOW);
    Serial.println("LOW");
    delay(period/2);

  }

  delay(100);
  digitalWrite(CS, HIGH);

}

bool connectMQTT(){

  if(!client.connected()){
    Serial.println("Connecting to HiveMQ..");
    String clientId = "ESP32S3-Client-";
    clientId += String(random(0xffff), HEX);

    if(client.connect(clientId.c_str(), mqtt_user, mqtt_password)){
      Serial.println("Conectado!");
      return true;
    }else{
      Serial.print(" error, rc=");
      Serial.print(client.state());
      Serial.println("Retrying in 5 seconds");
      return false;

    }
  }
  return true;

}

void taskCheckWiFi(){

  //First check if wifi is not currently connected
  if(WiFi.status() != WL_CONNECTED){
      //If it was connected previously, it means it disconnected
      if(wifiConnectedPreviously){
        Serial.println("Wifi disconnected, retrying");
        WiFi.begin(SSID, PASSWORD);
        wifiConnectedPreviously = false;
      }

  }else{

    // WiFi is connected
    //If it was not connected previously, it has reconnected
    if(!wifiConnectedPreviously){
      Serial.print("IP ADDRESS: ");
      Serial.println(WiFi.localIP());

      //We set the flag to make sure to only run once this section
      wifiConnectedPreviously = true;
    } 
  }

  if((time(nullptr)<1000000000)){
    configTime(0,0,"pool.ntp.org", "time.nist.gov"); //NEED TO GET THE CURRENT TIME SO THE CERTIFICATE WONT INVALIDATE THE CONNECTION
    Serial.println("Sincronizando hora");
  }

}

void startupConnectionConfig(){

  //WiFi connection

  WiFi.begin(SSID, PASSWORD);
  while(WiFi.status()!= WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  wifiConnectedPreviously = true;

  //MQTT configuration

  espClient.setInsecure();
  client.setServer(mqtt_url, mqtt_port);

}