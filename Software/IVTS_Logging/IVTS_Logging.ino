#include <SPI.h>
#include <mcp2515.h>
#include <FS.h>
#include <SD.h>

// SPI Declaration & Pinout
#define CANSPI FSPI
#define SDSPI HSPI
SPIClass *spisd = NULL;
SPIClass *spican = NULL;

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

// Measurment Frame
struct CANSignals {
  uint32_t current;        // IVTS transmits 4 bytes, 
  uint32_t v1;
  uint32_t v2;
  uint32_t v3;
};

// Logging Details
const uint32_t loggingPeriod = 100; // Should be slower than measurement (ms)
uint32_t lastTime = 0;
uint32_t currentTime = 0;
uint32_t MaxTime = 10000;
CANSignals msg; 

void pollCAN() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    decodeFrame(canMsg);
  } 
}

void decodeFrame(const can_frame &canMsg) {
  switch (canMsg.can_id) {
    case 0x501: // Current 
      msg.current = ((uint32_t)canMsg.data[2] << 24) | ((uint32_t)canMsg.data[3] << 16) | ((uint32_t)canMsg.data[4] << 8) | ((uint32_t)canMsg.data[5]);
      //Serial.println(msg.current, HEX);
      break;
    
    case 0x502: // V1 Message
      msg.v1 = ((uint32_t)canMsg.data[2] << 24) | ((uint32_t)canMsg.data[3] << 16) | ((uint32_t)canMsg.data[4] << 8) | ((uint32_t)canMsg.data[5]);
      break;

    case 0x503: // V2 Message
      msg.v2 = ((uint32_t)canMsg.data[2] << 24) | ((uint32_t)canMsg.data[3] << 16) | ((uint32_t)canMsg.data[4] << 8) | ((uint32_t)canMsg.data[5]);
      //Serial.println(msg.current, HEX);
      break;
    
    case 0x504: // V3 Message
      msg.v3 = ((uint32_t)canMsg.data[2] << 24) | ((uint32_t)canMsg.data[3] << 16) | ((uint32_t)canMsg.data[4] << 8) | ((uint32_t)canMsg.data[5]);
      break;
  }
}

void logRow(uint32_t t) {
  Serial.print(t);
  Serial.print(" ");
  Serial.print(msg.current);
  Serial.print(" ");
  Serial.print(msg.v1);
  Serial.print(" ");
  Serial.print(msg.v2);
  Serial.print(" ");
  Serial.print(msg.v3);
  Serial.print("\n");
  // Log Time and Data
  File file = SD.open("/data/data.txt", FILE_APPEND);
  if (!file) return;

  file.print(t);
  file.print('\t');
  file.print(msg.current);
  file.print('\t');
  file.print(msg.v1);
  file.print('\t');
  file.print(msg.v2);
  file.print('\t');
  file.print(msg.v3);
  file.println();

  file.close();
}

void createDir(fs::FS &fs, const char *path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(3000);

  // Initialize 1 SPI Bus with Two Devices
  spisd = new SPIClass(SDSPI); // FSPI
  spican = new SPIClass(CANSPI); // SPI

  spisd->begin(sckSD, misoSD, mosiSD, csSD);
  spican->begin(sckCAN, misoCAN, mosiCAN, csCAN);

  pinMode(spican->pinSS(), OUTPUT);
  pinMode(spisd->pinSS(), OUTPUT);

  // Setup MCP
  mcp2515.reset();
  mcp2515.setBitrate(CAN_250KBPS, MCP_8MHZ); // IVTS operates on 500kBps
  mcp2515.setNormalMode();

  // Setup SD Card
  if (!SD.begin(csSD, *spisd)) {
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
  createDir(SD, "/data");              
  writeFile(SD, "/data.txt", "Time\tCurrent\tVoltage 1\tVoltage 2\tVoltage 3\n");
  Serial.println("Initial Setup Successful");
  Serial.println("Time Current Voltage 1 Voltage 2 Voltage 3");
}

// Non-Blocking Logging System
// Last known value fields  
void loop() {
  // Drains Entire Buffer
  pollCAN();

  // Logging outside of decoding to ensure timer execution 
  currentTime = millis();
  if (currentTime - lastTime >= loggingPeriod) {
    lastTime = currentTime;
    logRow(currentTime);
  }
}
