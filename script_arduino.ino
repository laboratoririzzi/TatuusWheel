#include "SPI.h"
#include <Adafruit_GFX.h>
#include <ILI9488.h>
#include <math.h>
#include <Wire.h>

// define pin for display
#define TFT_CS  10
#define TFT_DC  9
#define TFT_RST 8

// define display for buttons
#define button1 A5
#define button2 A4

// define array of leds
int const ledNum = 5;
int const ledNumPlus1 = ledNum + 1;
int const thresholdBlink = 96;
int const timeBetweenBlink = 250;
int led[ledNum] = {6, 5, 4, 3, 2};
int thresholdsArray[ledNumPlus1] = {88, 90, 92, 94, 95, thresholdBlink};

// define variable for buttons
unsigned long timeSinceLastClick = 0;
int timeThresholdButton = 600;

// bool for button release
bool button1WasReleased = true;
bool button2WasReleased = true;

// bool used in display cancellation
bool hasJustChangedMenu = true;

// define array of delimiters included 0
int const numOfVar = 8;
int delimiters[numOfVar + 1]; 

// gear variables
int gear = 1;
int prewGear = 1;

// percentage of rpms
int percInt;

// text size
int textSizeGear = 7;
int textSizeTime = 4;
int textSizeDelta = 4;
int textSizeFuel = 4;
int textSizePosition = 6;

// gear pos
int gearX = 40;
int gearY = 40;

// delta pos
int deltaX = 200;
int deltaY = 140;

// times pos
int bestTimeX = 250;
int bestTimeY = 40;
int lastTimeX = 250;
int lastTimeY = 100;

// fuel pos
int fuelX = 250;
int fuelY = 100;

// position pos
int positionX = 250;
int positionY = 40;

// declare strings
String delta;
String deltaPrew;
String bestTime;
String bestTimePrew;
String lastTime;
String lastTimePrew;
String position;
String positionPrew;
String fuel;
String fuelPrew;
String dataString, rpm, rpmMax;
String gears[10] = {"R", "N", "1", "2", "3", "4", "5", "6","7","8"};

// menu variables
int menuPage = 0;
int maxMenuPage = 3;
int prewMenuPage = 0;

// define tft
ILI9488 tft = ILI9488(TFT_CS, TFT_DC, TFT_RST);

void setup() {

  // serialize all pins
  for (int i = 0; i < ledNumPlus1; i++) {
    pinMode(led[i], OUTPUT);
  }
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);

  // start serial
  Serial.begin(9600);
  Serial.setTimeout(20);

  // start tft
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9488_BLACK);

  // set al leds to off
  for (int i = 0; i < ledNum; i++) {
    digitalWrite(led[i], LOW);
  }

  // set all leds to on
  for (int i = 0; i < ledNum; i++) {
    digitalWrite(led[i], HIGH);
    delay(300);
  }

  // set all leds to off
  for (int i = 0; i < ledNum; i++) {
      digitalWrite(led[i], LOW);
  }

}

