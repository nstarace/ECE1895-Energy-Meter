# Overview
Contained in this repository are all files relevant to a modular data acquisition and logging system for automotive applications. The system measures and records voltage and current for high voltage EV tractive systems. This design can be considered as an energy meter since all recorded data can be used to calculate power consumption of the overall battery.

The initial scope of this project details a minimal hardware architecture that utilized the Twisted Wire Automotive Interface (TWAI) available on the ESP32, eliminating the need for an external CAN bus transceiver. Due to a lack of open source documentation, a popular external controller was used in place. Additionally, the initial scope of the project included the measurement and logging of temperature sensors. This feature was not included in the final design due to the limited time spent on the project. All components used in the final design are detailed below. 

| Device Type | Component | Protocol |
| ----------- | --------- | -------- |
| Sensor | Isabellenhutte IVT-S | CAN |
| Transceiver | TJA1050 | CAN/UART |
| Controller | MCP2515 | UART/SPI |
| Microcontroller | ESP32-S3 | All |
| Storage | SD Breakout | SPI |

### Previous Work
The central components used here to process CAN signals build on a CAN controller board developed by Arduino. This CAN controller board uses the same MCP2515 CAN controller to convert analog signals to CAN. Used in the Arduino CAN Board is the Atmega328p, an 8-bit microcontroller that would clip the 32-bit messages sent by the IVT-S sensor. Therefore, the Arduino CAN Board was used as inspiration and expanded to the 32-bit ESP32. 

# Preliminary Verification
Before integrating the full logging system, each component and its functionality was prototyped individually as described below.

### IVT-S Sensor Setup
The IVT-S current and voltage sensor had to be configured over CAN before use by sending and receiving a command and response sequence. Typically performed in software such as BUSMASTER or PCAN View, these unfriendly programs require expensive hardware to "sniff" the CAN bus. Therefore, custom configure scripts were written in Arduino to send and receive the configure messages using the MCP2515. The following sensor parameters had to be adjusted for sensor use:
* Bus BAUD Rate (250kbps, 500kbps, 1000kbps)
* CANID (Current Measurement, Voltage Measurement 1, Voltage Measurement 2, Voltage Measurement 3)
* Result Configuration (Trigger Collection, Cyclic Collection, Period)

Each parameter adjustment requires a command and response sequence of placing the sensor in stop mode, configuring the selected parameter, storing the result, and placing the sensor back in run mode. An example command and response sequence for setting a CANID is shown below from the manufacturer:

<img width="700" alt="image" src="https://github.com/user-attachments/assets/eb07321b-e208-44fc-ba2e-022df567eb9c" />

The accompanying custom Arduino functions to set the CANID for this example are shown below. Each function sends a CAN message with its respective parameters and verifies the response from the sensor. Using these custom scripts, the time required to configure the sensor is drastically reduced compared to using the BUSMATER/PCAN setup. 

<img width="600" alt="image" src="https://github.com/user-attachments/assets/ffb9a721-89c1-494a-8c24-c117d80b10e7" />

The custom IVT-S configuration script can be found under Software -> Configure_IVTS

### MCP2515 Setup
Two example scripts were used from the Arduino MCP2515 CAN interface library by autowp to setup the MCP2515 CAN controller and TJA1050 CAN transceiver. These sample scripts enabled the sending and receiving of messages printed to the serial monitor. Hardware setup of the MCP2515 required connecting an SPI bus to the ESP32 and CAN bus to the IVTS. To reduce signal reflection, a CAN bus requires two 120 Ohm resistors to be placed at the ends of the bus. Therefore, these resistors were added as required. An example of a basic read CAN message script is described at a high level along with a sample output from testing (in HEX).
* Import SPI and mcp2515 libraries
* Assign custom SPI pins to ESP32
* Configure CAN bus rate
* Check if CAN message is being transmitted and output the message: CANID (11 bits), Data Length Frame (4 bits), and data (64 bits)
<img width="307" alt="output of basic read" src="https://github.com/user-attachments/assets/42bf36e2-bad5-4ccd-a945-b113dd67e2ee" />

The MCP2515 setup scripts can be found under Software -> CAN_Read, Speed_Test

### SD Card Setup
The 5V ready Micro-SD Breakout board+ from Adafruit was used to interface the ESP32 with a micro SD card over traditional SPI. The SD card was preformatted to a FAT32 standard and sample scripts were used from the arduino-esp32 SD library. These example scripts provided functionality for creating, deleting, and appending files and their directories. The SPI bus was configured on custom pins similar to the MCP2515. Finally, the sample script was run to verify communication and functionality of the SD card. The output of this setup script is shown below:

<img width="320" alt="image" src="https://github.com/user-attachments/assets/06ec8e6e-3a97-4eef-ab42-61394e9904ea" />

The SD Card setup script can be found under Software -> SDCard_Logging

# Implementation
Each individual setup was integrated into a full data measurement and logging pipeline. 

### Hardware Architecture
Beginning at the measurement stage, the IVTS sensor busbars are wired in series with the measured load. Voltage channels of interest are connected to the opposite side of the load the sensor is in series with. For example, if the sensor is on the high side of the load, the voltage measurement is connected to the low side. The sensor is powered with 12 V while twisted CAN wire (to reduce noise) is connected to the TJA1050 transceiver. The transceiver transmits the data over UART to the MCP2515 which sends the data over SPI to the ESP32. Finally, the ESP32 logs the data to the micro SD card over a second SPI bus. 

