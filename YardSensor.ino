#include <Arduino.h>
#include <WiFi.h>
#include "MyConfig.h"

#define PROJECT_VERSION "1.0.0"
#define PROJECT_NAME "YARD_SENSOR"
#define ID 0xA2
#define START_BYTE 0x30
#define STOP_BYTE 0x31
#define GET_BYTE 0x32
#define SET_BYTE 0x33
#define PAYLOAD_BYTE 0x34

#define SENSOR1 0x60

#define STATUS 0xc0
#define READY 0xc1
#define FREE 0xc3
#define CONTEST1 0xc4
#define CONTEST2 0xc5
#define CONTEST3 0xc6
#define CONTEST4 0xc7

uint8_t SS_PIN[8] = {SS1_PIN, SS2_PIN, SS3_PIN, SS4_PIN, SS5_PIN, SS6_PIN, SS7_PIN, SS8_PIN};

const char *ssid = "PC_NET_MOBILE";
const char *password = "1234567890";

WiFiServer server(123);
WiFiClient client;

unsigned long previousMillis = 0;
uint32_t lastSensorData = 0;

struct FrameConvert
{
    uint8_t start;
    uint8_t key;
    uint8_t getset;
    uint8_t id;
    uint32_t data;
    uint8_t stop;
};

void setup()
{
    Serial.begin(115200);
    Serial.println("Yard Sensor Initialized");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Khởi tạo server
    server.begin();
    Serial.println("TCP Server started...");

    for (int i = 0; i < 8; i++)
    {
        pinMode(SS_PIN[i], INPUT);
    }
}
void loop()
{
    client = server.available();
    if (client)
    {
        Serial.println("New client connected");
        while (client.connected())
        {
            if (client.available())
            {
                char c = client.read();
                Serial.print("Received: ");
                Serial.println(c);
            }
            uint32_t sensorData = readSensorData();
            if (sensorData != lastSensorData)
            {
                lastSensorData = sensorData;
                Serial.println("Sensor data changed, sending update...");
                FrameConvert frame;
                frame.start = START_BYTE;
                frame.key = STATUS;
                frame.getset = GET_BYTE;
                frame.id = ID;
                frame.data = sensorData;
                frame.stop = STOP_BYTE;
                uint8_t buffer[10];
                ConvertFrameToBytes(buffer, frame);
                client.write(buffer, sizeof(buffer));
                Serial.println("Send: ");
                for (int i = 0; i < sizeof(buffer); i++)
                {
                    Serial.print("0x");
                    Serial.print(buffer[i], HEX);
                    Serial.print(" ");
                }
                Serial.println();
            }
            if (millis() - previousMillis >= 5000)
            {
                previousMillis = millis();
            }
        }
        client.stop();
        Serial.println("Client disconnected");
    }
    // Placeholder for yard sensor logic
    // Serial.println("Reading yard sensor data...");
    delay(100); // Simulate a delay between readings
}
void ConvertFrameToBytes(uint8_t *buffer, FrameConvert &frame)
{
    buffer[0] = START_BYTE;
    buffer[1] = frame.key;
    buffer[2] = frame.getset;
    buffer[3] = frame.id;
    buffer[4] = (frame.data >> 24) & 0xFF;
    buffer[5] = (frame.data >> 16) & 0xFF;
    buffer[6] = (frame.data >> 8) & 0xFF;
    buffer[7] = frame.data & 0xFF;
    buffer[8] = crc8(buffer);
    buffer[9] = STOP_BYTE;
}
uint8_t crc8(const uint8_t *data)
{
    uint8_t crc = 0x00;  // giá trị khởi tạo
    uint8_t poly = 0x07; // đa thức CRC-8

    for (int i = 0; i < 8; i++)
    {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ poly;
            else
                crc <<= 1;
        }
    }
    return crc;
}
uint32_t readSensorData()
{
    uint32_t data = 0;
    for (int i = 0; i < 8; i++)
    {
        data <<= 1;
        data |= digitalRead(SS_PIN[i]);
    }
    Serial.print("Sensor Data: 0b");
    Serial.println(data, BIN);

    return data;
}