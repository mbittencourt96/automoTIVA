#include <mcp_can.h>
#include <SPI.h>
#include "pids.h"

//Using Cory J Fowler's fork of the Seeed Studio CAN BUS Shield library:
//https://github.com/coryjfowler/MCP_CAN_lib


#define CAN1_INT 2                                     // Set INT to pin 2

#define SPEED_PIN A1
#define RPM_PIN A0
#define ETHANOL_PIN A4
#define FUEL_PIN A5
#define THROTTLE_PIN A2
#define TEMP_PIN A4

MCP_CAN CAN1(9);                                      // Set CS to pin 9

// This will store the ID of the incoming can message
long unsigned int canId = 0x000;
// this is the length of the incoming message
unsigned char len = 0;
// This the eight byte buffer of the incoming message data payload
unsigned char buf[8];
unsigned char canMessageRead[8];
int startedFlag = 0;

unsigned int ect = 0;
unsigned int rpm = 0;
unsigned int fuel_level = 0;
unsigned int eth_percentage = 0;
unsigned int throttle_pos = 0;
unsigned int veh_speed = 0;
unsigned int odometer = 0;
unsigned int pid = 0;

unsigned long lastTime = 0;
unsigned long currentTime = 0;

byte uintMSB(unsigned int value)
{
  return (byte)((value & 0xFF00) >> 8);
}

byte uintLSB(unsigned int value)
{
  return (byte)(value & 0x00FF);
}

int convertSensorValue(int pid, int sensorValue)
{
  Serial.println(sensorValue);
  
  switch(pid)
  {
    case 5:  //Engine coolant temperature
    {
      //MAX VALUE = 215ºC
      //MIN VALUE = -40ºC
      //EQUATION => Temp = 0.25*sensorValue - 40

      return (int) (0.25*sensorValue);
      break;
    }   
    case 12:  //Engine RPM
    {
      //MAX VALUE = 16383.75 rpm
      //MIN VALUE = 0 rpm
      //EQUATION => Engine_rpm = 16*sensorValue

        return (int) (16*sensorValue)*4;
        break;
    }
    case 17:   //Throttle position
    {
      //MAX VALUE = 100%
      //MIN VALUE = 0%
      //EQUATION => Throttle_position = 0.097*sensorValue

        return (int) (0.097*sensorValue) * (255/100);
        break;
    }
    case 13:   //Vehicle speed
    {
      //MAX VALUE = 255 km/h
      //MIN VALUE = 0 km/h
      //EQUATION => Vehicle_speed = 0.097*sensorValue

        return (int) (0.249*sensorValue);
        break;
    }
    case 82:   //Ethanol percentage
    {
      //MAX VALUE = 100%
      //MIN VALUE = 0%
      //EQUATION => Ethanol_percentage = 0.097*sensorValue

        return (int) (0.097*sensorValue) * (255/100);
        break;
    }
    case 47:   //Fuel level
    {
      //MAX VALUE = 100%
      //MIN VALUE = 0%
      //EQUATION => Fuel_level = 0.097*sensorValue

        return (int) (0.097*sensorValue) * (255/100);
        break;
    }
    default:
    {
      return -1;
      break;
    }
}
}

void setup() 
{
  Serial.begin(115200);
  
  while (startedFlag == 0)
{
  // Try and open the CAN controller:
  // https://github.com/coryjfowler/MCP_CAN_lib/blob/master/mcp_can_dfs.h
  // MCP_ANY = Disables Masks and Filters and accepts any message on the CAN Bus (MCP_STDEXT,MCP_STD,MCP_EXT,MCP_ANY)
  // CAN_500KBPS specifies a baud rate of 500 Kbps
  // MCP_16MHZ indicates a 16MHz oscillator (crystal) is used as a clock for the MCP2515 chip
  //           you need to check your MCP2515 circuit for the right frequency (MCP_8MHZ, MCP_16MHZ, MCP_20MHZ)
  if(CAN_OK == CAN1.begin(MCP_ANY,CAN_250KBPS, MCP_8MHZ))                   
  {
      Serial.println("CAN BUS Shield init ok!");
      CAN1.setMode(MCP_NORMAL); // Set operation mode to normal so the MCP2515 sends acks to received data.
      pinMode(CAN1_INT, INPUT); // Configuring pin for /INT input
      startedFlag = 1;
      delay(1000);
  }
  else
  {
      Serial.println("CAN BUS Shield init fail");
      Serial.println("Init CAN BUS Shield again");
      delay(100);
  }
}
}

