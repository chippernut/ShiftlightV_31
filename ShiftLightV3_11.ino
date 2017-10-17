 /* 
   ________    _                                   __ 
  / ____/ /_  (_)___  ____  ___  _________  __  __/ /_
 / /   / __ \/ / __ \/ __ \/ _ \/ ___/ __ \/ / / / __/
/ /___/ / / / / /_/ / /_/ /  __/ /  / / / / /_/ / /_  
\____/_/ /_/_/ .___/ .___/\___/_/  /_/ /_/\__,_/\__/  
            /_/   /_/         


Visit us: www.chippernut.com 


ARDUINO RPM TACHOMETER MULTI-DISPLAY 
Written by Jonduino (Chippernut)
03-20-2016
Updated 08-09-2017


********************* Version NOTES ******************** 
/* v1.0 BETA 11/17/2013 -- Initial Release 
** v1.1 03/09/2014 - 
** Fixed bug with flasher that didn't correspond to brightness value 
** Improved sleep function, now it shuts off after 5-seconds of engine OFF 
** Other minor improvements. 
** v2.1 08/10/2015 -- Too many to list here. 
** v2.2 01/17/2016 -- 
** New Display Support
** v2.4 03/20/2016 --
** Added dimmer support 
** Fixed array mathematics (bug)
** Improved rotary encoder response
** Finer PPr Control (0.1)
** v3.0 08/08/2017
* New display support
* New LEDS (APA102c)
* New Animations
* General code improvements and optimization
*/ 


// Include these libraries 
#include <Adafruit_DotStar.h>
#include <EEPROM.h> 
#include <EEPROMAnything.h> 
#include "SSD1306Ascii.h"
#include "SSD1306AsciiSoftSpi.h"

void(* resetFunc) (void) = 0;
int DEBUG;
int NUMPIXELS; 
                                        
#define DATAPIN    A4
#define CLOCKPIN   A5

Adafruit_DotStar strip = Adafruit_DotStar(EEPROM.read(11), DATAPIN, CLOCKPIN, DOTSTAR_BGR);
                                             
const int rpmPin = 2; 
const int dimPin = A2;
const int sensorInterrupt = 0; 
const int timeoutValue = 10; 
volatile unsigned long lastPulseTime; 
volatile unsigned long interval = 0; 
volatile int timeoutCounter; 
long rpm;                                             
long rpm_last;                                               
int display_rpm;
int activation_rpm; 
int shift_rpm; 
int menu_enter = 0; 
int current_seg_number = 1;
int seg_mover = 0;
long previousMillis = 0;
int shiftinterval = 50;
boolean flashbool = true;      
int prev_animation;
int prev_color;
boolean testbright = false;
int prev_variable; 
int prev_menu;
boolean testdim = false; 
int justfixed; 

//array for rpm averaging, filtering comparison
const int numReadings = 5;
int rpmarray[numReadings];
int index = 0;                  // the index of the current reading
long total = 0;                  // the running total
long average = 0;                // the average


//These are stored memory variables for adjusting the (5) colors, activation rpm, shift rpm, brightness 
//Stored in EEPROM Memory 
int c1; 
int c2; 
int c3; 
int c4; 
int c5; 
int brightval;      //7-seg brightness 
int dimval;         //7-seg dim brightness
int sb;             //strip brightness
int dimsb;          // strip dim brightness
boolean dimmer = false;
boolean dimmerlogic = false;
int pixelanim=1;                                     
int smoothing;
int rpmscaler;
int shift_rpm1;                          // USED TO BE A LONG!!
int shift_rpm2;
int shift_rpm3;
int shift_rpm4;
int seg1_start = 1;
int seg1_end = 1;
int seg2_end = 2; 
int seg3_end = 3;
int activation_rpm1;
int activation_rpm2;
int activation_rpm3;
int activation_rpm4;
int a; 

int rst = 0; 
int cal; 

int prev_cal;

// COLOR VARIABLES - for use w/ the strips and translated into 255 RGB colors 
long color1; 
long color2; 
long color3; 
long flclr1; 


//Creates a 32 wide table for our pixel animations
int rpmtable[32][2];

// ROTARY ENCODER VARIABLES 
int button_pin = A3; 
int menuvar; 
int val; 
int rotaryval = 0; 


// CONFIGURATION FOR OLED DISPLAY
SSD1306AsciiSoftSpi oled;
#define OLED_DC    11
#define OLED_CS   12
#define OLED_CLK  10
#define OLED_DATA 9


// CONFIGURATION FOR THE ROTARY ENCODER 
  // Arduino pins the encoder is attached to. Attach the center to ground. 
  #define ROTARY_PIN1 A0
  #define ROTARY_PIN2 A1
  
  // Use the full-step state table (emits a code at 00 only) 
  const char ttable[7][4] = { 
  {0x0, 0x2, 0x4, 0x0}, {0x3, 0x0, 0x1, 0x40}, 
  {0x3, 0x2, 0x0, 0x0}, {0x3, 0x2, 0x1, 0x0}, 
  {0x6, 0x0, 0x4, 0x0}, {0x6, 0x5, 0x0, 0x80}, 
  {0x6, 0x5, 0x4, 0x0}, 
  }; 
  
 volatile unsigned char state = 0; 
  
char rotary_process() {
  char pinstate = (digitalRead(ROTARY_PIN2) << 1) | digitalRead(ROTARY_PIN1);
  state = ttable[state & 0xf][pinstate];
  return (state & 0xc0);
}



                              
                              /*************
                               * SETUP *
                              *************/

//SETUP TO CONFIGURE THE ARDUINO AND GET IT READY FOR FIRST RUN 
void setup() {
  
  //get stored variables 
  getEEPROM();
  check_first_run();
  
    if(DEBUG){
      Serial.begin(57600);
      Serial.println("ChipperNut ShiftLight Project. V2.");
      Serial.println("Prepare for awesome......");
    }
  
  
    for (int thisReading = 0; thisReading < numReadings; thisReading++){
      rpmarray[thisReading] = 0;  
    }
  
  timeoutCounter = timeoutValue;
  buildarrays();
  
  strip.begin(); 
  strip.clear();
  strip.show(); // Initialize all pixels to 'off' 
  
  
  //ROTARY ENCODER 
  pinMode(rpmPin, INPUT);
  pinMode(dimPin, INPUT_PULLUP);
  pinMode(button_pin, INPUT_PULLUP); 
  pinMode(ROTARY_PIN1, INPUT_PULLUP); 
  pinMode(ROTARY_PIN2, INPUT_PULLUP); 
  
  attachInterrupt(0, sensorIsr, RISING); 
  
  loadallcolors();
  
  oled.begin(&Adafruit128x64, OLED_CS, OLED_DC, OLED_CLK, OLED_DATA);
  oled.clear();
  oled.setFont(chippernut);
  oled.print("c");

  bootanimation();
  delay(1000);
  oled.clear();
  
  if(DEBUG){Serial.println("LOADED.");}

} 





                        /*************
                         * LOOP *
                        *************/
