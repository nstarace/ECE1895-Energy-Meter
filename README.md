# Tractive System Energy Meter
Project repository for data aquisition system to measure and log high voltage, current, and temperature to an SD card. 

## Product Setup
See Other -> Reports -> ECE1895 Energy Meter Final Report for details on how to use the hardware and software within the project

## File Descriptions
All files relevant to the current state of the product are described below
  
Hardware
* BOMs: All bill of materials files used throughout development
* DBC Files: Files used to decode CAN messages outlining the structure and format of messages
* Datasheets: Reports of all hardware components used or considered in the design

Software
* CAN_Read: Basic script to read CAN messages transmitted on the bus
* SDCard_Logging: Basic script to write, delete, or append folders and files to an SD card. Provides hollistic details
* IVTS_Logging: Integration of CAN_Read and SDCard_Logging to log data from current sensor to an SD card
* Speed_Test: Assesses the speed that CAN messages are being transmitted on the bus. 

Mechanical
* Plug Layouts: Low resolution prototype for PCB and IVT-S integration with energy meter mating plug receptacle
* IVTS Pinnout: Assigned wire pins, wire color, and AWG of IVT-S current sensor
* Other files include FSAE related guidelines

Other
* Reports: Proposals and reports related to ECE1895
* Resources: Personal notes, initial planning documentation, and 1Wire documentation for reference
* Results: Important outputs of the design shown through screenshots

## Future Work
Given the precise nature of the IVT-S sensor and its measurements, a Kalman filter or other state estimation technique could be implemented to find the state of battery charge. 