void loop()
{
   if(!digitalRead(CAN1_INT))
   {

      Serial.println("Message received"); 
      CAN1.readMsgBuf(&canId, &len, buf); 
        // FROM: https://en.wikipedia.org/wiki/OBD-II_PIDs#CAN_(11-bit)_bus_format
        // A CANId value of 0x7DF indicates a query from a diagnostic reader, which acts as a broadcast address and accepts
        // responses from any ID in the range 0x7E8 to 0x7EF.  ECUs that can respond to OBD queries listen both to the functional 
        // broadcast ID of 0x7DF and one assigned in the range 0x7E0 to 0x&7E7.  Their response has an ID of their assigned ID
        // plus 8 (e.g. 0x7E8 through 0x7EF).  Typically, the main ECU responds on 0x7E8.
        
        for(int i = 0; i<len; i++)
        {  
          canMessageRead[i] = buf[i];
        }
  
        Serial.print("PID Requested from device:");
        Serial.println(canMessageRead[2]);
        
        int responseId = canId+8;
        //Check which message was received
        pid = canMessageRead[2];

        switch(pid)
          {
            case 5:  //Engine coolant temperature
            {
                ect = convertSensorValue(5,analogRead(TEMP_PIN));
                //ect += 40;
                byte ectSensor[8] = {3, 65, ENGINE_COOLANT_TEMPERATURE, ect ,0,0,0,0};
                CAN1.sendMsgBuf(responseId, 0, 8, ectSensor);
                Serial.println("Sending back the coolant temperature to device!"); 
                Serial.println(ect);      
                break;
            }   
            case 12:  //Engine RPM
            {
                rpm = convertSensorValue(12,analogRead(RPM_PIN));
                byte rpmSensor[8] = {4, 65, ENGINE_RPM, uintMSB(rpm), uintLSB(rpm),0,0,0};
                CAN1.sendMsgBuf(responseId, 0, 8, rpmSensor);
                Serial.println("Sending back the engine RPM to device!");     
                Serial.println(rpm); 
                break;
            }
            case 17:   //Throttle position
            {
                throttle_pos = convertSensorValue(17,analogRead(THROTTLE_PIN));
                byte throttleSensor[8] = {3, 65, THROTTLE_POSITION, throttle_pos,0,0,0,0};
                CAN1.sendMsgBuf(responseId, 0, 8, throttleSensor);
                Serial.println("Sending back the throttle position to device!"); 
                Serial.println(throttle_pos);     
                break;
            }
            case 13:   //Vehicle speed
            {
                veh_speed = convertSensorValue(13,analogRead(SPEED_PIN));
                byte speedSensor[8] = {3, 65, VEHICLE_SPEED, veh_speed,0,0,0,0};
                CAN1.sendMsgBuf(responseId, 0, 8, speedSensor);
                Serial.println("Sending back the vehicle speed to device!");  
                Serial.println(veh_speed);    
                break;
            }
            case 49:   //Distance since DTC cleared
            {
                currentTime = millis();
                Serial.print("Current time  = ");
                Serial.println(currentTime);
                 Serial.print("Last time  = ");
                Serial.println(lastTime);
                unsigned long elapsedTime = (currentTime - lastTime);  //get the elapsed time in seconds
                Serial.print("Elapsed time  = ");
                Serial.println(elapsedTime);
                float deltaOdometer = (veh_speed/3.6) * elapsedTime * 0.001;
                Serial.print("Delta odometer  = ");
                Serial.println(deltaOdometer);
                odometer += deltaOdometer; 
                byte odometerSensor[8] = {4, 65, ODOMETER, odometer, 0,0,0,0};
                CAN1.sendMsgBuf(responseId, 0, 8, odometerSensor);
                Serial.println("Sending back the odometer to device!");  
                Serial.println(odometer);    
                break;
            }
            case 82:   //Ethanol percentage
            {
                eth_percentage = convertSensorValue(82,analogRead(ETHANOL_PIN));
                byte ethanolSensor[8] = {3, 65, ETHANOL_PERCENTAGE, eth_percentage,0,0,0,0};
                CAN1.sendMsgBuf(responseId, 0, 8, ethanolSensor);
                Serial.println("Sending back the ethanol percentage to device!");     
                Serial.println(eth_percentage); 
                break;
            }
            case 47:   //Fuel level
            {
                fuel_level = convertSensorValue(47,analogRead(FUEL_PIN));
                byte fuelSensor[8] = {3, 65, FUEL_LEVEL, fuel_level,0,0,0,0};
                CAN1.sendMsgBuf(responseId, 0, 8, fuelSensor);
                Serial.println("Sending back the fuel level to device!");     
                Serial.println(fuel_level); 
                break;
            }
            default:
            {
              return -1;
              break;
            }
    }
    delay(500);
    lastTime = millis();
}
}