void loop() { 

   rpm = long(60e7/cal)/(float)interval;

     if (timeoutCounter > 0){ timeoutCounter--;}  
     if (timeoutCounter <= 0){rpm = 0;}

  if (((rpm > (rpm_last +(rpm_last*.2))) || (rpm < (rpm_last - (rpm_last*.2)))) && (rpm_last > 0) && (justfixed < 3)){
        rpm = rpm_last;
        justfixed++;
        if(DEBUG){
          Serial.print("FIXED!  ");
          Serial.println(rpm_last);}              
      
  } else {
    rpm_last = rpm;
    justfixed--;
    if (justfixed <= 0){justfixed = 0;}
  }

  
  if (smoothing){
    total = total - rpmarray[index]; 
    rpmarray[index] = rpm;
    total = total + rpmarray[index]; 
    index = index + 1;   
    if (index >= numReadings){              
        index = 0;    
     }                       
    average = total / numReadings; 
    if(DEBUG){Serial.print("average: ");
    Serial.println(average);}
    rpm = average;       
  }

  
  if (rpm > 0 ){
    if(DEBUG){Serial.println(rpm); }
    if(DEBUG){
      oled.set1X(); 
      oled.setFont(chippernutserial);
      oled.setCursor(78, 0);
      oled.print("c");
    }
    
    oled.set1X();  
    oled.setCursor(0, 0);
    oled.setFont(utf8font10x16);
    oled.print("RPM");  
    oled.set2X();    
    oled.setFont(lcdnums12x16);
    oled.setCursor(0, 2);
    if ((display_rpm>=10000 && rpm <10000) || 
        (display_rpm>=1000 && rpm <1000)){
          oled.clearToEOL();
         }  
    oled.print(rpm);
    display_rpm = rpm;  
  
      if (digitalRead(dimPin) != dimmer){
         dimmer = digitalRead(dimPin);
          loadallcolors(); 
  
          if (digitalRead(dimPin) == dimmerlogic){
            oled.set1X();  
            oled.setFont(chippernutdimmer);
            oled.setCursor(108, 0);
            oled.print("c");;                                              
           }else{
            oled.set1X();  
            oled.setFont(chippernutdimmer);
            oled.setCursor(108, 0);
            oled.clearToEOL();
          
           }
      }
  } else {
    rpm = 0;
    oled.clear();      
    strip.clear();
    strip.show();
  }


   if (rpm < shift_rpm){
    for (a = 0; a<NUMPIXELS; a++){
      if (rpm>rpmtable[a][0]){
          switch (rpmtable[a][1]){
            case 1:
              strip.setPixelColor(a,color1);
            break;
    
            case 2:
              strip.setPixelColor(a,color2);  
            break;
    
            case 3:
               strip.setPixelColor(a,color3);  
            break;        
          }
      } else {
    strip.setPixelColor(a, strip.Color(0, 0, 0));    
      } 
    }
  
  } else {
  
    unsigned long currentMillis = millis();
  
      if(currentMillis - previousMillis > shiftinterval) {
          previousMillis = currentMillis;   
          flashbool = !flashbool; 
  
          if (flashbool == true)
           for(int i=0; i<NUMPIXELS; i++) { 
              strip.setPixelColor(i, flclr1); 
           }
          else
           for(int i=0; i<NUMPIXELS; i++) { 
               strip.setPixelColor(i, 0,0,0); 
            }      
        }  
  }

  strip.show(); 

  
    //Poll the Button, if pushed, cue animation and enter menu subroutine 
    if (digitalRead(button_pin) == LOW){ 
      delay(250); 
      strip.clear(); 
      oled.clear();
      entermenu();  
      menuvar=1; 
      menu(); 
    } 
} 




