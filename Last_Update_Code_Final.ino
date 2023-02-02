// Code created for Tronicos - Air Quality Device.
// Any enquiries about the code email pablo.prieto@kcl.ac.uk

#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h> // Touch screen GFX+Hardware library 
#include <dht.h> // Humidity and Temperature Hardware library
// RTC Hardware library
#include <Wire.h>
#include <RTClib.h>
// SD libraries
#include <SPI.h>
#include <SD.h>


#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

// Setting up Touch Screen pins 
#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

// Setting up Touch Screen Boundaries
#define TS_MINX 120
#define TS_MAXX 900
#define TS_MINY 70
#define TS_MAXY 920

// Creating touch screen object
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// Setting up LCD pins for display.
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Defining reset pin for the LCD screen

// Defining sensor MQ2 pin
#define MQ2pin A13
// Define DHT11 sensor pin
#define DHT11_PIN 19

// Defining LEDs for interactive display
int GREEN_LED = 45;
int YELLOW_LED = 43;
int RED_LED = 41;

// Def Buzzer Pin
int BP = 31;
// Def SD Module
int pin_SD = 53;

// Defining color codes for LCD graphics.
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// Initialising LCD screen object
Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
// Initialising DHT object
dht DHT;
// Initialising RTC counter object
RTC_DS1307 rtc;
// Initialising File for recording variables
File recording_file;

// Defining size of interactive box and radius of pen (touch), this enables for the touch screen to work.
#define BOXSIZE 40
#define PENRADIUS 3

// Creating setup in order to create initial graphics of display and checkups
void setup(void) {
  Serial.begin(9600);
  Serial.println(F("TFT LCD test"));
  tft.reset();
  
  // SETUP RTC MODULE
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1);
  }

  // Automatically seting the RTC to the date & time on PC this sketch was compiled
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  DateTime start_time = rtc.now();

  // Shield Pinout checks in case of using one
  #ifdef USE_Elegoo_SHIELD_PINOUT
    Serial.println(F("Using Elegoo 2.4\" TFT Arduino Shield Pinout"));
  #else
    Serial.println(F("Using Elegoo 2.4\" TFT Breakout Board Pinout"));
  #endif

  // Resetting Screen to create the graphics on a new canvas
  tft.reset();
  
  // Identifier and check up setup
   uint16_t identifier = tft.readID();
   if(identifier == 0x9325) {
    Serial.println(F("Found ILI9325 LCD driver"));
  } else if(identifier == 0x9328) {
    Serial.println(F("Found ILI9328 LCD driver"));
  } else if(identifier == 0x4535) {
    Serial.println(F("Found LGDP4535 LCD driver"));
  }else if(identifier == 0x7575) {
    Serial.println(F("Found HX8347G LCD driver"));
  } else if(identifier == 0x9341) {
    Serial.println(F("Found ILI9341 LCD driver"));
  } else if(identifier == 0x8357) {
    Serial.println(F("Found HX8357D LCD driver"));
  } else if(identifier==0x0101)
  {     
      identifier=0x9341;
       Serial.println(F("Found 0x9341 LCD driver"));
  }
  else if(identifier==0x1111)
  {     
      identifier=0x9341;
       Serial.println(F("Found 0x9341 LCD driver"));
  }
  else {
    Serial.print(F("Unknown LCD driver chip: "));
    Serial.println(identifier, HEX);
    Serial.println(F("If using the Elegoo 2.8\" TFT Arduino shield, the line:"));
    Serial.println(F("  #define USE_Elegoo_SHIELD_PINOUT"));
    Serial.println(F("should appear in the library header (Elegoo_TFT.h)."));
    Serial.println(F("If using the breakout board, it should NOT be #defined!"));
    Serial.println(F("Also if using the breakout, double-check that all wiring"));
    Serial.println(F("matches the tutorial."));
    identifier=0x9341;
  }

  // Creating the start screen graphics
  tft.begin(identifier);
  display_start_screen(); // Creating graphics interface
  print_time(start_time); // Printing start time at the bottom of the screen

  // Setting up pen/finger to be used
  pinMode(13, OUTPUT);

  // Setup LEDs PINs
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  // Seting up Buzzer 
  //pinMode(BP,OUTPUT);

  // Seting up of SD Card
  pinMode(pin_SD, OUTPUT);
  // SD Card Initialization and checkups
  if (SD.begin())
  {
    Serial.println("SD card is ready to use.");
  } else
  {
    Serial.println("SD card initialization failed");
    return;
  }

  // Create/Open file 
  recording_file = SD.open("test.txt", FILE_WRITE);

  // Running tests for the SD card Usage and File Write/Read
  // if the file opened okay, check whether to write to it:
  if (recording_file) {
    Serial.println("Writing to file...");
    Serial.println("Writing Correct.");
  }
  // if the file didn't open, print an error:
  else {
    Serial.println("error opening test.txt");
  }
}

