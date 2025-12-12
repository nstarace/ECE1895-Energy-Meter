#include <SPI.h>
#include <mcp2515.h>

// SPI Setup
#define CANSPI FSPI
SPIClass *canspi = NULL;

// CAN Library Setup
struct can_frame canMsg;
MCP2515 mcp2515(5);

void setup() {
  Serial.begin(115200);
  delay(500);

  canspi = new SPIClass(CANSPI);
  canspi->begin(6, 16, 15, 5); // CLK, MISO, MOSI, CS
  pinMode(canspi->pinSS(), OUTPUT);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Serial.println("------- CAN Read ----------");
  Serial.println("ID  DLC   DATA"); 
}

void loop() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.print(canMsg.can_id, HEX); // print ID (11 bits)
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