void buildarrays(){
                   
int x;  //rpm increment
int y;  //starting point pixel address
int ya; // second starting point pixel address (for middle-out animation only)
int i;  //temporary for loop variable

if(DEBUG){
  Serial.print("NUMPIXELS:  ");
 Serial.println(NUMPIXELS);
 Serial.print("PIXELANIM:  ");
 Serial.println(pixelanim);
 Serial.print("Start1: ");
 Serial.println(seg1_start);
 Serial.print("End1: ");
 Serial.println(seg1_end);
 Serial.print("End2: ");
 Serial.println(seg2_end);
 Serial.print("End3: ");
 Serial.println(seg3_end);
 Serial.print("  Activation RPM ");
 Serial.println(activation_rpm);
 Serial.print("  SHIFT RPM ");
 Serial.println(shift_rpm);
}

  switch(pixelanim){

    case 1:        
      y=0;
      x = ((shift_rpm - activation_rpm)/NUMPIXELS);
      for (i = 0; i<seg1_end+1; i++){
        rpmtable[i][0] = activation_rpm + (i*x);
        rpmtable[i][1] = 1;
      }
       for (i = seg1_end+1; i<seg2_end+1; i++){
        rpmtable[i][0] = activation_rpm + (i*x);
        rpmtable[i][1] = 2;
      }
      for (i = seg2_end+1; i<seg3_end+1; i++){
        rpmtable[i][0] = activation_rpm + (i*x);
        rpmtable[i][1] = 3;
      }
    break;


    case 2:
     if (((NUMPIXELS-1)%2)> 0){    
         x = ((shift_rpm - activation_rpm)/(NUMPIXELS/2));  //EVEN PIXELS
     }else{
         x = ((shift_rpm - activation_rpm)/((NUMPIXELS/2)+1));   //ODD PIXELS       
     }
     

     ya = 0;   // SEGMENT 1
     for (i = seg1_start; i<seg1_end+1; i++){      
        rpmtable[i][0] = activation_rpm + (ya*x);
        rpmtable[i][1] = 1;
        ya++;
      }

          
         if (((NUMPIXELS-1)%2)> 0){
          ya = 0;
           for (i = seg1_start-1; i>seg1_start-(seg1_end-seg1_start)-2; i--){
              rpmtable[i][0] = activation_rpm + (ya*x);
              rpmtable[i][1] = 1;
              ya++;
            } 
         } else {
          ya = 1;
           for (i = seg1_start-1; i>seg1_start-(seg1_end-seg1_start)-1; i--){
              rpmtable[i][0] = activation_rpm + (ya*x);
              rpmtable[i][1] = 1;
              ya++;     
            }
         }



      if ((seg1_end+1) == seg2_end){
        ya =  seg2_end - seg1_start;  //SEGMENT 2
      } else {
        ya =  (seg1_end+1) - seg1_start;  
      }
      
     for (i = (seg1_end+1); i<seg2_end+1; i++){
        rpmtable[i][0] = activation_rpm + (ya*x);
        rpmtable[i][1] = 2;
        ya++;
      }
      
      if ((seg1_end+1) == seg2_end){
        ya =  seg2_end - seg1_start;  //SEGMENT 2
      } else {
        ya =  (seg1_end+1) - seg1_start;  
      }
      
         if (((NUMPIXELS-1)%2)> 0){
           for (i = seg1_start-(seg1_end-seg1_start)-2; i>seg1_start-(seg2_end-seg1_start)-2; i--){
              rpmtable[i][0] = activation_rpm + (ya*x);
              rpmtable[i][1] = 2;
              ya++;
            }
          } else {
            for (i = seg1_start-(seg1_end-seg1_start)-1; i>seg1_start-(seg2_end-seg1_start)-1; i--){
              rpmtable[i][0] = activation_rpm + (ya*x);
              rpmtable[i][1] = 2;
              ya++;
             }
           }
           
      if ((seg2_end+1) == seg3_end){
         ya =  seg3_end - seg1_start;    //SEGMENT 3
      } else {
         ya =  (seg2_end+1) - seg1_start;    //SEGMENT 3  
      }
      
      
     for (i = (seg2_end+1); i<seg3_end+1; i++){
        rpmtable[i][0] = activation_rpm + (ya*x);
        rpmtable[i][1] = 3;
        ya++;
      }

      if ((seg2_end+1) == seg3_end){
         ya =  seg3_end - seg1_start;   
      } else {
         ya =  (seg2_end+1) - seg1_start;     
      }
      
      if (((NUMPIXELS-1)%2)> 0){
          for (i = seg1_start-(seg2_end-seg1_start)-2; i>seg1_start-(seg3_end-seg1_start)-2; i--){
             rpmtable[i][0] = activation_rpm + (ya*x);
             rpmtable[i][1] = 3;
             ya++;
           }
      } else {
          for (i = seg1_start-(seg2_end-seg1_start)-1; i>seg1_start-(seg3_end-seg1_start)-1; i--){
              rpmtable[i][0] = activation_rpm + (ya*x);
              rpmtable[i][1] = 3;
              ya++;
          }        
      }
    break;
    
  case 3:        
        y=0;
        x = ((shift_rpm - activation_rpm)/NUMPIXELS);
        for (i = NUMPIXELS-1; i>seg1_end-1; i--){
          rpmtable[i][0] = activation_rpm + (y*x);
          rpmtable[i][1] = 1;
          y++;
        }
         for (i = seg1_end-1; i>seg2_end-1; i--){
          rpmtable[i][0] = activation_rpm + (y*x);
          rpmtable[i][1] = 2;
          y++;
        }
        for (i = seg2_end-1; i>seg3_end-1; i--){
          rpmtable[i][0] = activation_rpm + (y*x);
          rpmtable[i][1] = 3;
          y++;
        }
      break;

case 4:                                           
     if (((NUMPIXELS-1)%2)> 0){    
         x = ((shift_rpm - activation_rpm)/(NUMPIXELS/2));  //EVEN PIXELS
     }else{
       x = ((shift_rpm - activation_rpm)/((NUMPIXELS/2)+1));   //ODD PIXELS       
     }
     

      // SEGMENT 1
     for (i = seg1_start; i<seg1_end+1; i++){      
        rpmtable[i][0] = activation_rpm + (i*x);
        rpmtable[i][1] = 1;
         }
          
          ya = 0;
           for (i = NUMPIXELS-1; i>NUMPIXELS - (seg1_end) - 2; i--){
              rpmtable[i][0] = activation_rpm + (ya*x);
              rpmtable[i][1] = 1;
              ya++;
            } 

    // SEGMENT 2
     for (i = (seg1_end+1); i<seg2_end+1; i++){
        rpmtable[i][0] = activation_rpm + (i*x);
        rpmtable[i][1] = 2;
      }
      

      ya = seg1_end+1; 
      for (i = NUMPIXELS-(seg1_end)-2; i>NUMPIXELS-seg2_end-2; i--){
           rpmtable[i][0] = activation_rpm + (ya*x);
           rpmtable[i][1] = 2;
           ya++;
          }
 
    // SEGMENT 3
     for (i = (seg2_end+1); i<seg3_end+1; i++){
        rpmtable[i][0] = activation_rpm + (i*x);
        rpmtable[i][1] = 3;
      }

      ya = seg2_end+1;
      for (i = NUMPIXELS-(seg2_end)-2; i>NUMPIXELS-seg3_end-2; i--){
           rpmtable[i][0] = activation_rpm + (ya*x);
           rpmtable[i][1] = 3;
           ya++;
          }

    break;
   
  }

  

if(DEBUG){
  for (i = 0; i<NUMPIXELS; i++){
    Serial.print(rpmtable[i][0]);
    Serial.print("  ");
    Serial.println(rpmtable[i][1]);
  }
}
}







/*************************
 * MENU SYSTEM
 *************************/