void loop() {

  // receive data from Python
  while(Serial.available() > 0) {
    String dataString = Serial.readString();
    dataString.trim();
    // unpack all data
    delimiters[0] = dataString.indexOf("|");
    for (int i = 1; i < ( numOfVar + 1); i++) {
      delimiters[i] = dataString.indexOf("|", delimiters[i - 1] + 1);
    }
    // assign to correct variable the unpacked data
    gear = (dataString.substring(delimiters[0] + 1, delimiters[1])).toInt();
    delta = dataString.substring(delimiters[1] + 1, delimiters[2]);
    fuel = dataString.substring(delimiters[2] + 1, delimiters[3]);
    rpm = dataString.substring(delimiters[3] + 1, delimiters[4]);
    rpmMax = dataString.substring(delimiters[4] + 1, delimiters[5]);
    bestTime = dataString.substring(delimiters[5] + 1, delimiters[6]);
    lastTime = dataString.substring(delimiters[6] + 1, delimiters[7]);
    position = dataString.substring(delimiters[7] + 1, delimiters[8]);
    //compute percentage
    percInt = (rpm.toInt() * 100 ) / rpmMax.toInt();
    // led on-off
    if(percInt <= thresholdBlink) {
      comparator(led, percInt, thresholdsArray);
    }
    else {
      blink(led, ledNum, timeBetweenBlink);
    }
  }

  // detect button 1 click
  if(digitalRead(button1) == LOW) {
    // timeSinceLastClick is to avoid a multiple click in a limited part of time, buttonWasReleased is necessary to avoid click before releasing the button
    if(millis() - timeSinceLastClick > timeThresholdButton && button1WasReleased == true) {
      button1WasReleased = false;
      prewMenuPage = menuPage;
      menuPage -= 1;
      if(menuPage < 0) {
        menuPage = maxMenuPage - 1;
      }
      hasJustChangedMenu = true;
      delay(1); // this delay is necessary to avoid double increment of n1
      timeSinceLastClick = millis();
    }
  }

  // reset button1
  else if(digitalRead(button1) == HIGH) {
      button1WasReleased = true;
  }

  if(digitalRead(button2) == LOW) {
    if(millis() - timeSinceLastClick > timeThresholdButton && button2WasReleased == true) {
      button2WasReleased = false;
      prewMenuPage = menuPage;
      menuPage += 1;
      if(menuPage > maxMenuPage - 1){
        menuPage = 0;
      }
      hasJustChangedMenu = true;
      delay(1);
      timeSinceLastClick = millis();
    }
  }

  else if(digitalRead(button2) == HIGH) {
      button2WasReleased = true;
  }

  // the concept to clean the screen is rewrite in black the same value and after that write in white the new value, in this way I use a lot less cpu
  if ( gear == 1 && (prewGear == 0 || prewGear == 2 ) ) {
    textPrint(gears[prewGear], gearX, gearY, textSizeGear, ILI9488_BLACK);
    textPrint("N", gearX, gearY, textSizeGear, ILI9488_WHITE);
    prewGear = gear;
    delay(1);
  }
      
  else if (gear != 1 && gear != prewGear && (prewGear != 0 || prewGear != 2 )) {
    textPrint(gears[prewGear], gearX, gearY, textSizeGear, ILI9488_BLACK);
    textPrint(gears[gear], gearX, gearY, textSizeGear, ILI9488_WHITE);
    prewGear = gear;
    delay(1);
  }

  if(menuPage == 0) {
    // first time after changing menu I need to clean all the old data, in this case I remove deltaPrew and after I write best time and lastTime
    if( hasJustChangedMenu == true && prewMenuPage == 1) {
      clear1();
      textPrint(bestTime, bestTimeX, bestTimeY, textSizeTime, ILI9488_WHITE);
      textPrint(lastTime, lastTimeX, lastTimeY, textSizeTime, ILI9488_WHITE);
      hasJustChangedMenu = false;
    }

    if( hasJustChangedMenu == true && prewMenuPage == 2) {
      clear2();
      textPrint(bestTime, bestTimeX, bestTimeY, textSizeTime, ILI9488_WHITE);
      textPrint(lastTime, lastTimeX, lastTimeY, textSizeTime, ILI9488_WHITE);
      hasJustChangedMenu = false;
    }    

    // in the other situation I clean only the old value, in this case I clean the prewious best time and I write the new best time
    if(bestTime != bestTimePrew) {
      textPrint(bestTimePrew, bestTimeX, bestTimeY, textSizeTime, ILI9488_BLACK);
      textPrint(bestTime, bestTimeX, bestTimeY, textSizeTime, ILI9488_WHITE);
      bestTimePrew = bestTime;
    }

    if(lastTime != lastTimePrew) {
      textPrint(lastTimePrew, lastTimeX, lastTimeY, textSizeTime, ILI9488_BLACK);
      textPrint(lastTime, lastTimeX, lastTimeY, textSizeTime, ILI9488_WHITE);
      lastTimePrew = lastTime;
    }
  }

  else if(menuPage == 1) {
    if( hasJustChangedMenu == true && prewMenuPage == 0) {
      clear0();
      if(delta.charAt(0) == '-') {
        textPrint(delta, deltaX, deltaY, textSizeDelta, ILI9488_GREEN);
      }
      else{
        textPrint(delta, deltaX, deltaY, textSizeDelta, ILI9488_RED);
      }
      hasJustChangedMenu = false;
    }

    if( hasJustChangedMenu == true && prewMenuPage == 2) {
      clear2();
      if(delta.charAt(0) == '-') {
        textPrint(delta, deltaX, deltaY, textSizeDelta, ILI9488_GREEN);
      }
      else{
        textPrint(delta, deltaX, deltaY, textSizeDelta, ILI9488_RED);
      }
      hasJustChangedMenu = false;
    }

    if( delta != deltaPrew ){
      textPrint(deltaPrew, deltaX, deltaY, textSizeDelta, ILI9488_BLACK);
      if(delta.charAt(0) == '-') {
        textPrint(delta, deltaX, deltaY, textSizeDelta, ILI9488_GREEN);
      }
      else{
        textPrint(delta, deltaX, deltaY, textSizeDelta, ILI9488_RED);
      }
      deltaPrew = delta;
    }
  }

  else if(menuPage == 2){
    if( hasJustChangedMenu == true && prewMenuPage == 0) {
      clear0();
      textPrint(fuel, fuelX, fuelY, textSizeFuel, ILI9488_WHITE);
      textPrint(position, positionX, positionY, textSizePosition, ILI9488_WHITE);
      hasJustChangedMenu = false;
    }

    if( hasJustChangedMenu == true && prewMenuPage == 1) {
      clear1();
      textPrint(fuel, fuelX, fuelY, textSizeFuel, ILI9488_WHITE);
      textPrint(position, positionX, positionY, textSizePosition, ILI9488_WHITE);      
      hasJustChangedMenu = false;
    }

    if(fuel != fuelPrew){
      textPrint(fuel, fuelX, fuelY, textSizeFuel, ILI9488_BLACK);
      textPrint(fuel, fuelX, fuelY, textSizeFuel, ILI9488_WHITE);
      fuelPrew = fuel;
    }

    if(position != positionPrew){
      textPrint(position, positionX, positionY, textSizePosition, ILI9488_BLACK);
      textPrint(position, positionX, positionY, textSizePosition, ILI9488_WHITE);
      positionPrew = position;
    }
  }
}

