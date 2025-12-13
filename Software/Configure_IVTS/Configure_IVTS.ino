// Script to Configure IVT-S mode, rate, CANID, cycle time, etc.
#include <SPI.h>
#include <mcp2515.h>

// SPI Setup
#define CANSPI FSPI
SPIClass *canspi = NULL;

// CAN Library Setup
struct can_frame txMsg;
struct can_frame rxMsg;
MCP2515 mcp2515(5);

bool valid;
bool once = 0;

bool commandSTOP() {
  // Send STOP Command
  txMsg.can_id  = 0x411;
  txMsg.can_dlc = 8;
  txMsg.data[0] = 0x34;
  txMsg.data[1] = 0x00;
  txMsg.data[2] = 0x01;
  txMsg.data[3] = 0x00;
  txMsg.data[4] = 0x00;
  txMsg.data[5] = 0x00;
  txMsg.data[6] = 0x00;
  txMsg.data[7] = 0x00;

  delay(300);

  mcp2515.sendMessage(&txMsg);

  // Verify STOP Response
  unsigned long t0 = millis();
  while (millis() - t0 < 5000) {
    if (mcp2515.readMessage(&rxMsg) == MCP2515::ERROR_OK) {
      if (rxMsg.can_id == 0x511) {
        if (rxMsg.data[0] == 0xB4 && rxMsg.data[1] == 0x00 && rxMsg.data[2] == 0x01) {
          Serial.println("IVT-S Placed in STOP Mode");
          return true;
        } 
      }
    }
  }
  Serial.println("Bus Timeout in STOP");
  return false;
}

bool setConfigResult(unsigned int channel, unsigned int trigger, unsigned int outputcycle) {
  // Parse Config Result (D0, 1, 2, 3)
  unsigned int DB0, DB1, DB2, DB3;
  DB0 = 0x20 | (channel & 0x0F);
  DB1 = trigger;
  DB2 = (outputcycle >> 8);
  DB3 = 0x00FF & outputcycle;

  // Structure tx can message
  txMsg.can_id  = 0x411;
  txMsg.can_dlc = 8;
  txMsg.data[0] = DB0;
  txMsg.data[1] = DB1;
  txMsg.data[2] = DB2;
  txMsg.data[3] = DB3;
  txMsg.data[4] = 0x00;
  txMsg.data[5] = 0x00;
  txMsg.data[6] = 0x00;
  txMsg.data[7] = 0x00;
  delay(300);

  mcp2515.sendMessage(&txMsg);

  // Verify Result
  unsigned long DB0Test = 0x99;
  unsigned long DB1Test = DB0Test;
  unsigned long DB2Test = DB0Test;
  unsigned long DB3Test = DB0Test;
  unsigned long t0 = millis();
  while (millis() - t0 < 5000) {
    if (mcp2515.readMessage(&rxMsg) == MCP2515::ERROR_OK) {
      if (rxMsg.can_id == 0x511) {
        if (((rxMsg.data[0] >> 4) == 0xA) && ((rxMsg.data[0] & 0x0F) == channel)) {
          Serial.println("IVT-S Channel Configured");
          if (rxMsg.data[1] == DB1) {
            Serial.println("IVT-S Trigger Configured");
            if (rxMsg.data[2] == DB2 && rxMsg.data[3] == DB3) {
              Serial.println("IVT-S Cycle Configured");
              return true;
            }
          }
        }
        DB0Test = rxMsg.data[0];
        DB1Test = rxMsg.data[1];
        DB2Test = rxMsg.data[2];
        DB3Test = rxMsg.data[3];
      }
    }
  }
  Serial.println("Bus Timeout in setConfigResult");
  Serial.println(DB0Test, HEX);
  Serial.println(DB1Test, HEX);
  Serial.println(DB2Test, HEX);
  Serial.println(DB3Test, HEX);
  return false;
}

bool setCANID(unsigned int MsgType, unsigned int CANID) {
  // Parse Config Result (D0, 1, 2, 3)
  unsigned int DB0, DB1, DB2, DB3, DB4, DB5, DB6, SerialNum;
  DB0 = MsgType;
  DB1 = (CANID >> 8);
  DB2 = 0x00FF & CANID;
  DB3 = 0x00; // Setting Serial Number
  DB4 = 0xD6;
  DB5 = 0x53;
  DB6 = 0xFC;
  SerialNum = 0xD653FC;

  // Structure tx can message
  txMsg.can_id  = 0x411;
  txMsg.can_dlc = 8;
  txMsg.data[0] = DB0;
  txMsg.data[1] = DB1;
  txMsg.data[2] = DB2;
  txMsg.data[3] = DB3;
  txMsg.data[4] = DB4;
  txMsg.data[5] = DB5;
  txMsg.data[6] = DB6;
  txMsg.data[7] = 0x00;
  delay(300);

  mcp2515.sendMessage(&txMsg);

  // Verify Result
  unsigned int channelcheck1 = 0x99;
  unsigned int channelcheck2 = 0x99;
  unsigned long t0 = millis();
  while (millis() - t0 < 5000) {
    if (mcp2515.readMessage(&rxMsg) == MCP2515::ERROR_OK) {
      if (rxMsg.can_id == 0x511) {
        if ((rxMsg.data[0] & 0x0F) == (DB0 & 0x0F) && ((rxMsg.data[0] & 0xF0) == 0x90)) {
          Serial.println("IVT-S Message Type Configured");
          if (rxMsg.data[1] == DB1 && rxMsg.data[2] == DB2) {
            Serial.println("IVT-S CANID Configured");
            if ((rxMsg.data[4] & 0xF00) | (rxMsg.data[5] & 0x0F0) | (rxMsg.data[6] & 0x00F) == SerialNum) {
              Serial.println("IVT-S Serial Num Confirmed");
              return true;
            }
          }
          channelcheck1 = rxMsg.data[1];
          channelcheck2 = rxMsg.data[2];
        }
      }
    }
  }
  Serial.println("Bus Timeout in setCANID: ");
  Serial.println(channelcheck1, HEX);
  Serial.println(channelcheck2, HEX);
  return false;
}