// MENU SYSTEM 
void menu(){ 
prev_menu = 2;
//this keeps us in the menu 
while (menuvar == 1){   
  
  // This little bit calls the rotary encoder   
  int result = rotary_process(); 
  if(DEBUG){if(result!=0){Serial.println(result);}}
  if (result == -128){rotaryval--;}   
  else if (result == 64){rotaryval++;} 
 
  rotaryval = constrain(rotaryval, 0, 15); 

//Poll the rotary encoder button to enter menu items
     if (digitalRead(button_pin) == LOW){
        oled.clear(); 
        delay(250); 
        menu_enter = 1;         
      } 
      
      if (prev_menu != rotaryval || menu_enter == 1 || menu_enter == 2){      
          prev_menu = rotaryval;
          if (menu_enter == 2){menu_enter = 0;}
       
          oled.clear();
          oled.set1X();  
          oled.setFont(utf8font10x16);
      
        switch (rotaryval){ 
        
          case 0: //Menu Screen. Exiting saves variables to EEPROM 
        
          oled.setCursor(50, 0); 
          oled.print(F("MENU"));
          oled.setCursor(0, 2);              
          oled.print(F("Press to Save & Exit."));
          oled.setCursor(0, 4);              
          oled.print(F(" "));
          oled.setCursor(0, 6);              
          oled.print(F("Rotate to browse menu"));

            //Poll the Button to exit 
            if (menu_enter == 1){ 
              oled.clear();
              delay(250); 
              rotaryval = 0; 
              menuvar=0;
              menu_enter = 0;              
              writeEEPROM(); 
              getEEPROM(); 
              buildarrays();
              loadallcolors();
              entermenu();  
            }     
          break; 
          
          
          case 1: //Adjust the global brightness 
         
            if (menu_enter == 0){
              oled.setCursor(30, 0); 
              oled.print(F("BRIGHTNESS"));
              oled.setCursor(0, 2);              
              oled.print(F("Adjust the global"));
              oled.setCursor(0, 4);              
              oled.print(F("brightness level"));
              oled.setCursor(0, 6);              
              oled.print(F("")); 
            }
            
            while (menu_enter == 1){ 
            
            int bright = rotary_process();            
            if (bright == -128){ 
              sb++;
              testbright = false;
            } 
            if (bright == 64){ 
              sb--;
              testbright = false;
            } 

             sb = constrain(sb, 1, 15); 
      
            if (prev_variable != sb){
                  prev_variable = sb;
                  loadallcolors();                   
                  oled.set1X(); 
                  oled.setFont(utf8font10x16);
                  oled.setCursor(30, 0); 
                  oled.print(F("BRIGHTNESS"));
                  oled.set2X();                   
                  oled.setFont(lcdnums12x16);
                  oled.setCursor(0, 2);
                  oled.clearToEOL();
                  oled.print(16-sb);                 
                  
                  if (testbright == false){
                  testlights();
                  testbright = true;
                  }
              }
      
      
           if (digitalRead(button_pin) == LOW){ 
              delay(250); 
              menu_enter = 2;
              prev_variable = 0;
              exitmenu();        
            }      
            }     
          break; 

      
         //dimPin
          case 2: //Adjust the global brightness DIMMER (when Pin 9 == HIGH)

            if (menu_enter == 0){
              oled.setCursor(40, 0); 
              oled.print(F("DIMMER"));
              oled.setCursor(0, 2);              
              oled.print(F("Set dim brightness and"));
              oled.setCursor(0, 4);              
              oled.print(F("logic of the dimmer"));
              oled.setCursor(0, 6);              
              oled.print(F("wire"));
            }
               
            while (menu_enter == 1){ 
            
            int bright = rotary_process(); 
            
            if (bright == -128){ 
              dimsb++;
              testbright = false;
            } 
            if (bright == 64){ 
              dimsb--;
              testbright = false;
            } 

            dimval  = map(dimsb,1,15,15,8);          
            dimval = constrain (dimval, 8, 15); 
            dimsb = constrain(dimsb, 1, 15); 
      
            if (prev_variable != dimsb){
                  prev_variable = dimsb;
                  testdim = true;
                  loadallcolors();
                  oled.set1X(); 
                  oled.setFont(utf8font10x16);
                  oled.setCursor(40, 0); 
                  oled.print(F("DIMMER"));
                  oled.set2X();                   
                  oled.setFont(lcdnums12x16);
                  oled.setCursor(0, 2);
                  oled.clearToEOL();
                  oled.print(16-dimsb);
               
                  if (testbright == false){
                  testlights();
                  testbright = true;
                  }
              }
      
      
           if (digitalRead(button_pin) == LOW){ 
              delay(250); 
              menu_enter = 4;
              oled.clear();     
            }      
            }
 while (menu_enter == 4){ 
            
            int bright = rotary_process(); 
            
            if (bright == -128){ 
              dimmerlogic = false;
              testbright = false;
            } 
            if (bright == 64){ 
              dimmerlogic = true;
              testbright = false;
            } 
      
            if (prev_variable != dimmerlogic){
                  prev_variable = dimmerlogic;
                  
                if (dimmerlogic == false){
                  oled.set1X(); 
                  oled.setFont(utf8font10x16);
                  oled.setCursor(30, 0); 
                  oled.print(F("DIMMER LOGIC"));
                  oled.set2X();                   
                  oled.setFont(utf8font10x16);
                  oled.setCursor(0, 2);
                  oled.clearToEOL();
                  oled.print(F("HIGH"));
                                      
               } else if (dimmerlogic == true){  
                  
                  oled.set1X(); 
                  oled.setFont(utf8font10x16);
                  oled.setCursor(30, 0); 
                  oled.print(F("DIMMER LOGIC"));
                  oled.set2X();                   
                  oled.setFont(utf8font10x16);
                  oled.setCursor(0, 2);
                  oled.clearToEOL();
                  oled.print(F("LOW"));                      
                }
          
                  if (testbright == false){
                  testlights();
                  testbright = true;
                  }
              }
 
           
                    if (digitalRead(dimPin) == dimmerlogic){
                      oled.set1X();  
                      oled.setFont(chippernutdimmer);
                      oled.setCursor(108, 0);
                      oled.print("c");;                                               // ADd in Flashlight or headlight bulb Icon HERE
                     }else{
                      oled.set1X();  
                      oled.setFont(chippernutdimmer);
                      oled.setCursor(108, 0);
                      oled.clearToEOL();                    
                     }
      
      
           if (digitalRead(button_pin) == LOW){ 
              delay(250); 
              menu_enter = 2;
              prev_variable = 0;
              testdim = false;
              exitmenu();        
            }      
            }
          break; 

          
          case 3: // ACTIVATION RPM 
            if (menu_enter == 0){
              oled.setCursor(25, 0); 
              oled.print(F("ACTIVATION RPM"));
              oled.setCursor(0, 2);              
              oled.print(F("Select the RPM"));
              oled.setCursor(0, 4);              
              oled.print(F("value for the start"));
              oled.setCursor(0, 6);              
              oled.print(F("of the LED graph."));
            }
      
            while (menu_enter == 1){      
              int coloradjust1 = rotary_process();           
              if (coloradjust1 == -128){activation_rpm=activation_rpm-10;} 
              if (coloradjust1 == 64){activation_rpm=activation_rpm+10;}         
              activation_rpm = constrain(activation_rpm, 0, 20000); 
              
              if (prev_variable != activation_rpm) {                

                  oled.set1X(); 
                  oled.setFont(utf8font10x16);
                  oled.setCursor(25, 0); 
                  oled.print(F("ACTIVATION RPM"));
                  oled.set2X();                   
                  oled.setFont(lcdnums12x16);
                  oled.setCursor(0, 2);
                  if ((prev_variable>=10000 && activation_rpm <10000) || 
                      (prev_variable>=1000 && activation_rpm <1000)){
                      oled.clearToEOL();
                  }
                  oled.print(activation_rpm);
                  prev_variable = activation_rpm;
              }   
              
              if (digitalRead(button_pin) == LOW){ 
                delay(250);
                prev_variable = 0;
                menu_enter = 2; 
                exitmenu();        
              } 
            }   
          break; 
          
          
          case 4: // SHIFT RPM 

            if (menu_enter == 0){
              oled.setCursor(35, 0); 
              oled.print(F("SHIFT RPM"));
              oled.setCursor(0, 2);              
              oled.print(F("Select the RPM"));
              oled.setCursor(0, 4);              
              oled.print(F("that flashes the"));
              oled.setCursor(0, 6);              
              oled.print(F("LED graph.")); 
            }


            
            while (menu_enter == 1){                     
              int coloradjust1 = rotary_process();               
              if (coloradjust1 == -128){shift_rpm = shift_rpm-10;}           
              if (coloradjust1 == 64){shift_rpm = shift_rpm+10;}                   
              shift_rpm = constrain(shift_rpm, 0, 20000); 
      
              if (prev_variable != shift_rpm) {
                  oled.set1X(); 
                  oled.setFont(utf8font10x16);
                  oled.setCursor(35, 0); 
                  oled.print(F("SHIFT RPM"));
                  oled.set2X();                   
                  oled.setFont(lcdnums12x16);
                  oled.setCursor(0, 2);
                  if ((prev_variable>=10000 && shift_rpm <10000) || 
                      (prev_variable>=1000 && shift_rpm <1000)){
                      oled.clearToEOL();
                  }
                  oled.print(shift_rpm);
                  prev_variable = shift_rpm;
              }
              
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                menu_enter = 2; 
                prev_variable = 0;
                exitmenu();        
              } 
            }    
          break;
      
        
           case 5:  //SMOOTHING (conditioning)
            if (menu_enter == 0){
              oled.setCursor(30, 0); 
              oled.print(F("SMOOTHING"));
              oled.setCursor(0, 2);              
              oled.print(F("Select ON if you get"));
              oled.setCursor(0, 4);              
              oled.print(F("erratic or jumpy"));
              oled.setCursor(0, 6);              
              oled.print(F("RPM values."));
              prev_variable = -1;
            }
      
            while (menu_enter == 1){ 
              
              int coloradjust1 = rotary_process();         
              if (coloradjust1 == -128){smoothing--;}           
              if (coloradjust1 == 64){smoothing++;}                   
              smoothing = constrain(smoothing, 0, 1); 
      
          if (prev_variable != smoothing) {
                   prev_variable = smoothing;
            if (smoothing){
                  oled.clear();
                  oled.set1X(); 
                  oled.setFont(utf8font10x16);
                  oled.setCursor(30, 0); 
                  oled.print(F("SMOOTHING"));
                  oled.set2X();                   
                  oled.setFont(utf8font10x16);
                  oled.setCursor(0, 2);
                  oled.clearToEOL();
                  oled.print(F("ON"));             
            }else{     
                  oled.clear();
                  oled.set1X(); 
                  oled.setFont(utf8font10x16);
                  oled.setCursor(30, 0); 
                  oled.print(F("SMOOTHING"));
                  oled.set2X();                   
                  oled.setFont(utf8font10x16);
                  oled.setCursor(0, 2);
                  oled.clearToEOL();
                  oled.print(F("OFF"));
             }
             
          }      
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                menu_enter = 2; 
                prev_variable = 255;                 
                exitmenu();             
              } 
            }    
           break;
      
      
       case 6:  // PULSES PER REVOLUTION
        if (menu_enter == 0){
            oled.setCursor(0, 0); 
            oled.print(F("PULSES PER ROTATION"));
            oled.setCursor(0, 2);              
            oled.print(F("Select your engine size"));
            oled.setCursor(0, 4);              
            oled.print(F("2.0 = 4cyl  3.0 = 6cyl"));
            oled.setCursor(0, 6);              
            oled.print(F("4.0 = 8cyl")); 
        }
            while (menu_enter == 1){               
              int coloradjust1 = rotary_process();         
              if (coloradjust1 == -128){cal--;}           
              if (coloradjust1 == 64){cal++;}                   
              cal = constrain(cal, 1, 255); 
      
              if (prev_cal != cal){
                  oled.set1X(); 
                  oled.setFont(utf8font10x16);
                  oled.setCursor(0, 0); 
                  oled.print(F("PULSES PER ROTATION"));
                  oled.set2X();                   
                  oled.setFont(lcdnums12x16);
                  oled.setCursor(0, 2);
                  oled.print((cal/10));
                  oled.setCursor(25, 2);
                  oled.print(F("."));
                  oled.setCursor(45, 2);
                  oled.print((cal%10));
                  prev_variable = shift_rpm;
                  prev_cal = cal;
              }
       
                  rpm = long(60e7/cal)/(float)interval;

                  oled.set1X();    
                  oled.setFont(lcdnums12x16);
                  oled.setCursor(0, 6);
                  if ((display_rpm>=10000 && rpm <10000) || 
                      (display_rpm>=1000 && rpm <1000)){
                        oled.clearToEOL();
                       }  
                  oled.print(rpm);
                  display_rpm = rpm;
                  oled.set1X();    
                  oled.setFont(utf8font10x16);
                  oled.setCursor(65, 6);
                  oled.print(F("RPM")); 
                 
             if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                menu_enter = 2;
                prev_cal = 0;
//                display.setColon(false);
                exitmenu();
              } 
            }    
           break;
      
     
           case 7:  // NUMBER OF LEDS

              if (menu_enter == 0){
                oled.setCursor(20, 0); 
                oled.print(F("NUMBER OF LEDS"));
                oled.setCursor(0, 2);              
                oled.print(F("Set the number of LEDS"));
                oled.setCursor(0, 4);              
                oled.print(F("in the light strip"));
                oled.setCursor(0, 6);              
                oled.print(F(""));
              }
       
            
            while (menu_enter == 1){ 
              
              int coloradjust1 = rotary_process();         
              if (coloradjust1 == -128){NUMPIXELS--;}           
              if (coloradjust1 == 64){NUMPIXELS++;}                   
              NUMPIXELS = constrain(NUMPIXELS, 0, 32); 
      
              if (prev_variable != NUMPIXELS) {
                  if ((NUMPIXELS < 10) && prev_variable >=10){
                    oled.set2X();
                    oled.setFont(lcdnums12x16);
                    oled.setCursor(0, 2);
                    oled.clearToEOL(); 
                  }
                
                  prev_variable = NUMPIXELS;
                  oled.set1X(); 
                  oled.setFont(utf8font10x16);
                  oled.setCursor(20, 0); 
                  oled.print(F("NUMBER OF LEDS"));
                  oled.set2X();                   
                  oled.setFont(lcdnums12x16);
                  oled.setCursor(0, 2);       
                  oled.print(NUMPIXELS);
                  
              }
             if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                menu_enter = 2;
                prev_variable = 0;
                exitmenu();
      
                  oled.set1X(); 
                  oled.setFont(utf8font10x16);
                  oled.setCursor(0, 0);
                  oled.clearToEOL(); 
                  oled.print(F("REBOOT REQUIRED"));
                  oled.set2X();                   
                  oled.setFont(lcdnums12x16);
                  oled.setCursor(0, 2);
                  oled.clearToEOL();
                  for (int x=3; x>-1; x--){
                    oled.clearToEOL();
                    oled.setCursor(55, 2);       
                    oled.print(x);
                    delay(1000);
                  }

             writeEEPROM(); 
             resetFunc();
              } 
            }    
           break;    
          
       
           case 8:  // Color Segmentation   

          if (menu_enter == 0){
            oled.setCursor(0, 0);
            oled.print(F("COLOR SEGMENTS"));
            oled.setCursor(0, 2);   
            oled.setFont(utf8font10x16);        
            oled.print(F("Sets the width of the"));
            oled.setCursor(0, 4); 
            oled.print(F("colors across the LED"));
            oled.setCursor(0, 6); 
            oled.print(F("strip."));
          }           
            
            if (menu_enter == 1){
                loadallcolors();
                build_segments();
                menu_enter = 2;
                current_seg_number = 1;
                seg_mover = 0;
                buildarrays();
                exitmenu();
             }
      break;    
      
      
           case 9:  // PIXEL ANIMATION MODE

          if (menu_enter == 0){
            oled.setCursor(0, 0);
            oled.print(F("ANIMATION MODE"));
            oled.setCursor(0, 2);   
            oled.print(F("Choose the animation"));
            oled.setCursor(0, 4); 
            oled.print(F("style for the LED strip"));
            oled.setCursor(0, 6); 
            oled.print("");
            prev_animation = 0;
          }

          
            while (menu_enter == 1){        
              int coloradjust1 = rotary_process();         
              if (coloradjust1 == -128){pixelanim--;} 
              if (coloradjust1 == 64){pixelanim++;}         
              pixelanim = constrain(pixelanim, 1, 4);        
       
                if (prev_animation != pixelanim){
                 if(DEBUG){ Serial.println("Animation Change");}
                 oled.setCursor(0, 0);
                 oled.print(F("ANIMATION MODE"));
            
                prev_animation = pixelanim;
                loadallcolors();
                
                if (pixelanim == 1){
                  oled.setCursor(0, 2);
                  oled.clearToEOL();  
                  oled.print(F("LEFT TO RIGHT"));                           
                  for (int a = 0; a<NUMPIXELS; a++){
                    strip.setPixelColor(a,color1);
                    strip.show();
                    delay(50);
                  }
                  
                }else if(pixelanim == 2){
                  oled.setCursor(0, 2);
                  oled.clearToEOL();  
                  oled.print(F("CENTER TO OUT"));                  
                  for (int a = NUMPIXELS/2; a<NUMPIXELS; a++){
                    strip.setPixelColor(a,color1);
                    strip.setPixelColor(NUMPIXELS-a,color1);
                    strip.show();
                    delay(75);
                  }
                   
                } else if(pixelanim == 3) {
                  oled.setCursor(0, 2);
                  oled.clearToEOL();  
                  oled.print(F("RIGHT TO LEFT"));                  
                  for (int a = NUMPIXELS; a>-1; a--){
                    strip.setPixelColor(a,color1);
                    strip.show();
                    delay(50);
                  }
                } else if(pixelanim == 4){
                  oled.setCursor(0, 2);
                  oled.clearToEOL();  
                  oled.print(F("OUT TO CENTER"));
                  for (int a = NUMPIXELS; a>(NUMPIXELS/2)-1; a--){
                    strip.setPixelColor(a,color1);
                    strip.setPixelColor(NUMPIXELS-a,color1);
                    strip.show();
                    delay(75);
                  }
                  
                }
                  oled.setCursor(0, 6);
                  oled.clearToEOL();  
                  oled.print(F("PUSH TO SAVE"));
                  strip.clear();
                }
      
              
              if (digitalRead(button_pin) == LOW){           
                delay(250);
                menu_enter = 2;
                oled.clear();
                oled.setCursor(0, 2);   
                oled.print(F("YOU MUST REDO"));
                oled.setCursor(0, 4); 
                oled.print(F("SEGMENTS. PLEASE WAIT"));                
                oled.print("");
                
             delay(2000);
             oled.clear();
             build_segments();
             loadallcolors();
             exitmenu();
           }        
          } 
         break;  
      
              
          
          case 10: //Adjust Color #1 

            oled.setCursor(0, 0);
            oled.print(F("SET COLOR 1"));
            oled.setCursor(0, 2);         
            oled.print(F("Set the color of the"));
            oled.setCursor(0, 4); 
            oled.print(F("first LED segment."));
            oled.setCursor(0, 6); 
            oled.print("");

           while (menu_enter == 1){ 
              
              int coloradjust1 = rotary_process();         
              if (coloradjust1 == -128){c1--;} 
              if (coloradjust1 == 64){c1++;}         
              c1 = constrain(c1, 0, 255); 
      
              if (prev_color != c1){
                prev_color = c1;
                color1 = load_color(c1); 
                testlights();
                //testlights(1);
              }
            
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                prev_color = 0;
                menu_enter = 2; 
                exitmenu();        
              } 
            } 
          break; 
             
          
          
          case 11: //Adjust Color #2 
      
            oled.setCursor(0, 0);
            oled.print(F("SET COLOR 2"));
            oled.setCursor(0, 2);
            oled.print(F("Set the color of the"));
            oled.setCursor(0, 4); 
            oled.print(F("second LED segment."));
            oled.setCursor(0, 6); 
            oled.print("");

                 
            while (menu_enter == 1){ 
            
              int coloradjust1 = rotary_process(); 
              
              if (coloradjust1 == -128){c2--;} 
              if (coloradjust1 == 64){c2++;}         
              c2 = constrain(c2, 0, 255); 
              
              if (prev_color != c2){
                prev_color = c2;
                color2 = load_color(c2); 
                testlights();
                //testlights(2);
              }
              
              
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                prev_color = 0;
                menu_enter = 2; 
                exitmenu();          
              } 
            }    
          break; 
          
          case 12: //Adjust Color #3 

            oled.setCursor(0, 0);
            oled.print(F("SET COLOR 3"));
            oled.setCursor(0, 2);          
            oled.print(F("Set the color of the"));
            oled.setCursor(0, 4); 
            oled.print(F("third LED segment."));
            oled.setCursor(0, 6); 
            oled.print("");

            while (menu_enter == 1){       
              int coloradjust1 = rotary_process();         
              if (coloradjust1 == -128){c3--;} 
              if (coloradjust1 == 64){c3++;}        
              c3 = constrain(c3, 0, 255); 
              
               if (prev_color != c3){
                prev_color = c3;
                color3 = load_color(c3); 
                testlights();
              }
              
              
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                prev_color = 0;
                menu_enter = 2; 
                exitmenu();       
              } 
            }    
          break; 
          
          case 13: //Adjust Shift Color 
    
            oled.setCursor(0, 0);
            oled.print(F("SHIFT COLOR"));
            oled.setCursor(0, 2);          
            oled.print(F("Set the color of the"));
            oled.setCursor(0, 4); 
            oled.print(F("shift flash."));
            oled.setCursor(0, 6); 
            oled.print(F(""));

            while (menu_enter == 1){       
              int coloradjust1 = rotary_process();         
              if (coloradjust1 == -128){c4--;} 
              if (coloradjust1 == 64){c4++;} 
              
              c4 = constrain(c4, 0, 255); 
              
              flclr1 = load_color(c4);        
              
              for(int i=0; i<NUMPIXELS+1; i++) { 
                strip.setPixelColor(i, flclr1); 
              } 
              
              strip.show(); 
                     
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                menu_enter = 2; 
                 exitmenu();        
              } 
            }    
          break; 
          
 
          case 14:   //DEBUG MODE
          
            oled.setCursor(0, 0);
            oled.print(F("DEBUG MODE"));
            oled.setCursor(0, 2);       
            oled.print(F("Turn ON to enable"));
            oled.setCursor(0, 4); 
            oled.print(F("serial output via USB."));
            oled.setCursor(0, 6); 
            oled.print(F("57600 Baud"));

              
       while (menu_enter == 1){       
              int coloradjust1 = rotary_process();        
              if (coloradjust1 == -128){DEBUG--;} 
              if (coloradjust1 == 64){DEBUG++;}         
              DEBUG = constrain(DEBUG, 0, 1); 
               
            if (DEBUG== 1){
                oled.set1X(); 
                oled.setFont(chippernutserial);
                oled.setCursor(78, 0);
                oled.print("c"); 
            }else{
                oled.set1X();  
                oled.setFont(chippernutserial);
                oled.setCursor(78, 0);
                oled.clearToEOL();              
            }
      
      
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                menu_enter = 2; 
                strip.clear(); 
                strip.show(); 
                exitmenu();        
              } 
            }  
          break;
    
      
          case 15:   //RESET
      
            oled.setCursor(0, 0);   
            oled.print(F("SYSTEM RESET"));
            oled.setCursor(0, 2);                   
            oled.print(F("Restores default settings."));
            
                 
       while (menu_enter == 1){       
              int coloradjust1 = rotary_process();        
              if (coloradjust1 == -128){rst--;} 
              if (coloradjust1 == 64){rst++;}         
              rst = constrain(rst, 0, 1); 
              oled.setCursor(0, 4); 
              oled.print(F("Are you sure?"));
              
            if (rst == 1){      
              oled.setCursor(0, 6);
              oled.print(F("YES  "));              
            }else{
              oled.setCursor(0, 6);
              oled.print(F("NO  "));   
            }
      
                      
              if (digitalRead(button_pin) == LOW){ 
                delay(250); 
                menu_enter = 2; 
                strip.clear(); 
                strip.show(); 
                exitmenu();
                  if (rst ==1){
                    oled.clear();
                    oled.set1X();  
                    oled.setFont(utf8font10x16);
                    oled.setCursor(0, 4); 
                    oled.print(F("Deleting EEPROM memory block:"));

                     for (int i = 0; i < 512; i++){
                      EEPROM.write(i, 0);
                      oled.setCursor(0, 6);
                      oled.print(i);
                      delay(1); 
                      }
                     resetFunc();
                  
                  }       
              } 
            }  
          break;
                
          }
      } 
  } 
} 






