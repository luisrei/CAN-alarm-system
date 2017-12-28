///////////////////////////////////////////////////////////
//
//  Network Embedded Systems - 2016/2017 (second semester)
//  Interconnecting "mbeds" by a CAN
//
//  Code for the MBED2
//
//  Work done by:
//  Joao Belfo - 78913
//  Joao Girao - 78761
//  Luis Rei   - 78486
//
//  Rev: 1.4 (26/05/2017)
//
///////////////////////////////////////////////////////////

#include "stdio.h"
#include "mbed.h"
#include "LM75B.h"  //allows use of temperature sensor
#include "CAN.h"    //allows the sending/receiving of messages through a CAN bus
#include "C12832.h" //allows the use of LCD
#include <cstdlib>
#include <string>
#include <stdlib.h>
#include "PowerControl/PowerControl.h"


///////////////////////////////////////////////////////////////////////////////
//  DECLARING GLOBAL VARIABLES                                               //
///////////////////////////////////////////////////////////////////////////////


//Redefinition of LEDS
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

//Definition of receiver and trasmitter pins
CAN can1(p30, p29); //(receiver pin, trasmitter pin)
CANMessage msg; //Buffer handled in communications

bool start_send_info=false,see_temp=false, sleep=true, alarmOff = false;

float threshold = 34, period = 3.0;

char data[7];

//Buttons for Joystick use (and hexadecimal code for reference)
BusIn joy(p15,p12,p13,p16); //up (0x1), left (0x4), down (0x2), right (0x8)
DigitalIn fire(p14); //Middle click (0xf)

C12832 lcd(p5, p7, p6, p8, p11); //LCD manipulation

Timeout temp1; //program runtime

LM75B sensor(p28,p27); //temperature sensor


///////////////////////////////////////////////////////////////////////////////
//  DECLARING FUNCTIONS                                                      //
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT deals with the sending of a message identified by an ID.   //
///////////////////////////////////////////////////////////////////////////////

void receive() {
    if(can1.read(msg)){
        if(msg.id==0){
            if(msg.data[0] == '1'){
                lcd.cls();
                sleep = true;
                start_send_info=false;
                alarmOff = false;
            }
            else if(msg.data[0] == '0'){
                alarmOff = false;
                sleep = false;
            }
        }else if(msg.id == 1)
            //shuts down alarm
            alarmOff = false;
        else if(msg.id == 5){
            if(msg.data[0] == '2') 
                start_send_info = true;
            else if(msg.data[0] != '2') start_send_info=false;
        }else if(msg.id == 10){
            sprintf(data, "%s", msg.data);
            threshold = atof(data);
        }
        else if(msg.id == 12){
            sprintf(data, "%s", msg.data);
            period = atof(data);
        }
    }
}
 
 
///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT will be called when the timer reach the period time.       //
///////////////////////////////////////////////////////////////////////////////

 void check_temp(){   
    // just set a flag and then in the main, we will process information
    see_temp=true;    
}
   
   
///////////////////////////////////////////////////////////////////////////////
//  MAIN                                                                     //
///////////////////////////////////////////////////////////////////////////////

  
int main() {

    can1.attach(&receive);  //create interruptions when there is a message in the CAN 
    PwmOut spkr(p26);       //Speaker with PWM driver     

    float temperature=0;    // variavle that will have the temperature value, from the sensor
    bool prog_flag=false;   // variable that will be used to know if the timer was already programmed
    char c[7];              // variable that will have the temperature value, to send through the CAN network
    while(1) {
               
        if(sleep){
            prog_flag = false;
            Sleep();
        }
        else { 
            if(!sleep && !prog_flag){
                prog_flag = true;
                temp1.attach(&check_temp, period); // program the timer
            }
            
            //If the user chose a mode that required the assistance of this node...
            //The node receives a message from the master node and wakes up.
            if(see_temp){
                temperature = sensor.read();
                // if the temperature violates the threshold value, aan alarm message should be sent
                if(temperature >= threshold){
                    alarmOff=true;
                    lcd.cls(); lcd.locate(0,0); lcd.printf("Threshold %.2f exceeded!", threshold);
                    lcd.locate(0,10); lcd.printf("Temp is high: %.2f Celsius",temperature);  
                    sprintf(c, "%.2f", temperature); 
                    // write in the CAN the value that viotlates the threshold, using the alarm ID for this node
                    if(can1.write(CANMessage(3, c,7))){
                        lcd.locate(0,20); lcd.printf("Master notified of alarm!");
                        wait_us(1000000);
                    }
                    
                    //Plays alarm sound until user presses disable alarm button
                    while(alarmOff){
                        for (float i = 2000.0, n = 10000.0; i<n && alarmOff; i+=100) {
                            spkr.period(1.0/i);
                            spkr=0.5;
                            wait(0.1);
                        }
                        spkr=0.0;     
                    }
                }else{
                    lcd.cls(); lcd.locate(0,0); 
                    lcd.printf("Temperature at %.2f Celsius", temperature);
                    // if the temperature value is in the normal range of values, and if the master wants to see those values, we send them through the CAN
                    // with the infor ID of this node
                    if(start_send_info) {
                        sprintf(c, "%.2f", temperature);
                        if(can1.write(CANMessage(7, c,7))){}
                        else{
                            lcd.cls();
                            lcd.locate(0,10); lcd.printf("Error sending message");
                        }
                    }
                }
                see_temp=false;
                // if this node should go to sleep then we'll not program the timer
                if(!sleep)  temp1.attach(&check_temp, period);            
            }  
        }       
    }
}