Two separate SPI busses were used because of the skewed latency between operations. Since the file on the SD card must be opened and closed every time data is written, the time required to write the data files is significantly longer than the transmit time of the sensor. Therefore, if both components were on the same SPI bus, messages from the IVTS would have been missed. Additional details on the software architecture to handle the timing differences are described in the next section. An image of the final prototype is shown below. 

<img width="1122" alt="image" src="https://github.com/user-attachments/assets/5d9cf487-d721-4201-ab97-cf2add0b5351" />

### Software Architecture
To avoid the latency difference between writing to the SD card and collecting all data from the IVTS, a non-blocking software architecture was developed to ensure that no incoming messages were missed from the IVTS. The software stays in a state of polling the CAN bus, collecting all incoming messages and decoding only the data measurements of interest (voltage, current) that are stored in a buffer. Separately, the data is appended to a .txt file on the SD card at a constant rate slower than the transmit rate of the sensor. A high level description is summarized below:
* Declare main and peripheral SPI bus instances
* Configure SPI busses for SD card and MCP2515
* Configure CAN bus rate
* Initialize SD Card
* Poll CAN Bus
  * Decode incoming messages at 60 ms (11 bit ID, 32 bit data in frames 2 - 5)
  * Store in buffer
* Log to SD Card (100 ms)
  * Empty Buffer

The full measurement and logging script can be found under Software -> IVTS_Logging

### Decoding a CAN Message
A standard CAN frame has eight message fields containing message data and diagnostics. To effectively collect the data incoming from the IVTS, three fields had to be processed and were made available by the CAN frame structure defined by the MCP2515 library. The first field of a message is the 11 bit CANID describing the message type (current, voltage, etc.) and configured by the Configure_IVTS custom script. The second important field is the data length width (DLC) which defines the length of the data contents. Finally, the third field is the 64 bit data field. Specific to the IVT-S firmware, exact measurement data is stored in bytes two through five as big endian. Therefore, the IVTS_Logging script parses the four bytes to form the full measurement.  

# Testing
To test the logging system, a simple load was created using lab resistors in parallel to draw a current of 120 mA at 5 V. This load was connected to the logging systems current measurement by placing the sensor in series. The voltage measurement was placed on the 12 V rail. The logging system effectively measured and logged voltage and current to the micro SD card. Since the sensor is designed for high voltage, current and voltage measurements have a resolution of 47 mA and 50 mV, respectively. The limited time spent on the project did not justify the risk of testing with high voltage. Nonetheless, full functionality was verified against the current and voltage listed on the power supply. 

The biggest challenge of verifying the functionality of the system was properly decoding the CAN messages. Initially, the four bytes of data were parsed using a simple bitwise or statement and stored in a 32 bit integer. However, since each field of the can frame was only 8 bits, parsing the messages collapsed each operator and always yielded 8 bits. Therefore, the variables had to be typecasted to 32 bits before operating on them. An additional problem was that the sensor does not send valid data unless both the current and at least one voltage measurement are connected. Therefore, initially testing the voltage and current measurements in isolation yielded garbage values. 

The output of this system under the listed conditions are shown below along with the compared ratings on the power supply for the first five seconds of collection. Time is measured in ms, current in mA, and voltage in mV. In this output, voltage channels 2 and 3 are not connected and therefore log garbage values that should be disregarded. 

<img width="540" alt="image" src="https://github.com/user-attachments/assets/1e834f3f-c1c2-497d-8f09-eca34bae2fa9" />

<img width="540" alt="image" src="https://github.com/user-attachments/assets/90031cc0-3327-4e49-83be-3cd3468f88b3" />


# Conclusion
This repository presents the design and implementation of a modular data acquisition and logging system intended for automotive and electric vehicle applications. The system functions as an energy meter platform by measuring and recording current and voltage data from a high-voltage tractive system. The designed architecture enables downstream calculation of power and energy consumption. An ESP32-S3 microcontroller serves as the central processing unit and interfaces with an Isabellenhutte IVT-S current and voltage sensor over CAN using an MCP2515 controller and TJA1050 transceiver. Data is stored locally on a micro SD card via SPI.

The project began with a minimal hardware scope that aimed to leverage the ESP32’s native TWAI interface. However, limited open-source documentation motivated the use of a proven external CAN controller architecture. Each subsystem sensor configuration, CAN communication, and SD card logging was first validated independently before being integrated into a complete pipeline. Custom Arduino scripts were developed to configure the IVT-S sensor over CAN, significantly reducing setup time compared to traditional PC based CAN tools.

A key aspect of the final implementation is the non-blocking software architecture designed to handle mismatched latencies between high rate CAN message reception and slower SD card write operations. By buffering decoded measurements and decoupling acquisition from logging, the system reliably captures all incoming data without message loss. End-to-end testing with a low   power resistive load confirmed correct measurement, decoding, and logging of voltage and current data, validating the overall functionality of the system.

### Future Work
EV motorsport endurance events often pose challenges in the form of aggressive battery depletion. Given the precise nature of the IVT-S sensor and its measurements along with the modular capabilities of this logging system, a Kalman filter or other state estimation technique could be implemented to find the state of battery charge. Additional improvements might extend the logging system to a mobile app, where a user can view a stream of measured data and state estimation in real time over Bluetooth.  

