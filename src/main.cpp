#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#define SCK   4 //OK
#define CS    5 //OK
#define MISO  6 //ok




#define SSID "Mega-2.4G-32AD"
#define PASSWORD "f3mXcWDz7C"

const char* mqtt_url = "bdd53fed217747629eb941f979434002.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
//const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPu7OCiwAswDQYJKoZIhvcNAQELBQAw\n" \
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n" \
"WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n" \
"ZXQgU2VjdXJpdHkgUmVzZWFyY2ggUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElT\n" \
"UkcgUmovV2FzQ1RDeENvVEFTU2pXU3M0bFF0d2VwM0xUSXpORW9BMTBGQ0NBSTBD\n" \
"DQYJKoZIhvcNAQEBBQADggIBAADCCAEBCgoCggEBANW1L4V0vnzsl6vwCz88K8nQ\n" \
"0S8Y1A4i1W74R0V5C2F6x4w7E8G9B0C1D2E3F4G5H6I7J8K9L0M1N2O3P4Q5R6S7\n" \
"T8U9V0W1X2Y3Z4a5b6c7d8e9f0g1h2i3j4k5l6m7n8o9p0q1r2s3t4u5v6w7x8y9\n" \
"z0A1B2C3D4E5F6G7H8I9J0K1L2M3N4O5P6Q7R8S9T0U1V2W3X4Y5Z6a7b8c9d0e1\n" \
"f2g3h4i5j6k7l8m9n0o1p2q3r4s5t6u7v8w9x0y1z2A3B4C5D6E7F8G9H0I1J2K3\n" \
"L4M5N6O7P8Q9R0S1T2U3V4W5X6Y7Z8a9b0c1d2e3f4g5h6i7j8k9l0m1n2o3p4q5\n" \
"r6s7t8u9v0w1x2y3z4A5B6C7D8E9F0G1H2I3J4K5L6M7N8O9P0Q1R2S3T4U5V6W7\n" \
"X8Y9Z0a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3x4y5z6A7B8C9\n" \
"D0E1F2G3H4I5J6K7L8M9N0O1P2Q3R4S5T6U7V8W9X0Y1Z2a3b4c5d6e7f8g9h0i1\n" \
"j2k3l4m5n6o7p8q9r0s1t2u3v4w5x6y7z8A9B0C1D2E3F4G5H6I7J8K9L0M1N2O3\n" \
"P4Q5R6S7T8U9V0W1X2Y3Z4a5b6c7d8e9f0g1h2i3j4k5l6m7n8o9p0q1r2s3t4u5\n" \
"v6w7x8y9z0A1B2C3D4E5F6G7H8I9J0K1L2M3N4O5P6Q7R8S9T0U1V2W3X4Y5Z6==\n" \
"-----END CERTIFICATE-----\n";



const char* mqtt_user = "esp32_s3";
const char* mqtt_password = "passwordjeje";

WiFiClientSecure espClient;
PubSubClient    client(espClient);







void SPI_BIT_BANGING(uint8_t ClockCycles, uint16_t period);
void connectMQTT();


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


  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  configTime(0,0,"pool.ntp.org", "time.nist.gov"); //NEED TO GET THE CURRENT TIME SO THE CERTIFICATE WONT INVALIDATE THE CONNECTION
  Serial.println("Sincronizando hora");
  while((time(nullptr)<1000000000)){
    delay(500);
    Serial.print(".");
  }

  Serial.println("Hora sincronizada");
  
  espClient.setInsecure(); //CERTIFICATE TO VALODAITE SERVER AUTHENTICITY
  client.setServer(mqtt_url, mqtt_port); //COnnect to the mqtt server



  
}

void loop() {

  //CONFIRMAREMOS QUE EL GPIO 5 ESTÉ TOGGLEANDO


  if(!client.connected()){
    connectMQTT();

  }
  client.loop();

    if(!flag){

        SPI_BIT_BANGING(16,100);

        flag++;
        
        Serial.println(dataStored, BIN);
        temperatura = ((dataStored >> 3) & 0xFFF) * 0.25;
        Serial.println(temperatura);

        String payload = String(temperatura);
        // LA RUTA CREA EL TOPIC DONDE SE PUBLICARÁ, EL PARAMETRO TRUE INDICA QUE SSE UTILIZARÁ EL MESSAGE RETAINED
        //EL MESSAGE RETAINED SIRVE PARA QUE EN CUANTO UN NUEVO CLIENTE SE SUSCRIBA AL TOPIC, LEA EL UTLIMO MENSAJE
        if(client.publish("sensor/temperatura", payload.c_str()), true){
          Serial.println("Temperatura publicada con éxito");
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

void connectMQTT(){
  while(!client.connected()){
    Serial.println("Conectando a HIVEMQ..");
    String clientId = "ESP32S3-Client-";
    clientId += String(random(0xffff), HEX);

    if(client.connect(clientId.c_str(), mqtt_user, mqtt_password)){
      Serial.println("Conectado!");

    }else{
      Serial.print(" fallo, rc=");
      Serial.print(client.state());
      Serial.println("Intentando de nuevo en 5 segundos");
      delay(5000);

    }
  }
}