/*************************
 * SUBROUTINES
 *************************/


//This subroutine reads the stored variables from memory 
void getEEPROM(){ 
brightval = EEPROM.read(0); 
sb = EEPROM.read(1); 
c1 = EEPROM.read(2); 
c2 = EEPROM.read(3); 
c3 = EEPROM.read(4); 
c4 = EEPROM.read(5); 
c5 = EEPROM.read(6); 
activation_rpm = EEPROM.read(7); 
pixelanim  = EEPROM.read(8); 
//senseoption  = EEPROM.read(9); 
smoothing = EEPROM.read(10); 
NUMPIXELS = EEPROM.read(11); 
rpmscaler = EEPROM.read(12); 
shift_rpm1 = EEPROM.read(13); 
shift_rpm2 = EEPROM.read(14); 
shift_rpm3 = EEPROM.read(15); 
shift_rpm4 = EEPROM.read(16); 
DEBUG = EEPROM.read(17); 
seg1_start = EEPROM.read(18); 
seg1_end = EEPROM.read(19); 
//seg2_start = EEPROM.read(20); 
seg2_end = EEPROM.read(21); 
//seg3_start = EEPROM.read(22); 
seg3_end = EEPROM.read(23); 
activation_rpm1 = EEPROM.read(24); 
activation_rpm2 = EEPROM.read(25); 
activation_rpm3 = EEPROM.read(26); 
activation_rpm4 = EEPROM.read(27); 
cal = EEPROM.read(28);
dimval = EEPROM.read(29);
dimsb = EEPROM.read(30);
dimmerlogic = EEPROM.read(31);

activation_rpm = ((activation_rpm1 << 0) & 0xFF) + ((activation_rpm2 << 8) & 0xFFFF) + ((activation_rpm3 << 16) & 0xFFFFFF) + ((activation_rpm4 << 24) & 0xFFFFFFFF);
shift_rpm = ((shift_rpm1 << 0) & 0xFF) + ((shift_rpm2 << 8) & 0xFFFF) + ((shift_rpm3 << 16) & 0xFFFFFF) + ((shift_rpm4 << 24) & 0xFFFFFFFF);

buildarrays();

} 


