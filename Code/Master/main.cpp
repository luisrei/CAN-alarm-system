    ////////////////////////////////////////////////////////////////////////////////
//
//  Network Embedded Systems - 2016/2017 (second semester)
//  Interconnecting "mbeds" by a CAN
//
//  Code for the MBED1 - Master
//
//  Work done by:
//  Joao Belfo - 78913
//  Joao Girao - 78761
//  Luis Rei   - 78486
//
//  Rev: 1.7 (26/05/2017)
//
////////////////////////////////////////////////////////////////////////////////

#include "stdio.h"
#include "mbed.h"
#include "CAN.h"    //allows the sending/receiving of messages through a CAN bus
#include "C12832.h" //allows the use of LCD
#include "cstdlib"
#include "string"
#include "PinDetect.h"
#include "stdlib.h"
#include "PowerControl/PowerControl.h"

///////////////////////////////////////////////////////////////////////////////
//  DECLARING GLOBAL VARIABLES                                               //
///////////////////////////////////////////////////////////////////////////////


//Redefinition of LEDS
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

PinDetect OnOff( p7, PullUp ); //on/off switch
PinDetect alarm( p8, PullDown ); //opens menu

AnalogIn pot1(p19); //Potentiometers
AnalogIn pot2(p20);

//Definition of receiver and trasmitter pins
CAN can1(p30, p29);
CANMessage msg; //Defines buffer to handle communications

//Buttons for Joystick use (and hexadecimal code for reference)
BusIn joy(p15,p12,p13,p16); //up (0x1), left (0x4), down (0x2), right (0x8)
PinDetect fire(p14); //Middle click (0xf)
BusOut leds(LED1,LED2,LED3,LED4);

C12832 lcd(p5, p7, p6, p8, p11); //LCD manipulation

Timeout ticker; //Timed event
Timer temp1; //program runtime
Timer debounce1, debounce2, debounce3; //button debounce

char counter = 0; //Counter of messages sent
char select = '1'; //Predefined mode of functionality is mode 1
char shuttingDown = 0; //1 if it's the program is close to shutting down
char usedpass[5]; //Keeps the correct password for comparison
char pass[5]; //Keeps the password from the joystick
char c[7];

float period = 3.0; //periodicity of sensor information
//thresholds
float temp_threshold = 34;
float hum_threshold = 50;
float light_threshold = 500;
float threshold, hum_aux, temp_aux, light_aux;
float light_threshold2, hum_threshold2, temp_threshold2;

volatile int pressed_OnOff = 0, pressed_alarm = 0, pressed_fnc=0; //check if the button was pressed
int JoyIn; //Number of Joystick movements

bool on = false; //Defines system state
bool info = false, alarmed = false, menu, popmenu = false;

LocalFileSystem local("local");  // Create the local filesystem under the name "local"
FILE * fp; //file with the current password



///////////////////////////////////////////////////////////////////////////////
//  DECLARING FUNCTIONS                                                      //
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT                                                            //
//  Sends message to a node informing he needs to start sending info         //
///////////////////////////////////////////////////////////////////////////////

//Sends message to a node informing he needs to start sending info
void sendMode() {
    sprintf(c, "%c", select);
    if(can1.write(CANMessage(5, c, 1))){}
    else{
        lcd.cls();
        lcd.locate(0,10); lcd.printf("Error sending message");
    }
}


///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT PUTS EVERYONE TO SLEEP                                     //
///////////////////////////////////////////////////////////////////////////////

void sleepAll(){
    //Puts everyone to sleep
    if(can1.write(CANMessage(0000, "1",1))){
    }else{
        lcd.cls();
        lcd.locate(0,10); lcd.printf("Error sending message");
    }
    temp1.reset();
    temp1.stop();
    pressed_OnOff = 0;
    pressed_alarm = 0;
    pressed_fnc = 0;
    lcd.cls(); lcd.locate(0,0); lcd.printf("A desligar...");
    wait_us(2000000);
    lcd.cls();
}


///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT Stores the password in memory                              //
///////////////////////////////////////////////////////////////////////////////

