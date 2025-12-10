#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsg;
MCP2515 mcp2515(16);

void setup() {
  Serial.begin(115200);
  //while (!Serial);
  delay(500);
  Serial.print("YEAAA");

  mcp2515.reset();
  mcp2515.setBitrate(CAN_1000KBPS, MCP_8MHZ);
  mcp2515.setNormalMode(); 

}

void loop() {
  Serial.println("Communicating");
  delay(1000);
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
