/*
 * RX_Struct
*/

#include <SPI.h>
#include <SD.h>
#include <RH_RF95.h>

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Blinky on receipt
#define LED 13

struct data {
  double alt;
  double vertical_velocity;
  double horizontal_velocity;
  double vertical_acceleration;
  double horizontal_acceleration;
};

data datapacket = {0.0, 0.0, 0.0, 0.0, 0.0};

File dataFile = SD.open("datalog.txt", FILE_WRITE);

void setup()
{

  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  while (!Serial);
  Serial.begin(9600);
  delay(100);

  Serial.println("Arduino LoRa RX Test!");

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
  rf95.setTxPower(15, false);
}

int16_t packetnum = 0;  // packet counter, we increment per xmission
void loop()
{
  Serial.println("Waiting to receive");

  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(datapacket);

  if (rf95.waitAvailableTimeout(1000))
  {
    // Should be a reply message for us now
    if (rf95.recv((uint8_t*)&datapacket, &len))
    {
      Serial.print("Got reply: ");
      Serial.println(toString(datapacket));
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);

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
    else
    {
      Serial.println("Receive failed");
    }
  }
  else
  {
    Serial.println("No reply, is there a listener around?");
  }
  delay(1000);
}

String toString(data datapacket) {
  String dataString = "Altitude: " + String(datapacket.alt)
                      + ".  Vertical Velocity: " + String(datapacket.vertical_velocity)
                      + ".  Horizontal Velocity: " + String(datapacket.horizontal_velocity)
                      + ".  Vertical Acceleration: " + String(datapacket.vertical_acceleration)
                      + ".  Horizontal Acceleration: " + String(datapacket.horizontal_acceleration);
  return dataString;
}