void savepass(){
    fp = fopen("/local/pass.txt", "w");  // Write to "pass.txt"
    fprintf(fp, "%s", pass);
    fclose(fp);
}


///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT Reads password from local file                             //
///////////////////////////////////////////////////////////////////////////////

void readpass(){
    fp = fopen("/local/pass.txt", "r");  // Read "pass.txt"
    
    //if there's a file to read, read it and update 'usedpass' variable
    if(fp){
        fscanf(fp, "%s", usedpass);
        fclose(fp);
        
    //if there isn't, create a file and give 'usedpass' a default value
    }else{
        fp = fopen("/local/pass.txt", "w");
        fprintf(fp, "uldr");
        strcpy(usedpass, "uldr"); //default password
        fclose(fp);
    }
}

///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT Handles CAN interrupts                                     //
///////////////////////////////////////////////////////////////////////////////

void receive() {
    if(can1.read(msg)){
        if( msg.id == 2 || msg.id == 3 || msg.id == 4 ){
            alarmed = true;
            temp1.stop();  
        }
        else if( msg.id == 6 || msg.id == 7 || msg.id == 8){
            lcd.locate(0,10); lcd.printf("Reading %s from node %d", msg.data, msg.id);
        }
    }
    temp1.reset();
}

///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT Saves the password from the Joystick movements             //
///////////////////////////////////////////////////////////////////////////////


void check_JoyIn_pass(){
    JoyIn = 0;

    while(JoyIn < 4 && on){
        if( joy==0x1 || joy==0x4 || joy==0x2 || joy==0x8){
            temp1.reset();
            lcd.locate(0,20); lcd.printf("                       ");
            JoyIn++; //increment number of joystick movements 
            if (JoyIn == 1){
                led1 = 1;
                if( joy == 0x1 ){ pass[0]='u';}
                else if( joy == 0x2 ){ pass[0]='d';}
                else if( joy == 0x4 ){ pass[0]='l';}
                else if( joy == 0x8 ){ pass[0]='r';}
            }else if (JoyIn == 2){
                led2 = 1;
                if( joy == 0x1 ){ pass[1]='u';}
                else if( joy == 0x2 ){ pass[1]='d';}
                else if( joy == 0x4 ){ pass[1]='l';}
                else if( joy == 0x8 ){ pass[1]='r';}                
            }else if (JoyIn == 3){
                led3 = 1;
                if( joy == 0x1 ){ pass[2]='u';}
                else if( joy == 0x2 ){ pass[2]='d';}
                else if( joy == 0x4 ){ pass[2]='l';}
                else if( joy == 0x8 ){ pass[2]='r';}
            }else if (JoyIn == 4){
                led4 = 1;
                if( joy == 0x1 ){ pass[3]='u'; }
                else if( joy == 0x2 ){ pass[3]='d'; }
                else if( joy == 0x4 ){ pass[3]='l'; }
                else if( joy == 0x8 ){ pass[3]='r'; } 
            }                    
            wait_us(500000);  
        }     
    }
}

///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT Chooses the mode from the Joystick movements               //
///////////////////////////////////////////////////////////////////////////////

