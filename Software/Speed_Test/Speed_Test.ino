#include <SPI.h>
#include <mcp2515.h>

// SPI Setup
#define CANSPI FSPI
SPIClass *canspi = NULL;

// CAN Library Setup
struct can_frame canMsg;
MCP2515 mcp2515(5);

// Speed Diagnostics
int cntr = 0;
unsigned long oldTime = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  canspi = new SPIClass(CANSPI);
  canspi->begin(6, 16, 15, 5); // CLK, MISO, MOSI, CS
  pinMode(canspi->pinSS(), OUTPUT);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Serial.println("------- CAN Speedtest ----------");
}

void loop() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    cntr++;
  }
 
  if ((millis()-oldTime)>1000) {
    oldTime = millis();
    Serial.print(cntr);
    Serial.println(" msg/sec");
    cntr = 0;      
  }
}