//This subroutine writes the stored variables to memory 
void writeEEPROM(){ 

byte four = (shift_rpm & 0xFF);
byte three = ((shift_rpm >> 8) & 0xFF);
byte two = ((shift_rpm >> 16) & 0xFF);
byte one = ((shift_rpm >> 24) & 0xFF);

byte activation_four = (activation_rpm & 0xFF);
byte activation_three = ((activation_rpm >> 8) & 0xFF);
byte activation_two = ((activation_rpm >> 16) & 0xFF);
byte activation_one = ((activation_rpm >> 24) & 0xFF);

EEPROM.write(0, brightval); 
EEPROM.write(1, sb); 
EEPROM.write(2, c1); 
EEPROM.write(3, c2); 
EEPROM.write(4, c3); 
EEPROM.write(5, c4); 
EEPROM.write(6, c5); 
EEPROM.write(7, activation_rpm); 
EEPROM.write(8, pixelanim); 
//EEPROM.write(9, senseoption); 
EEPROM.write(10, smoothing); 
EEPROM.write(11, NUMPIXELS); 
EEPROM.write(12, rpmscaler); 
EEPROM.write(13, four); 
EEPROM.write(14, three); 
EEPROM.write(15, two); 
EEPROM.write(16, one); 
EEPROM.write(17, DEBUG); 
EEPROM.write(18, seg1_start); 
EEPROM.write(19, seg1_end); 
//EEPROM.write(20, seg2_start); 
EEPROM.write(21, seg2_end); 
//EEPROM.write(22, seg3_start); 
EEPROM.write(23, seg3_end); 
EEPROM.write(24, activation_four); 
EEPROM.write(25, activation_three); 
EEPROM.write(26, activation_two); 
EEPROM.write(27, activation_one); 
EEPROM.write(28, cal);
EEPROM.write(29, dimval);
EEPROM.write(30, dimsb);
EEPROM.write(31, dimmerlogic);
} 




