///////////////////////////////////////////////////////////
//
//  Network Embedded Systems - 2016/2017 (second semester)
//  Interconnecting "mbeds" by a CAN
//
//  Code for the MBED3
//
//  Work done by:
//  Joao Belfo - 78913
//  Joao Girao - 78761
//  Luis Rei   - 78486
//
//  Rev: 1.1 (26/05/2017)
//
///////////////////////////////////////////////////////////

#include "stdio.h"
#include "mbed.h"
#include "CAN.h"    //allows the sending/receiving of messages through a CAN bus
#include "C12832.h" //allows the use of LCD
#include "PowerControl/PowerControl.h"
#include "DHT22.h"

///////////////////////////////////////////////////////////////////////////////
//  DECLARING GLOBAL VARIABLES                                               //
///////////////////////////////////////////////////////////////////////////////

//Redefinition of LEDS
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

AnalogIn ldr(p17);  //allows us to use the light sensor
DHT22 dht22(p27);   //allows us to use the humisity sensor

//Definition of receiver and trasmitter pins
CAN can1(p30, p29); //(receiver pin, trasmitter pin)
CANMessage msg; //Buffer handled in communications

//Buttons for Joystick use (and hexadecimal code for reference)
BusIn joy(p15,p12,p13,p16); //up (0x1), left (0x4), down (0x2), right (0x8)
DigitalIn fire(p14); //Middle click (0xf)

C12832 lcd(p5, p7, p6, p8, p11); //LCD manipulation

Ticker ticker; //Timed event
Timeout light1; //program runtime
Timeout hum1; //program runtime

char data[7];

bool start_send_info_hum=false,start_send_info_lum=false, sleep=true, alarmOff=false;
bool see_luminosity=false,see_humidity=false;

int threshold_hum = 60;

float threshold_lum = 1, period = 3.0;


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
                start_send_info_lum=false;
                start_send_info_hum=false;
                alarmOff = false;
            }
            else if(msg.data[0] == '0'){
                alarmOff = false;
                sleep = false;
                start_send_info_lum=false;
                start_send_info_hum=false;
            }
        }else if(msg.id == 1){
            //shuts down alarm
            alarmOff = false;
            lcd.cls();
        }
        else if(msg.id == 5){
            if(msg.data[0] == '1') start_send_info_hum = true;
            else if(msg.data[0] != '1') start_send_info_hum=false;
            
            if(msg.data[0] == '3') start_send_info_lum=true;
            else if(msg.data[0] != '3') start_send_info_lum=false;
        }else if(msg.id == 9){
            sprintf(data, "%s", msg.data);
            threshold_hum = atof(data);
        }else if(msg.id == 11){
            sprintf(data, "%s", msg.data);
            threshold_lum = atof(data);
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

void check_light(){ 
    // just set a flag and then in the main, we will process information   
    see_luminosity=true;    
}

///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT will be called when the timer reach the period time.       //
///////////////////////////////////////////////////////////////////////////////

void check_hum(){
    // just set a flag and then in the main, we will process information    
    see_humidity=true;    
}
   
   
///////////////////////////////////////////////////////////////////////////////
//  MAIN                                                                     //
///////////////////////////////////////////////////////////////////////////////

int main() {
    can1.attach(&receive);    
    PwmOut spkr(p26); //Speaker with PWM driver   
    
    float luminosity=0;
    bool prog_flag = false;
    char c[7];
    int humidity=0;;
    
    while(1) {
        //Node sleeping
        if(sleep){
            prog_flag = false;
            lcd.cls();
            Sleep();
        }
        //Node woke up
        else if(!sleep && !prog_flag){
            prog_flag = true;
            light1.attach(&check_light,period);
            hum1.attach(&check_hum,period);
        }      
        
        //If the user chose a mode that required the assistance of this node...
        //The node receives a message from the master node (1100) and wakes up.
        if(see_humidity){    
            dht22.sample();
            humidity=dht22.getHumidity()/10.0;    
              
            if(humidity>=threshold_hum){
                alarmOff=true;
                lcd.cls(); lcd.locate(0,0); lcd.printf("Threshold %d exceeded!", threshold_hum);
                lcd.locate(0,10); lcd.printf("Hum is high: %d",humidity);  
                sprintf(c, "%d", humidity); 
                
                if(can1.write(CANMessage(2, c,7))){
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
                lcd.locate(0,0); 
                lcd.printf("Humidity at %d         ", humidity);
            }
            see_humidity = false;
            if(!sleep) hum1.attach(&check_hum,period);            
        }
        
        if(see_luminosity){    
            luminosity = ldr.read()*10; 
                
            if(luminosity<=threshold_lum){
                alarmOff = true;
                lcd.cls(); lcd.locate(0,0); lcd.printf("Threshold %.2f exceeded!", threshold_lum);
                lcd.locate(0,10); lcd.printf("Lum is high: %.2f         ",luminosity);  
                sprintf(c, "%.2f", luminosity); 
                if(can1.write(CANMessage(4, c,7))){
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
                lcd.locate(0,10); 
                lcd.printf("Luminosity at %.2f         ", luminosity);
            }
            see_luminosity=false;
            if(!sleep) light1.attach(&check_light,period);            
        }
        
        if(start_send_info_hum || start_send_info_lum){
            while(start_send_info_hum || start_send_info_lum){
                if(start_send_info_hum){
                    dht22.sample();
                    humidity = dht22.getHumidity()/10.0;
                    if(humidity>=threshold_hum){
                        alarmOff = true;
                        lcd.cls(); lcd.locate(0,0); lcd.printf("Threshold %.2f exceeded!", threshold_hum);
                        lcd.locate(0,10); lcd.printf("Hum is high: %d        ",humidity);  
                        sprintf(c, "%d", humidity);    
                        if(can1.write(CANMessage(2, c,7))){
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
                        lcd.locate(0,0); 
                        lcd.printf("Humidity at %d         ", humidity);
                        sprintf(c, "%d", humidity);
                        can1.write(CANMessage(6, c,7));
                        wait((int)period);
                    }
                }else if(start_send_info_lum){
                    luminosity= ldr.read()*10;
                    if(luminosity<=threshold_lum){
                        alarmOff = true;
                        lcd.cls(); lcd.locate(0,0); lcd.printf("Threshold %.2f exceeded!", threshold_lum);
                        lcd.locate(0,10); lcd.printf("Lum is high: %.2f        ",luminosity);  
                        sprintf(c, "%.2f", luminosity); 
                               
                        if(can1.write(CANMessage(4, c,7))){
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
                        lcd.locate(0,10); 
                        lcd.printf("Luminosity at %.2f         ", luminosity);
                        sprintf(c, "%.2f", luminosity);
                        can1.write(CANMessage(8, c,7));
                        wait((int)period);
                    }
                }           
            }
            
            see_luminosity = false;
            see_humidity = false;
            if(!sleep){
                 light1.attach(&check_light,period); 
                 hum1.attach(&check_hum,period); 
            }
        }
    }
}