// Defining minimun/maximun pressure boundaries for the pen/finger.
#define MINPRESSURE 10
#define MAXPRESSURE 1000


// Creating loop for the working of the device
void loop(void){

  // Measuring current time through the RTC module
  DateTime now = rtc.now();
  print_time_per_second(now); // Printing it at the bottom

  // Setting up pen usability
  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);

  // if sharing pins, check to fix the directions of the touchscreen pins
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  // Check whether the pressure of pen/finger is within boundaries
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    // Mapping position of finger/pen to x and y coordinates.
    p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0); // scale from 0->1023 to tft.width
    p.y = (tft.height()-map(p.y, TS_MINY, TS_MAXY, tft.height(), 0)); // scaling to the tft.height
      if (p.y < (BOXSIZE + 200)) {
          if (p.x < (BOXSIZE*6)) {
            tft.drawRect(20, 200, ((BOXSIZE * 5)),BOXSIZE, WHITE); // Erasing actioned start botton interaction.
            // Taking current time
            now = rtc.now();
            // Separating time into hours minutes and seconds
            const int hours = now.hour();
            const int min = now.minute();
            const int sec = now.second();
            // Setting an iteration variable to keep track of the number variables were measured
            int it = 0;

            // Starting display loop
            do {
            // Creating the Display Interface
            tft.fillScreen(BLACK);
            tft.drawRect(20, 200, ((BOXSIZE * 5)),BOXSIZE, BLACK);

            tft.setCursor(0, 20);
            tft.setTextColor(BLACK);  tft.setTextSize(5);
            tft.fillRect(0, tft.getCursorY()-2, (BOXSIZE * 6), BOXSIZE * 1.75, CYAN);

            tft.println("Tronicos");
            tft.setCursor(20,tft.getCursorY() - 40);
            tft.setTextSize(3); tft.println("Air Quality");

            // Getting MQ2 values and DHT values
            float sensval = analogRead(MQ2pin);
            int chk = DHT.read11(DHT11_PIN);

            // Displaying tft variables
            tft.setCursor(0,tft.getCursorY()+20);
            tft.setTextSize(2); tft.setTextColor(WHITE);
            tft.print("Gas: ");tft.print(sensval); tft.println(" PPM");
            tft.println(); tft.println();
            tft.print("Temperature: "); tft.print(DHT.temperature); tft.println(" C");
            tft.println(); 
            tft.print("Humidity: "); tft.print(DHT.humidity); tft.println(" %"); 

            // Printing currrent time
            print_time(now);
            tft.setCursor(20, 250);

            // Setting up buzzer to low by default
            digitalWrite(BP,LOW);

            // Setting up status interaction and graphic display
            if ((DHT.temperature > 23) && (DHT.humidity > 40) || ((sensval < 200) && (sensval > 160))){
              tft.setTextSize(3);
              tft.fillRect(0, tft.getCursorY()-10, (BOXSIZE * 6), BOXSIZE, YELLOW); // Creating yellow box graphics
              tft.setTextColor(BLACK);
              tft.setCursor(3, tft.getCursorY());
              tft.print("Status:MEDIUM"); // Displaying Status in the box
              digitalWrite(BP,HIGH);
              yellow_light(); // Lighting corresponding LED
              delay(5000); // Delaying time to perform measurement at each 10 min
            } else if  (sensval > 200){
              tft.setTextSize(3);
              tft.fillRect(tft.getCursorX()-2, tft.getCursorY()-10, (BOXSIZE * 5), BOXSIZE, RED); // Creating red box graphics
              tft.setTextColor(BLACK); 
              tft.println("Status: BAD"); // Displaying Status in the box
              digitalWrite(BP,LOW);
              red_light(); // Lighting corresponding LED
              delay(5000); // Delaying time to perform measurement at each 10 min
            } else { 
              tft.setTextSize(3);
              tft.fillRect(tft.getCursorX()-2, tft.getCursorY()-10, (BOXSIZE * 5), BOXSIZE, GREEN); // Creating green box graphics
              tft.setTextColor(BLACK);
              tft.println("Status:GOOD"); // Displaying Status in the box
              digitalWrite(BP,HIGH);
              green_light(); // Lighting corresponding LED
              delay(5000); // Delaying time to perform measurement at each 10 min
            }
             // Writing in txt file from sd card at each iteration the current time and variables measured.
            recording_file.print(now.hour(),DEC); recording_file.print(":"); recording_file.print(now.minute(),DEC); recording_file.print(":"); recording_file.print(now.second(),DEC); recording_file.print(" "); recording_file.print(sensval); recording_file.print(" "); recording_file.print(DHT.temperature); recording_file.print(" "); recording_file.println(DHT.humidity);
            now = rtc.now();
            it += 1;
          } while (it != 144); // Looping until 144 iterations over 600000 ms delays are done, which equates to 24 h in total.
          // if time
          recording_file.close(); // Closing file after measurements are done.
        }
      }
  }
  // If wanted to refresh the screen after using the device
  //display_start_screen();
}

