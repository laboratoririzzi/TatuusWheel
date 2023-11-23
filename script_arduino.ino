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
int n1 = 0;
int n2 = 0;
bool button1WasReleased = true;
bool button2WasReleased = true;

// define array of delimiters included 0
int const numOfVar = 3;
int delimiters[numOfVar + 1]; 

// 
int gear = 1;
int prewGear = 1;
int percInt;
String dataString, rpm, rpmMax;
String gears[8] = {"R", "N", "1", "2", "3", "4", "5", "6"};

// 
int menuPage = 0;
int maxMenuPage = 2;
// 
int textSize = 7;
int xC = 40;
int yC = 40;

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

  // set al led to off
  for (int i = 0; i < ledNum; i++) {
    digitalWrite(led[i], LOW);
  }

  // set all led to on
  for (int i = 0; i < ledNum; i++) {
    digitalWrite(led[i], HIGH);
    delay(300);
  }

  // set al led to off
  for (int i = 0; i < ledNum; i++) {
      digitalWrite(led[i], LOW);
  }

  textPrint("N", xC, yC, textSize, ILI9488_WHITE);

}

void loop() {

  // receive data from Python
  while(Serial.available() > 0) {
    String dataString = Serial.readString();
    dataString.trim();
    delimiters[0] = dataString.indexOf("|");
    for (int i = 1; i < ( numOfVar + 1); i++) {
      delimiters[i] = dataString.indexOf("|", delimiters[i - 1] + 1);
    }
    gear = (dataString.substring(delimiters[0] + 1, delimiters[1])).toInt();
    rpm = dataString.substring(delimiters[1] + 1, delimiters[2]);
    rpmMax = dataString.substring(delimiters[2] + 1, delimiters[3]);
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
      if(menuPage > 0) {
        menuPage -= 1;
      }
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
      if(menuPage < (maxMenuPage - 1)){
        menuPage += 1;
      }
      delay(1);
      timeSinceLastClick = millis();
    }
  }
 
  else if(digitalRead(button2) == HIGH) {
      button2WasReleased = true;
  }

  if(menuPage == 0) {
    if ( gear == 1 && (prewGear == 0 || prewGear == 2 ) ) {
      textClear(gears[prewGear], xC, yC, textSize, ILI9488_BLACK);
      textPrint("N", xC, yC, textSize, ILI9488_WHITE);
      prewGear = gear;
      delay(1);
    }
      
    else if (gear != 1 && gear != prewGear && (prewGear != 0 || prewGear != 2 )) {
      textClear(gears[prewGear], xC, yC, textSize, ILI9488_BLACK);
      textPrint(gears[gear], xC, yC, textSize, ILI9488_WHITE);
      prewGear = gear;
      delay(1);
    }
  }

  else if(menuPage == 1) {
    tft.fillScreen(ILI9488_RED);
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

// function used to blink, first arg is array of led, second arg is the number of led
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

void textClear(String text, int x, int y, int size, int color) {
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextSize(size);
  tft.println(text);
}

void textPrint(String text, int x, int y, int size, int color) {
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextSize(size);
  tft.println(text);
}