/*
 * MOSI (DI) - 8
 * MISO (DO) - 7
 * CLK (SCK) - 14
 * CS - 15
 */
 
#include <SPI.h>
#include <SD.h>
#include "SdFat.h"
#include <RH_RF95.h>
 
#define RFM95_CS 8
#define RFM95_RST 9
#define RFM95_INT 2
 
// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

#define FILE_BASE_NAME "Data"
 
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

const int chipSelect = 15;

struct data {
  double alt;
  double vertical_velocity;
  double horizontal_velocity;
  double vertical_acceleration;
  double horizontal_acceleration;
};

data datapacket = {5.0, 4.0, 3.0, 2.0, 1.0};

File dataFile;

void setup() 
{
  //const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  
  //pinMode(chipSelect, OUTPUT);
  
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  pinMode(RFM95_INT, INPUT);
 
  while (!Serial);
  Serial.begin(9600);
  delay(100);
 
  Serial.println("Arduino LoRa TX Test!");
 
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
 
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
 
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(5, false);


  /*while (sd.exists(fileName)) {
    if (fileName[BASE_NAME_SIZE + 1] != '9') {
      fileName[BASE_NAME_SIZE + 1]++;
    } else if (fileName[BASE_NAME_SIZE] != '9') {
      fileName[BASE_NAME_SIZE + 1] = '0';
      fileName[BASE_NAME_SIZE]++;
    } else {
      error("Can't create file name");
    }
  }
  
  if (!file.open(fileName, O_WRONLY | O_CREAT | O_EXCL)) {
    error("file.open");
  }*/
}
 
int16_t packetnum = 0;  // packet counter, we increment per xmission
 
void loop()
{  
  
  Serial.println("Sending to rf95_server");
  // Send a message to rf95_server

  Serial.print("Sending "); Serial.println(toString(datapacket));
  
  Serial.println("Sending..."); delay(10);
  rf95.send((uint8_t *)&datapacket, sizeof(datapacket));
 
  Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();
 
  Serial.println("Writing data to file");

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(toString(datapacket));
    Serial.println("Completed write to file");
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }
}

String toString(data datapacket) {
  String dataString = "Altitude: " + String(datapacket.alt)
    + ".  Vertical Velocity: " + String(datapacket.vertical_velocity) 
    + ".  Horizontal Velocity: " + String(datapacket.horizontal_velocity) 
    + ".  Vertical Acceleration: " + String(datapacket.vertical_acceleration)
    + ".  Horizontal Acceleration: " + String(datapacket.horizontal_acceleration);
  return dataString;
}