void loadallcolors(){
  color1 = load_color(c1); 
  color2 = load_color(c2); 
  color3 = load_color(c3); 
  flclr1 = load_color(c4);   
}



//Helper Color Manager - This translates our 255 value into a meaningful color
uint32_t load_color(int cx){ 
unsigned int r,g,b; 
if (cx == 0){ 
    r = 0; 
    g = 0; 
    b = 0; 
} 

if (cx>0 && cx<=85){ 
  r = 255-(cx*3); 
  g = cx*3; 
  b=0; 
} 

if (cx>85 && cx < 170){ 
  r = 0; 
  g = 255 - ((cx-85)*3); 
  b = (cx-85)*3; 
} 

if (cx >= 170 && cx<255){ 
  r = (cx-170)*3; 
  g = 0; 
  b = 255 - ((cx-170)*3); 
} 

if (cx == 255){ 
  r=255; 
  g=255; 
  b=255; 
} 



if (digitalRead(dimPin)== dimmerlogic || testdim == true){
  r = (r/dimsb); 
  g = (g/dimsb); 
  b = (b/dimsb);   
} else {
  r = (r/sb); 
  g = (g/sb); 
  b = (b/sb); 
}

return strip.Color(r,g,b); 
}



void testlights(){
  for (int a = 0; a<NUMPIXELS; a++){    
     switch (rpmtable[a][1]){
          case 1:
            strip.setPixelColor(a,color1);
          break;
  
          case 2:
            strip.setPixelColor(a,color2);  
          break;
  
          case 3:
             strip.setPixelColor(a,color3);  
          break; 
    }
      
  }
  strip.show(); 
}





