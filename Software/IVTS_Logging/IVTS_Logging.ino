#include <SPI.h>
#include <mcp2515.h>
#include <FS.h>
#include <SD.h>

// SPI Declaration & Pinout
#define CANSPI HSPI
#define SDSPI FSPI
SPIClass *spican = NULL;
SPIClass *spisd = NULL;

int sckSD = 8;
int misoSD = 3;
int mosiSD = 46;
int csSD = 9;
int sckCAN = 6;
int misoCAN = 16;
int mosiCAN = 15;
int csCAN = 5;

// MCP Setup
struct can_frame canMsg;
MCP2515 mcp2515(csCAN);

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(3000);

  // Initialize 2 SPI Busses
  spican = new SPIClass(CANSPI);
  spisd = new SPIClass(SDSPI);
  spican->begin(sckCAN, misoCAN, mosiCAN, csCAN);
  spisd->begin(sckSD, misoSD, mosiSD, csSD);
  pinMode(spican->pinSS(), OUTPUT);
  pinMode(spisd->pinSS(), OUTPUT);

  // Setup MCP
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); // IVTS operates on 500kBps
  mcp2515.setNormalMode();

  // Setup SD Card
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD Card Attached)");
    return;
  }
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);

  // Create SD Directory and File for Logging
  createDir(SD, "/data");               // CONFIGURE
  writeFile(SD, "/data.txt", "Time\ID\tDLC\tDATA\n");
}

void loop() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    //Serial.print(canMsg.can_id, HEX); // print ID (11 bits)
    appendFile(SD, "/data.txt", canMsg.can_id);
    Serial.print(" "); 
    Serial.print(canMsg.can_dlc, HEX); // print DLC (4 bits length of data)
    Serial.print(" ");
    
    for (int i = 0; i<canMsg.can_dlc; i++)  {  // print the data
      Serial.print(canMsg.data[i],HEX);
      Serial.print(" ");
    }

    Serial.println();      
  }
}
