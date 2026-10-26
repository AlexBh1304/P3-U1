# ESP32-S3 MQTT Temperature Node

An **ESP32-S3** reads a temperature sensor over a **bit-banged SPI** interface and publishes the readings to a **cloud MQTT broker (HiveMQ) over TLS**. Readings are buffered on the device, so no data is lost while the network is down.


## What this project shows

| Area | Implementation |
|------|----------------|
| **SPI** | Software (bit-banged) SPI master written with plain GPIO: chip select, clock generation and bit sampling |
| **Sensor decoding** | 16-bit frame, 12-bit temperature field extracted with shifts and masks, 0.25 °C per LSB |
| **Scheduling** | Non-blocking cooperative tasks driven by `millis()` (no RTOS, no blocking loop) |
| **Connectivity** | WiFi with automatic reconnection, MQTT over TLS (port 8883) |
| **Reliability** | FIFO store-and-forward buffer: a sample is removed only after a successful publish |
| **Security hygiene** | Credentials kept out of the source in `secrets.h` |

## Architecture

```
 Sensor --SPI--> [taskReadSensor]  every 15 s
                        |
                 FIFO buffer (1023 samples)
                        |
                 [taskPublishData]  every 5 s ---> MQTT broker (TLS, retained)
                                                    topic: sensor/temperatura
 [taskCheckWiFi]  every 5 s: reconnect WiFi, sync time (NTP)
```

## Hardware

| Signal | GPIO |
|--------|------|
| SCK | 4 |
| CS | 5 |
| MISO | 6 |

## How it works

### Bit-banged SPI

`spiBitBanging()` pulls CS low, generates 16 clock cycles on SCK and shifts in one bit per cycle from MISO, most significant bit first. The temperature is the 12-bit field in bits 14 to 3 of the frame:

```c
temperature = ((raw >> 3) & 0xFFF) * 0.25;
```


### Store-and-forward buffer

Each reading is appended to a 1023-sample FIFO. Every 5 seconds the oldest sample is published, and it is removed from the buffer only if `client.publish()` succeeds. If the network or broker is down, samples accumulate (about 4 hours at one reading every 15 s) and are sent in order once the connection returns.

### MQTT

- Broker: HiveMQ Cloud, TLS on port 8883
- Topic: `sensor/temperatura`
- Messages are published with the **retain** flag, so a new subscriber immediately receives the last value


### Connection handling

`taskCheckWiFi()` detects a lost connection and restarts it, and `taskPublishData()` reconnects the MQTT client when needed. The time is synchronized with NTP so certificate-based TLS can work.

## Configuration

Create `secrets.h` (it is not committed):

```c
#define SSID          "your-wifi-name"
#define PASSWORD      "your-wifi-password"
#define MQTT_URL      "your-cluster.s1.eu.hivemq.cloud"
#define MQTT_USER     "your-mqtt-user"
#define MQTT_PASSWORD "your-mqtt-password"
```

## Build

Arduino framework for ESP32-S3 with the `PubSubClient` library.

## Known limitations and next steps

- TLS certificate validation is currently disabled (`setInsecure()`); the next step is to pin the broker's CA certificate
- The sensor read uses blocking delays (about 3 seconds per read); moving to hardware SPI or a non-blocking state machine would remove them
- When the buffer is full, new readings are dropped; a ring buffer that overwrites the oldest data is an alternative
- Debug prints inside the SPI routine should be removed or made conditional
