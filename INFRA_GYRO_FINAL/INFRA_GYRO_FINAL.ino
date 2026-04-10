#include <SPI.h>
#include <SD.h>
#include <Adafruit_ICM20X.h>
#include <Adafruit_ICM20948.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_ICM20948 icm;
uint16_t measurement_delay_us = 65535;
// #define ICM_CS 10
// For software-SPI mode we need SCK/MOSI/MISO pins
#define ICM_SCK 13
#define ICM_MISO 12
#define ICM_MOSI 11
#define SD_CS 10


// int chipSelect = 10;
const int buttonPin = 2;
const int ledPin = 3;
const int maxReadings = 600; 
const long interval = 100;
bool isRecording = false;
int readingCount = 0;
unsigned long previousMillis = 0;
File dataFile;
char fileName[15]; // buffer to hold the filename string

void setup() {

Wire.setClock(100000);

Serial.begin(9600);

if (!icm.begin_I2C(0x69)) {
    // if (!icm.begin_SPI(ICM_CS)) {
    // if (!icm.begin_SPI(ICM_CS, ICM_SCK, ICM_MISO, ICM_MOSI)) {

    Serial.println("Failed to find ICM20948 chip");
    while (1) {
      delay(10);
    }
  }

pinMode(buttonPin, INPUT_PULLUP); 
pinMode(ledPin, OUTPUT);

if (!SD.begin(SD_CS)) {
Serial.println("sd error");
while (1); // stop if sd card initialization fails
  }
  uint8_t gyro_divisor = icm.getGyroRateDivisor();
  float gyro_rate = 1100 / (1.0 + gyro_divisor);

  Serial.print("Gyro data rate divisor set to: ");
  Serial.println(gyro_divisor);
  Serial.print("Gyro data rate (Hz) is approximately: ");
  Serial.println(gyro_rate);
}
void loop() {

  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t mag;
  sensors_event_t temp;  

  if (digitalRead(buttonPin) == LOW && !isRecording) { 
    // look for the next available file name (run0.csv, run1.csv...)
  int fileNum = 0;
  while (true) {
    sprintf(fileName, "run%d.csv", fileNum);
    if (!SD.exists(fileName)) {
      break; // we found a name that doesn't exist yet
    }
fileNum++;
}

dataFile = SD.open(fileName, FILE_WRITE);

if (dataFile) {
  isRecording = true;
  readingCount = 0;
  digitalWrite(ledPin, HIGH); // turn on the led to show it is recording
  Serial.print("recording to: ");
  Serial.println(fileName);
  delay(500); // debounce the button so it doesn't double-trigger
  }

}
  
// automatic recording logic
if (isRecording) {
unsigned long currentMillis = millis();
// check if it is time to take the next reading
if (currentMillis - previousMillis >= interval) {
  previousMillis = currentMillis;

    icm.getEvent(&accel, &gyro, &temp, &mag);
    
    // basic sanity check - gyro values above 10 rad/s are likely corrupt
    if (abs(gyro.gyro.y) > 10.0) {
    Serial.println("bad reading, skipping");
} else {
    dataFile.print(gyro.gyro.x); dataFile.print(",");
    dataFile.print(gyro.gyro.y); dataFile.print(",");
    dataFile.println(gyro.gyro.z);
    dataFile.flush();
    
    Serial.print("Gyro X: "); Serial.print(gyro.gyro.x);
    Serial.print(" Y: ");     Serial.print(gyro.gyro.y);
    Serial.print(" Z: ");     Serial.println(gyro.gyro.z);
    
    readingCount++;
    
    if (readingCount >= maxReadings) {
        isRecording = false;
        dataFile.close();
        digitalWrite(ledPin, LOW);
        Serial.println("finished. file closed.");
    }
}

   }
  }
}