// Defining functions for lighting the LEDs
  void green_light()
  {
  // Usage:
  //  When this function is called, the Green LED is lighted up. 
  //  It is important to note that the different pins of the different color LEDs
  //  need to be pre-allocated before using this function.
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);
  }
 
void yellow_light()
  {
  // Usage:
  //  When this function is called, the Yellow LED is lighted up. 
  //  It is important to note that the different pins of the different color LEDs
  //  need to be pre-allocated before using this function.
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, HIGH);
  digitalWrite(RED, LOW);
  }
 
void red_light()
  {
  // Usage:
  //  When this function is called, the Red LED is lighted up. 
  //  It is important to note that the different pins of the different color LEDs
  //  need to be pre-allocated before using this function.
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  }

  
void print_time(DateTime now){
  // Usage:
  //  This function prints the current time at the bottom of the display.
  // Arguments:
  //  DateTime now, this argument can be obtained using rtc.now and it indicates the current time given by the RTC module.
  tft.setCursor(40, 300);
  tft.setTextColor(WHITE); tft.setTextSize(2);
  tft.print("GMT: "); tft.print(now.hour(),DEC); tft.print(":"); tft.print(now.minute(),DEC); tft.print(":"); tft.print(now.second(),DEC);
}

void delete_time(DateTime now){
  // Usage:
  //  This function deletes the previous time plotted, by printing a black box over it.
  // Arguments:
  //  DateTime now, this argument can be obtained using rtc.now and it indicates the current time given by the RTC module.
  tft.setCursor(40, 300);
  tft.setTextColor(BLACK); tft.setTextSize(2);
  tft.fillRect(tft.getCursorX(), tft.getCursorY(), BOXSIZE  + (BOXSIZE * 4), BOXSIZE, BLACK);
}

void print_time_per_second(DateTime now){
  // Usage:
  //  This function, deletes the displayed time appearing at the bottom of the display and displays the current time in the display.
  // Arguments:
  //  DateTime now, this argument can be obtained using rtc.now and it indicates the current time given by the RTC module.
  delete_time(now);
  print_time(now);
}

void display_start_screen(){
  // Usage:
  //  When called, this function creates the starting interface of the display.
  //  It creates two graphic parts, firstly, the name of the device, "Tronicos Air Quality"
  //  and secondly a green start box that indicates pressing and holding to activate the device.

  tft.fillScreen(BLACK); // This functions deletes all previous graphics in the screen.
  tft.setRotation(2); // Setting rotation to display the graphic interface in the right orientation.

  // Creating the graphics for the start screen
  tft.fillRect(20, 200, BOXSIZE  + (BOXSIZE * 4), BOXSIZE, GREEN);  // Creating Start Green Box
  tft.setCursor(50,203); // Allocating position of cursor (where it start drawing or putting text)
  tft.setTextColor(BLACK);  tft.setTextSize(5); tft.println("START"); // Setting text sizing and printing text

  tft.setCursor(5, BOXSIZE); // Setting cursor to right position to display the name of the device
  tft.setTextColor(BLACK);  tft.setTextSize(5); tft.setCursor(0,tft.getCursorY()+10);
  tft.fillRect(0, tft.getCursorY()-10, BOXSIZE  + (BOXSIZE * 5), BOXSIZE * 2, CYAN); // Filling background rectangle
  tft.print("Tronicos"); // Printing name of the company
  tft.setCursor(20,tft.getCursorY()); // Setting cursor to right position under the name of the company
  tft.setTextSize(3); tft.println("Air Quality"); // Printing name of the device

  tft.setTextColor(WHITE); // Creating interface for the interactive button
  tft.setCursor(50,tft.getCursorY()+30); tft.print("To begin"); // Printing and setting text to correct position
  tft.println(); // Setting empty line for spacing purposes
  tft.setCursor(15,tft.getCursorY()+7); // Setting Cursor to right position for text under "To begin"
  tft.print("Press & Hold");
  tft.setCursor(65,tft.getCursorY()+15);
}