// function used to light on leds, it receive the actual value and it compare to an array of different threshold in this way it can deceide what led (in an array) 
// should be activated
void comparator(int l[], int perc, int thresholds[]) {
  for (int i = 0; i < ledNum; i++) {
    if ( perc > thresholds[i]) {
      digitalWrite(l[i], HIGH);
    }
    else{
      digitalWrite(l[i], LOW);
    }
  }
}

// function used to blink, first arg is array of led, second arg is the number of led and the third is time between blink
void blink(int b[], int ledNum, int time) {
  for (int i = 0; i < ledNum; i++) {
    digitalWrite(b[i], HIGH);
  }
  delay(time);
  
  for (int i = 0; i < ledNumPlus1; i++){
    digitalWrite(b[i], LOW);
  }
  delay(time);
}

// function to write text, the args are the string, the coordinates, the size and the color
void textPrint(String text, int x, int y, int size, int color) {
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextSize(size);
  tft.println(text);
}

void clear0(){
  textPrint(bestTimePrew, bestTimeX, bestTimeY, textSizeTime, ILI9488_BLACK);
  textPrint(lastTimePrew, lastTimeX, lastTimeY, textSizeTime, ILI9488_BLACK);
}

void clear1(){
  textPrint(deltaPrew, deltaX, deltaY, textSizeDelta, ILI9488_BLACK);
  delay(30); 
}

void clear2(){
  textPrint(fuel, fuelX, fuelY, textSizeFuel, ILI9488_BLACK);
  textPrint(position, positionX, positionY, textSizePosition, ILI9488_BLACK);
}