void check_first_run(){

  if (shift_rpm == 0){
     Serial.println("FIRST RUN! LOADING DEFAULTS");  
      brightval = 15; 
      dimval = 8;
      dimsb = 15;
      dimmerlogic = false;
      sb = 3; 
      c1 = 79; 
      c2 = 48; 
      c3 = 1; 
      c4 = 255; 
      c5 = 0; 
      
      activation_rpm = 1000; 
      shift_rpm = 6000;
      pixelanim  = 1; 
      smoothing = 0; 
      NUMPIXELS = 16;
      //rpmscaler = EEPROM.read(12);  
      DEBUG = 1; 
      seg1_start = 0; 
      seg1_end = 10; 
      seg2_end = 13; 
      seg3_end = 15;
      cal = 30;
      writeEEPROM();
      resetFunc();
  }  
}

        
void build_segments(){

              oled.setCursor(0, 0);
              oled.print(F("COLOR SEGMENTS"));
              oled.setCursor(0, 2);   
              oled.setFont(utf8font10x16);        
              oled.print(F("Rotate the knob"));
              oled.setCursor(0, 4); 
              oled.print(F("to set color width."));
              oled.setCursor(0, 6); 
              oled.print(F("Press when done."));

// Resets segmentation variables, sets segments 2 and 4 outside of the range
   int prev_seg_mover = -1;
   current_seg_number = 1;
   seg1_start = 0;
   seg1_end = 0;
   seg2_end = NUMPIXELS + 1;
   seg3_end = NUMPIXELS + 1;

// Based on the animation, we must reconfigure some segmentation variables to known limits  
    switch(pixelanim){
      case 1:
       seg_mover = 0; 
      break;
    
      case 2:
       seg1_start = (NUMPIXELS / 2);
       seg_mover = (NUMPIXELS / 2);
      break;
    
      case 3:
        seg_mover = NUMPIXELS-1;
        seg1_end = NUMPIXELS-1;
        seg2_end = 0;
        seg3_end = 0;    
      break;
    
      case 4:
        seg_mover = 0;
        if (((NUMPIXELS-1)%2)> 0){
          seg2_end = (NUMPIXELS/2)-1;
        } else {
          seg2_end = NUMPIXELS/2;
        };        
        if (((NUMPIXELS-1)%2)> 0){
          seg3_end = (NUMPIXELS/2)-1;
        } else {
          seg3_end = NUMPIXELS/2;
        };
      break;
    }
  
  
    while (current_seg_number<3){
      int colorsegmenter = rotary_process();         
      if (colorsegmenter == -128){seg_mover--;} 
      if (colorsegmenter == 64){seg_mover++;}    
  
      switch(pixelanim){
        case 1:
          switch (current_seg_number){
            case 1:
              seg_mover = constrain(seg_mover,0,NUMPIXELS-1);
            break;
            case 2:
              seg_mover = constrain(seg_mover,seg1_end+1,NUMPIXELS-1); 
            break;        
          }
    
        break;
      
        case 2:
          switch (current_seg_number){
            case 1:
               seg_mover = constrain(seg_mover,NUMPIXELS/2,NUMPIXELS-1);
            break;
            case 2:
               seg_mover = constrain(seg_mover,seg1_end+1,NUMPIXELS-1);
            break;        
          } 
        break;
      
        case 3:
          switch (current_seg_number){
            case 1:
              seg_mover = constrain(seg_mover,0,NUMPIXELS-1);
            break;
            case 2: 
              seg_mover = constrain(seg_mover,0,seg1_end-1); 
            break;        
          }
      
        break;
      
        case 4:
          switch (current_seg_number){
            case 1:
              if (((NUMPIXELS-1)%2)> 0){
                seg_mover = constrain(seg_mover,0,(NUMPIXELS/2)-2);
              }else{
                seg_mover = constrain(seg_mover,0,(NUMPIXELS/2)-1);
              }
            break;
            case 2:
              if (((NUMPIXELS-1)%2)> 0){
                seg_mover = constrain(seg_mover,seg1_end+1,(NUMPIXELS/2)-1);
              }else{
                seg_mover = constrain(seg_mover,seg1_end+1,NUMPIXELS/2);
              }
            break;        
          } 
        break;
      }
      
      
      if (digitalRead(button_pin) == LOW){ 
          delay(250);
          current_seg_number++;     
      }
  
  
   if (prev_seg_mover != seg_mover){  
         prev_seg_mover = seg_mover;
                    
        switch(current_seg_number){
            case 1:
              seg1_end = seg_mover;
            break;
      
            case 2:
              seg2_end = seg_mover;   
            break; 
      
        }
        buildarrays();
        loadallcolors();
        testlights();             
      }
   }
}




void sensorIsr() 
{ 
  unsigned long now = micros(); 
  interval = now - lastPulseTime; 
  lastPulseTime = now; 
  timeoutCounter = timeoutValue; 
} 


void exitmenu(){
  strip.clear(); 
  strip.show();
  for(int i=0; i<NUMPIXELS+1; i++) { 
  strip.setPixelColor(i, strip.Color(50, 50, 50)); 
  strip.show(); 
  delay(15); 
  strip.setPixelColor(i, strip.Color(0, 0, 0)); 
  strip.show(); 
  }
}

void entermenu(){
//Ascend strip 
  for (int i=0; i<(NUMPIXELS/2)+1; i++){ 
    strip.setPixelColor(i, strip.Color(0, 0, 25)); 
    strip.setPixelColor(NUMPIXELS-i, strip.Color(0, 0, 25)); 
    strip.show(); 
  delay(35); 
  } 
  // Descend Strip 
  for (int i=0; i<(NUMPIXELS/2)+1; i++){ 
    strip.setPixelColor(i, strip.Color(0, 0, 0)); 
    strip.setPixelColor(NUMPIXELS-i, strip.Color(0, 0, 0)); 
    strip.show(); 
    delay(35); 
  }
}


void bootanimation(){
    int colorwidth;
  colorwidth = (50/(NUMPIXELS/2));
  for (int i = 0; i<(NUMPIXELS/2)+1; i++){
    strip.setPixelColor(i,strip.Color((i*colorwidth),(i*colorwidth),((25*(NUMPIXELS/2))-((25*(i/2))+1))));
    strip.setPixelColor(NUMPIXELS-i,strip.Color((i*colorwidth),(i*colorwidth),((25*(NUMPIXELS/2))-((25*(i/2))+1))));  
    strip.show();
    delay(25);
    strip.setPixelColor(i-1,strip.Color(0,0,0));
    strip.setPixelColor(NUMPIXELS-i+1,strip.Color(0,0,0));
    strip.show();  
  }
  
  for (int i = 35; i>0 ; i--){
    delay(15);
    strip.setPixelColor((NUMPIXELS/2),strip.Color(i,i,i));
    strip.show();
  }
  
  strip.clear();
  strip.show();
}
