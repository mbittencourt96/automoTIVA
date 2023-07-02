# AutomoTIVA
Welcome to the "AutomoTIVA" repository! Here are all the files you will need to get started with this project!
# What it is
"AutomoTIVA" is a project developed as a final project for the Electronics Engineering course in the Federal Technological University of Parana (UTFPR) - Brazil.
It is a shield for the TIVA EK-TM4C1294XL Evaluation Board, that is capable of reading CAN bus and also has GPS, RTC and Wi-fi capabilities.
A shield for the Arduino Uno was also developed to simulate a vehicle sending OBD messages in the CAN Bus.
# Why it is useful
The boards can be used in the university, to teach students how the CAN and OBD protocol works (and also the TIVA board and peripherals) in a pratical and easy way. 
But of course, with some adjustments, you can even use this shield in real vehicles! 
# Testing in real vehicles
For testing with real vehicles, I recommend to build a 12V-5V regulator (to connect in the 12V of the OBD) and to also connect the CANH and CANL lines in the shield to the OBD connector. The board was not tested in a real vehicle yet, so be careful, especially when building the regulator, to protect the board from damage. <br>
  ### CAN ID
  Another thing about testing in real vehicles, some vehicles may have an 29-bit CAN ID, which is not compatible with the code I built for the TIVA. So, if the vehicle you are testing is 29-bit, you will have to modify the code to handle this situation. I suggest that you build some code to identify the CAN ID length and then with this information you can do a "IF" or "SWITCH" statement to choose how to handle CAN messages. <br>
  ### OBD messages
  About the OBD messages, I implemented the most common ones (Engine RPM, Vehicle Speed, Engine Coolant Temperature...), but you can implement any messages you want. One suggestion would be to send a 00h message in the bus (To know the supported PIDs of the vehicle) and then implement only the supported PIDs
  
# How to get started
  ### IDE
  For the TIVA shield code, you can get started by downloading an IDE for ARM development. I used IAR workbench and the files that are in the repository are from this IDE. But if you get all the source files and compile them, I believe you can use any ARM IDE.<br>
  For the Arduino shield code, you can use Arduino IDE. I used the 1.8.19 version, but it also worked in 2.1.0.
 ### List of materials
 For the TIVA shield, you will need:<br>
 ->1 TIVA EK-TM4C1294XL Evaluation Board + Usb cable<br>
 ->1 NEO 6M GPS Module<br>
 ->1 ESP8266 NodeMCU<br>
 ->1 DS3231 RTC Module<br>
 ->1 RGB LED Module<br>
 ->1 TJA1050 CAN Transceiver<br>
 ->Set of jumpers to connect all these things together<br>
 For the Arduino shield, you will need:<br>
->1 Arduino Uno + Usb cable<br>
->1 MCP2515 CAN Module<br>
->6 10k ohms potenciometers<br>
### Connections
Soon I will provide the connection diagrams!
### Final steps
 You must, after connecting all the parts together, connect the boards to the USB ports in your computer and load the programs in the respective boards. And then you are ready to go!

 # How to get help
 You can reach me directly in my e-mail mbittencourtj@outlook.com for any questions regarding this project.
 