void check_JoyIn_mode(){

    while(!fire && on){
        //Selection of the modes
        //Press right or left to choose the intended functionality
        if (joy == 0x8){
            temp1.reset();
            lcd.locate(0,20); lcd.printf("                       ");
            if(led1){             
                select = '2';
                led1 = 0;
                led2 = 1; 
                
                lcd.cls(); lcd.locate(0,0); lcd.printf("Select Mode:");        
                lcd.locate(0,10); lcd.printf("Mode 2: Temperature");
                 
            }else if (led2){
                select = '3';
                led2 = 0; 
                led3 = 1; 
                
                lcd.cls(); lcd.locate(0,0); lcd.printf("Select Mode:");        
                lcd.locate(0,10); lcd.printf("Mode 3: Luminosity");
            }else if (led3){
                select = '4';
                led3 = 0; 
                led4 = 1; 
                
                lcd.cls(); lcd.locate(0,0); lcd.printf("Select Mode:");        
                lcd.locate(0,10); lcd.printf("Mode 4: Change password");
            }else{
                lcd.cls(); lcd.locate(0,0); 
                lcd.printf("Numero de modos excedido!");
                wait_us(2000000);
                lcd.cls(); lcd.locate(0,0); lcd.printf("Select Mode:");        
                lcd.locate(0,10); lcd.printf("Mode 4: Change password");
            }
            wait_us(500000);      
        }else if (joy == 0x4){
            temp1.reset();
            lcd.locate(0,20); lcd.printf("                       ");
            if(led1){                
                lcd.cls(); lcd.locate(0,0); 
                lcd.printf("Numero de modos excedido!");
                wait_us(2000000);
                lcd.cls(); lcd.locate(0,0); lcd.printf("Select Mode:");        
                lcd.locate(0,10); lcd.printf("Mode 1: Humidity");
                
            }else if (led2){
                select = '1';
                led2 = 0;
                led1 = 1;
                
                lcd.cls(); lcd.locate(0,0); lcd.printf("Select Mode:");        
                lcd.locate(0,10); lcd.printf("Mode 1: Humidity");
            }else if (led3){
                select = '2';
                led3 = 0;
                led2 = 1;
                
                lcd.cls(); lcd.locate(0,0); lcd.printf("Select Mode:");        
                lcd.locate(0,10); lcd.printf("Mode 2: Temperature");
            }else if (led4){
                select = '3';            
                led4 = 0;
                led3 = 1;
                
                lcd.cls(); lcd.locate(0,0); lcd.printf("Select Mode:");        
                lcd.locate(0,10); lcd.printf("Mode 3: Luminosity");
            }
            wait_us(500000);  
        }   
    } 
}


///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT Implements password system                                 //
///////////////////////////////////////////////////////////////////////////////

void passInsert(){  
    menu = true;     
    led1 = led2 = led3 = led4 = 0;
    
    readpass();
    
    //Writes a message in a certain location (in this case position (0,0))
    lcd.cls();  lcd.locate(0,0); lcd.printf("WELCOME!");
    wait_us(1000000);
    lcd.locate(0,10); lcd.printf("INSERT PASSWORD, PLEASE");
    temp1.start();
    
    while(on){ 
        check_JoyIn_pass();
        if(!on){
            info = false;
            alarmed = false;
            menu = false;
            break; 
        } 
        
        //If it doesn't, ask to reinsert password
        if(strcmp(pass, usedpass)){            
            lcd.locate(0,20); lcd.printf("PASSWORD INCORRECT");
            wait_us(1000000);
            led1 = led2 = led3 = led4 = 0;
            lcd.cls(); lcd.locate(0,0); lcd.printf("REINSERT PASSWORD");
        }else{
            //If it does, unlock the system. You can now proceed
            lcd.cls(); lcd.locate(0,10); lcd.printf("PASSWORD CORRECT");
            led1 = led2 = led3 = led4 = 0;
            // wakes up everybody
            if(can1.write(CANMessage(0000, "0", 1))){
            }else{
                lcd.cls();
                lcd.locate(0,10); lcd.printf("Error sending message");
            }
            wait_us(1000000);
            menu = false;
            break;            
        }           
    }
    
}


///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT Implements mode selection                                  //
//  The password was correct. You can now choose from a variety of modes.    //
//  To do so, choose the mode use the joystick once again.                   //
//  The number of the mode is the amount of leds turned on. The modes are:   //
//                                                                           //
//  Mode 1: Selects Humidity mode                                            //
//  Mode 2: Selects Temperature mode                                         //
//  Mode 3: Selects Luminosity mode                                          //
//  Mode 4: Change password                                                  //
///////////////////////////////////////////////////////////////////////////////