bool setBAUD(unsigned int newRate) {
  // Structure tx can message
  txMsg.can_id  = 0x411;
  txMsg.can_dlc = 8;
  txMsg.data[0] = 0x3A;
  txMsg.data[1] = newRate;
  txMsg.data[2] = 0x00;
  txMsg.data[3] = 0x00;
  txMsg.data[4] = 0x00;
  txMsg.data[5] = 0x00;
  txMsg.data[6] = 0x00;
  txMsg.data[7] = 0x00;
  delay(300);

  mcp2515.sendMessage(&txMsg);

  delay(3000); // Let IVTS Reboot
  Serial.println("IVT-S Baud Rate Changed");
  return true;

  /*// Verify Result
  unsigned long t0 = millis();
  unsigned long real = 0xFF;
  while (millis() - t0 < 5000) {
    if (mcp2515.readMessage(&rxMsg) == MCP2515::ERROR_OK) {
      if (rxMsg.can_id == 0x511) {
        if (rxMsg.data[0] == 0xB4) {
          Serial.println("IVT-S Baud Rate Changed");
          return true;
        }
      }
      real = rxMsg.data[0];
    }
  }
  Serial.print("Bus Timeout in setBAUD: ");
  Serial.println(real);
  return false;
  */
}

bool commandSTORE() {
  // Send STORE Command
  txMsg.can_id  = 0x411;
  txMsg.can_dlc = 8;
  txMsg.data[0] = 0x32;
  txMsg.data[1] = 0x00;
  txMsg.data[2] = 0x00;
  txMsg.data[3] = 0x00;
  txMsg.data[4] = 0x00;
  txMsg.data[5] = 0x00;
  txMsg.data[6] = 0x00;
  txMsg.data[7] = 0x00;

  delay(300);

  mcp2515.sendMessage(&txMsg);

  // Verify STORE Response
  unsigned long t0 = millis();
  while (millis() - t0 < 5000) {
    if (mcp2515.readMessage(&rxMsg) == MCP2515::ERROR_OK) {
      if (rxMsg.can_id == 0x511) {
        if (rxMsg.data[0] == 0xB2) {
          Serial.println("IVT-S STORE Successful");
          return true;
        }
      }
    }
  }
  Serial.println("Bus Timeout in STORE");
  return false;
}

bool commandSTART() {
  // Send START Command
  txMsg.can_id  = 0x411;
  txMsg.can_dlc = 8;
  txMsg.data[0] = 0x34;
  txMsg.data[1] = 0x01;
  txMsg.data[2] = 0x01;
  txMsg.data[3] = 0x00;
  txMsg.data[4] = 0x00;
  txMsg.data[5] = 0x00;
  txMsg.data[6] = 0x00;
  txMsg.data[7] = 0x00;

  delay(300);

  mcp2515.sendMessage(&txMsg);

  // Verify START Response
  unsigned long t0 = millis();
  while (millis() - t0 < 5000) {
    if (mcp2515.readMessage(&rxMsg) == MCP2515::ERROR_OK) {
      if (rxMsg.can_id == 0x511) {
        if (rxMsg.data[0] == 0xB4 && rxMsg.data[1] == 0x01 && rxMsg.data[2] == 0x01) {
          Serial.println("IVT-S START Successful");
          return true;
        }
      }
    }
  }
  Serial.println("Bus Timeout in START");
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  canspi = new SPIClass(CANSPI);
  canspi->begin(6, 16, 15, 5); // CLK, MISO, MOSI, CS
  pinMode(canspi->pinSS(), OUTPUT);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_250KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
}

void loop() {
  /*
  // Change Baud Rate
  Serial.println("------ Configure Baud Rate ------");
  valid = commandSTOP();
  if (valid) { valid = setBAUD(0x08); } // 0x08, 0x04, 0x02 (250k, 500k, 1000k, respectively)
  */

  
  // Change Result Configuration
  Serial.println("------ Configure Result Output ------");
  valid = commandSTOP();
  if (valid) { valid = setConfigResult(0x01, 0x02, 0x003C); } // 02 (high nibble, low nibble) message, trigger mode (always 0x0n), cycle time in ms see datasheet p21 for assignments, 0x003C is 60ms
  if (valid) { valid = commandSTORE(); }
  if (valid) { valid = commandSTART(); }
  

  /*
  // Change CANID
  Serial.println("------ Configure CANID ------");
  valid = commandSTOP();
  if (valid) { valid = setCANID(0x11, 0x0777); } // see datasheet p23 for assignments
  if (valid) { valid = commandSTORE(); }
  if (valid) { valid = commandSTART(); }
  */

  if (valid) { Serial.println("Finish Successful"); }
  else { Serial.println("Finish Unsuccessful"); }
  while (1) {
    delay(10000);
  }
}