/*
   Simple data logger.
*/
#include <SPI.h>
#include "SdFat.h"
#include <RH_RF95.h>

#define RFM95_CS 8
#define RFM95_RST 9
#define RFM95_INT 2

#define FILE_BASE_NAME "Data"

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

struct data {
  double alt;
  double vertical_velocity;
  double horizontal_velocity;
  double vertical_acceleration;
  double horizontal_acceleration;
};

data datapacket = {5.0, 4.0, 3.0, 2.0, 1.0};

//---------------------------------------------------------------------------

// SD chip select pin.  Be sure to disable any other SPI devices such as Enet.
const uint8_t chipSelect = SS;

// Interval between data records in milliseconds.
// The interval must be greater than the maximum SD write latency plus the
// time to acquire and write data to the SD to avoid overrun errors.
// Run the bench example to check the quality of your SD card.
const uint32_t SAMPLE_INTERVAL_MS = 1000;

// Log file base name.  Must be six characters or less.
#define FILE_BASE_NAME "Data"
//------------------------------------------------------------------------------
// File system object.
SdFat sd;

// Log file.
SdFile file;

// Time in micros for next data record.
uint32_t logTime;

//==============================================================================
// User functions.  Edit writeHeader() and logData() for your requirements.

const uint8_t ANALOG_COUNT = 4;
//------------------------------------------------------------------------------
// Write data header.
void writeHeader() {
  file.print(F("micros"));
  for (uint8_t i = 0; i < ANALOG_COUNT; i++) {
    file.print(F(",adc"));
    file.print(i, DEC);
  }
  file.println();
}
//------------------------------------------------------------------------------
// Log a data record.
void logData() {
  uint16_t data[ANALOG_COUNT];

  // Read all channels to avoid SD write latency between readings.
  for (uint8_t i = 0; i < ANALOG_COUNT; i++) {
    data[i] = analogRead(i);
  }
  // Write data to file.  Start with log time in micros.
  //file.print(toString(datapacket));
  file.print((uint8_t *)&datapacket, sizeof(datapacket));

  // Write ADC data to CSV record.
  for (uint8_t i = 0; i < ANALOG_COUNT; i++) {
    file.write(',');
    file.print(data[i]);
  }
  file.println();
}
//==============================================================================
// Error messages stored in flash.
#define error(msg) sd.errorHalt(F(msg))
//------------------------------------------------------------------------------
void setup() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  pinMode(RFM95_INT, INPUT);

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
  //------------------------------------------------------------------------------------

  const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  char fileName[13] = FILE_BASE_NAME "00.csv";
  
  // Initialize at the highest speed supported by the board that is
  // not over 50 MHz. Try a lower speed if SPI errors occur.
  if (!sd.begin(chipSelect, SD_SCK_MHZ(10))) {
    sd.initErrorHalt();
  }

  // Find an unused file name.
  if (BASE_NAME_SIZE > 6) {
    error("FILE_BASE_NAME too long");
  }
  while (sd.exists(fileName)) {
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
  }
  
  Serial.print(F("Logging to: "));
  Serial.println(fileName);

  // Write data header.
  writeHeader(); 

  // Start on a multiple of the sample interval.
  logTime = micros() / (1000UL * SAMPLE_INTERVAL_MS) + 1;
  logTime *= 1000UL * SAMPLE_INTERVAL_MS;
  
}

//------------------------------------------------------------------------------
int16_t packetnum = 0;  // packet counter, we increment per xmission
void loop() {
  Serial.println("Sending to rf95_server");
  // Send a message to rf95_server

  Serial.print("Sending "); Serial.println(toString(datapacket));

  Serial.println("Sending..."); delay(10);
  rf95.send((uint8_t *)&datapacket, sizeof(datapacket));

  Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();
  //------------------------------------------------------------------

    logData();

    // Force data to SD and update the directory entry to avoid data loss.
    if (!file.sync() || file.getWriteError()) {
    error("write error");
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