void modeSelect(){
    temp1.reset();
    wait_us(100000);
    menu = true;

    while(on){
        led1 = 1;
        select = '1';
        lcd.cls(); lcd.locate(0,0); lcd.printf("Select Mode:");        
        lcd.locate(0,10); lcd.printf("Mode 1: Humidity");
        check_JoyIn_mode();
        if(!on){
            info = false;
            alarmed = false;
            menu = false;
            break; 
        }
        lcd.cls(); lcd.locate(0,0); lcd.printf("Mode selected successfully");
        led1 = led2 = led3 = led4 = 0;
        wait_us(1000000);
        
        //if the forth mode was picked, choose the new password
        //if not, wake up the respective node and proceed
        if (select == '4'){
            lcd.cls(); lcd.locate(0,0); lcd.printf("Select New Pass");
            
            check_JoyIn_pass();
            if(!on){
                info = false;
                alarmed = false;
                menu = false;
                break; 
            }
            strcpy(usedpass, pass); //updates password
            savepass(); //Stores the password in memory         
            lcd.cls(); lcd.locate(0,0); lcd.printf("Password reselected: %s", pass);
            wait_us(1000000);
            led1 = led2 = led3 = led4 = 0;
        }else{ 
            shuttingDown = 0;
            break;     
        } 
    }
    
    if(on){
        lcd.cls();
        lcd.locate(0,0); lcd.printf("Press fire for more actions"); 
        wait_us(1000000);
        menu = false;
        pressed_fnc = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT Chooses threshold                                          //
///////////////////////////////////////////////////////////////////////////////

void choose_threshold(){  
    temp1.stop();
    lcd.cls(); lcd.locate(0,0); lcd.printf("Entered Threshold mode!");
    lcd.locate(0,10); lcd.printf("Left/right to change");
    lcd.locate(0,20); lcd.printf("                        ");
    wait_us(500000);
    
    while(!fire && on){
        //Calculates new humidity threshold
        if(select == '1'){
            hum_aux = hum_threshold;
            hum_threshold2 = 2*(pot1.read())*hum_aux - 1.5*(pot2.read())*hum_aux;
            lcd.locate(0,0); lcd.printf("                        ");
            lcd.locate(0,10); lcd.printf("                        ");
            lcd.locate(0,20); lcd.printf("                        "); 
            lcd.locate(0,0);
            lcd.printf("Humidity threshold: %.2f", hum_threshold2);
            wait_us(300000);  
        }
        //Calculates new temperature threshold
        else if(select == '2'){
            temp_aux = temp_threshold;
            temp_threshold2 = 2*(pot1.read())*temp_aux - 1.5*(pot2.read())*temp_aux;
            lcd.locate(0,0); lcd.printf("                        ");
            lcd.locate(0,10); lcd.printf("                        ");
            lcd.locate(0,20); lcd.printf("                        "); 
            lcd.locate(0,0); 
            lcd.printf("Temperature threshold: %.2f", temp_threshold2);
            wait_us(300000); 
        }
        //Calculates new light threshold
        else if(select == '3'){
            light_aux = light_threshold;
            light_threshold2 = 2*(pot1.read())*light_aux - 1.5*(pot2.read())*light_aux;
            lcd.locate(0,0); lcd.printf("                        ");
            lcd.locate(0,10); lcd.printf("                        ");
            lcd.locate(0,20); lcd.printf("                        ");
            lcd.locate(0,0); 
            lcd.printf("Light threshold: %.2f", light_threshold2);
            wait_us(300000); 
        }
        wait_us(500000);
    }
    light_threshold = light_threshold2;
    temp_threshold = temp_threshold2;
    hum_threshold = hum_threshold2;
    temp1.start();
}

///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT  Chooses node information periodicity                      //
///////////////////////////////////////////////////////////////////////////////

void choose_period_info(){   
    lcd.cls(); lcd.locate(0,0); lcd.printf("Entered Period mode!");
    lcd.locate(0,10); lcd.printf("Current period: %.2f", period);
    lcd.locate(0,20); lcd.printf("Left/right to change");
    
    while(!fire && on){
        //Joystick to the right
        if (joy == 0x8){
            temp1.reset();
            //Superor limit
            if(period <= 9.9){     
                period = period + 0.2;  
                lcd.locate(0,0); lcd.printf("                        ");
                lcd.locate(0,10); lcd.printf("                        ");
                lcd.locate(0,20); lcd.printf("                        ");         
                lcd.locate(0,0); lcd.printf("New period: %.2f", period);
                lcd.locate(0,10); lcd.printf("Left/right to change");        
                wait_us(300000);
            }else{
                lcd.locate(0,10); lcd.printf("Can't go above 9.9 secs");
                lcd.locate(0,20); lcd.printf("                       ");
                wait_us(1000000);
                lcd.locate(0,10); lcd.printf("                       ");

                wait_us(300000);
            } 
        }
        //Joystick to the left
        else if (joy == 0x4){
            temp1.reset();            
            //Inferior limit
            if(period >= 2.1){     
                period = period - 0.1;  
                lcd.locate(0,0); lcd.printf("                        ");
                lcd.locate(0,10); lcd.printf("                        ");
                lcd.locate(0,20); lcd.printf("                        ");         
                lcd.locate(0,0); lcd.printf("New period: %.2f", period);
                lcd.locate(0,10); lcd.printf("Left/right to change");        
                wait_us(300000);
            }else{
                lcd.locate(0,10); lcd.printf("Can't go lower than 2 secs");
                lcd.locate(0,20); lcd.printf("                        ");
                wait_us(1000000);
                lcd.locate(0,10); lcd.printf("                        ");
                wait_us(300000);
            }    
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT                                                            //
//  Chooses several functionalities through Joystick commands                //
//  Up to choose a new threshold                                             //
//  Right to select a new period                                             //
//  Down to toggle messages                                                  //
//  Left to open the menu                                                    //
///////////////////////////////////////////////////////////////////////////////

void options(){
    menu = true;  
    temp1.reset();
    lcd.cls();
    lcd.locate(0,0); lcd.printf("Up: Threshold, Right: Period");
    lcd.locate(0,10); lcd.printf("Left open menu");
    lcd.locate(0,20); lcd.printf("Down to toggle messages");   
        
    while(on){ 
        //Joystick right
        if (joy == 0x8){
            temp1.reset();
            wait_us(300000);
            choose_period_info();
            if(!on){
                info = false;
                alarmed = false;
                menu = false;
                break; 
            }
            sprintf(c, "%.2f", period);
            
            if(can1.write(CANMessage(12, c, 7))){}
            else{
                lcd.cls();
                lcd.locate(0,0); lcd.printf("Error sending message");   
            }
            
            wait_us(200000);
            break;              
        }
        //Joystick up
        else if (joy == 0x1){
            temp1.reset();
            wait_us(300000);
            choose_threshold();
            if(!on){
                info = false;
                alarmed = false;
                menu = false;
                break; 
            }
            
            if(select == '1'){
                sprintf(c, "%.2f", hum_threshold);
                
                if(can1.write(CANMessage(9, c, 7))){}
                else{
                    lcd.cls();
                    lcd.locate(0,0); lcd.printf("Error sending message");   
                }
                wait_us(200000);
            }
            else if (select == '2'){
                sprintf(c, "%.2f", temp_threshold);
                
                if(can1.write(CANMessage(10, c, 7))){}
                else{
                    lcd.cls();
                    lcd.locate(0,0); lcd.printf("Error sending message");   
                }
                wait_us(200000);
            }
            else if(select == '3'){
                sprintf(c, "%.2f", light_threshold);
                if(can1.write(CANMessage(11, c, 7))){}
                else{
                    lcd.cls();
                    lcd.locate(0,0); lcd.printf("Error sending message");   
                }
                wait_us(200000);
            }
            break;
        }
        //Joystick down
        else if (joy == 0x2){
            temp1.reset();
            wait_us(300000);
            info = !info;
            if(info){
                sendMode();
                wait_us(100000);
                break;
            }else {
                if(can1.write(CANMessage(5, "0", 1))){}
                else{
                    lcd.cls();
                    lcd.locate(0,0); lcd.printf("Error sending message");   
                }
                wait_us(100000);
                break;
            }
            
        }
        //Joystick left
        else if (joy == 0x4){
            temp1.reset();
            wait_us(300000);
            
            //Stops receiving info
            if(can1.write(CANMessage(5, "0" ,1))){
                info = false;
            }
            else{
                lcd.cls();
                lcd.locate(0,0); lcd.printf("Error sending message");   
            }
            wait_us(1000000);
            modeSelect();
            if(!on){
                info = false;
                alarmed = false;
                menu = false;
                break; 
            }
            popmenu = true;
            break;
        }
    }
    
    if(on && !popmenu){
        lcd.cls();
        lcd.locate(0,0); lcd.printf("Press fire for more actions");
        wait_us(1000000);
        menu = false;
    }
    pressed_fnc = 0;
    popmenu = false;
}


///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT Deals with switch bouncing on the on/off button            //
///////////////////////////////////////////////////////////////////////////////

void onIn( void ) {
    if (debounce1.read_us() > 200000) {
        on = !on;
        pressed_OnOff++;
        temp1.reset();
        debounce1.reset();
        led1 = led2 = led3 = led4 = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT                                                            //
//  Verifies program run time                                                //
///////////////////////////////////////////////////////////////////////////////

void verifyTimer(){
    if( temp1.read_us() > 10000000 && temp1.read_us() < 20000000 && shuttingDown == 0){
        shuttingDown = 1;
        lcd.locate(0,20); lcd.printf("                       ");
        lcd.locate(0,20); lcd.printf("Vamos desligar em breve");      
    }
    else if (temp1.read_us() < 10000000){
        shuttingDown = 0; 
    }
    else if(temp1.read_us() > 20000000 ){
        on = false;
    }
    if(on) ticker.attach(&verifyTimer, 2.0);
}


///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT Deals with switch bouncing on the alarm button!            //
///////////////////////////////////////////////////////////////////////////////

void alarmOff( void ) {
    if (debounce2.read_us() > 200000) {
        if(alarmed){
            temp1.start();
            //Makes nodes stop alarm
            if(can1.write(CANMessage(1, "1" ,1))){}
            else{
                lcd.cls();
                lcd.locate(0,0); lcd.printf("Error sending message");   
            }
            alarmed = false;
        }
        lcd.locate(0,20); lcd.printf("                       ");
        temp1.reset();
        debounce2.reset();
    }      
}

///////////////////////////////////////////////////////////////////////////////
//  FUNCTION THAT Deals with switch bouncing on the options button!          //
///////////////////////////////////////////////////////////////////////////////
void fnc(){
    if (debounce3.read_us() > 200000) {
        pressed_fnc++;
        temp1.reset();
        debounce3.reset();
    } 
}



///////////////////////////////////////////////////////////////////////////////
//  MAIN                                                                     //
///////////////////////////////////////////////////////////////////////////////

int main() {
     
    can1.attach(&receive); 
    
    //Defines button interrupt
    debounce1.start();   //Allows us to manage button debouncing problems
    debounce2.start();
    debounce3.start();
    temp1.start();
    
    OnOff.setSampleFrequency(10000);
    alarm.setSampleFrequency(20000);
    fire.setSampleFrequency(30000);
    
    OnOff.setSamplesTillAssert(3);
    alarm.setSamplesTillAssert(3);
    fire.setSamplesTillAssert(3);
    
    OnOff.attach_asserted( &onIn );
    alarm.attach_asserted( &alarmOff );
    fire.attach_asserted( &fnc );
    
    while(1){
        Sleep();
        while(on){
            //Pressing the button turns on/off the system
            if( pressed_OnOff > 0){
                pressed_OnOff = -1;
                ticker.attach(&verifyTimer, 2.0);
                //Insert password and choose a mode for the program to function        
                passInsert();
                if(!on){
                    info = false;
                    alarmed = false;
                    menu = false;
                    sleepAll();
                    break; 
                }
                //everyone is already checking for sensor values
                //select mode
                modeSelect();
                if(!on){
                    info = false;
                    alarmed = false;
                    menu = false;
                    sleepAll();
                    break; 
                }   
            }             

            //Check for the options menu button
            if( pressed_fnc > 0){
                pressed_fnc = 0;
                //Master stops receiving info
                if(can1.write(CANMessage(5, "0" ,1))){}
                else{
                    lcd.cls();
                    lcd.locate(0,0); lcd.printf("Error sending message");   
                }
                options();               
            } 
            
            if(!on){
                info = false;
                alarmed = false;
                menu = false;
                sleepAll(); 
            }
        } 
    }
}