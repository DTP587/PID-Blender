#include <Arduino.h>

// ------------------ Defines -------------------

#define LIGHT_GATE  0 // Light gate digital input
#define ZERO_PIN    1 // Interrupt to zero cross detection source.
#define LED_PIN     4 // LED pin output.
#define BUTTON_PIN  7 // Button pin.
#define AC_LOAD     5 // Output to Opto-Triac GATE pin.

#define TOTAL_TIME  5     // Minutes, total time on + off
#define ON_TIME     10   // Seconds, on time
#define OFF_TIME    10    // Seconds, off time
#define SET_OMEGA   10000 // RPM 1000 min, 20000 max

// ------------------ Screen -------------------

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Screen information
#define SCREEN_ADDRESS 0x3C
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void displayMessage(Adafruit_SSD1306 &display, String MESSAGE, int TIME_SO_FAR) {
  // Display initial text
  display.clearDisplay();
  display.setTextSize(2); // 2x Size
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0,0); // Start at top-left corner
  
  display.println(MESSAGE);
  display.setTextSize(1); // Normal 1:1 pixel scale
  display.println(F("---------------------"));
  
  display.print(F("On  time: ")); display.print(ON_TIME); display.println(F(" s"));
  display.print(F("Off time: ")); display.print(OFF_TIME); display.println(F(" s"));
  display.print(F("In Total: ")); display.print(TIME_SO_FAR); display.print("/"); display.print(TOTAL_TIME); display.println(F(" m"));
  display.println(F("---------------------"));

  display.setTextSize(1);
  display.print(F("Target:   ")); display.print(SET_OMEGA); display.println(F(" rpm"));
  display.display();
  }

// ----------------------------------------------

// Speed counter
volatile int COUNT;
// ZERO_CROSS_FLAG, ZERO_START_TIME, ZERO_READ_TIME
volatile unsigned int ZERO_STATE[3];
// Button flag
volatile bool BUTTON_FLAG = false;
// Button timer
volatile long BUTTON_TIME;

unsigned int DIM_TIME = constrain( // LARGE JUG
    pow((11782 - SET_OMEGA)/2.0849E-8, 0.33317),  // Approximate DIM_TIME
    7000, 8000 // min and max
    );  // Dimming 8000 off 5 on

unsigned int START_COUNT;
unsigned int START_TIME;
unsigned int READ_COUNT;
unsigned int READ_TIME;

float  PID_ERROR[4];
float OMEGA;

// ----------------------------------------------

void zeroCrossISR() {
  digitalWrite(AC_LOAD, LOW);
  ZERO_STATE[2] = micros();
  ZERO_STATE[1] = 1E3;
}

void UpdateCount() {
  COUNT++;
}

void interruptISR() {
  if (BUTTON_FLAG == false) {
    BUTTON_TIME = millis() - BUTTON_TIME;
    if (BUTTON_TIME > 200) {
      Serial.println("Button Triggered");
      BUTTON_FLAG = true;
    }
  }
  else if (BUTTON_FLAG == true) {
    // skip
  }
  BUTTON_TIME = millis();
}

int ReadCount() {
  return COUNT;
}

// ----------------------------------------------


long millisSince(long TIME) {
  return (millis()-TIME);
  }

long roundMillisSince(long TIME) {
  return floor(millisSince(TIME));
  }

bool gapTimeIsReached(unsigned int GAP_TIME, long START_TIME) {
  return (roundMillisSince(START_TIME) >= GAP_TIME*1E3);
  }

float isTotalRuntimeReached(long TIME) {
  return (roundMillisSince(TIME) < (60E3*TOTAL_TIME));
  }

// ------------------- routine ------------------



// ==============================================

void setup() {

  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  pinMode(AC_LOAD, OUTPUT);   // Set AC Load pin as output
  digitalWrite(AC_LOAD, LOW); // triac On
  pinMode(LED_PIN, OUTPUT);   // LED pin output
  digitalWrite(BUTTON_PIN, HIGH); // Set button high
  
  attachInterrupt( // Check whether signal is rising
    digitalPinToInterrupt(ZERO_PIN),
    zeroCrossISR,
    RISING
  );

  attachInterrupt(
    digitalPinToInterrupt(LIGHT_GATE),
    UpdateCount,
    RISING
  );

  attachInterrupt(
    digitalPinToInterrupt(BUTTON_PIN),
    interruptISR,
    LOW
  );
}

// ----------------------------------------------

void loop() {

  unsigned int GAP_TIME = 0;
  unsigned int ON_COUNTER = 0;
  unsigned int OFF_COUNTER = 0;
  unsigned int TARGET = 0;
  unsigned int SPEED_READ_COUNTER = 0;

  displayMessage(display, "CONNECTED", 0);
  
  while (!BUTTON_FLAG) {
    // Do nothing
    }
  BUTTON_FLAG = false;


  displayMessage(display, "PROCESSING", 0);
  delay(1000);

  unsigned long START_ROUTINE_TIME = millis();
  
  // This loop needs to stay FAST, no display commands!
  while (isTotalRuntimeReached(START_ROUTINE_TIME)) {
    
    if (gapTimeIsReached(GAP_TIME, START_ROUTINE_TIME)) {

      if (OFF_COUNTER >= ON_COUNTER) {
        
        // Display on message

        TARGET = SET_OMEGA;
        PID_ERROR[2] = 0; //Clear Integral memory

        START_COUNT = COUNT;
        START_TIME  = millis();
        SPEED_READ_COUNTER = 0;

        digitalWrite(LED_PIN, HIGH);
        ON_COUNTER += 1;
        }

      else if (OFF_COUNTER < ON_COUNTER) {

        // Display off message
        displayMessage(display, "PROCESSING", GAP_TIME/60);
        TARGET = 0;

        digitalWrite(LED_PIN, LOW);
        OFF_COUNTER += 1;
        }
      
      GAP_TIME = ON_TIME*ON_COUNTER + OFF_TIME*OFF_COUNTER;
      }

    // Check for external interrupt.
    if (BUTTON_FLAG) {
      digitalWrite(LED_PIN, LOW);
      break;
    }

    // Speed control if on.
    if (TARGET == SET_OMEGA) {
      READ_TIME  = millis() - START_TIME;
      READ_COUNT = ReadCount() - START_COUNT;
      
      // Update speed measurement every xxx ms
      if (READ_TIME >= 1000) {
        OMEGA = ((60*1E3*READ_COUNT)/READ_TIME);
        
        // check for nan values
        if (!OMEGA) {
          OMEGA = 0;
          }
        else if ((SPEED_READ_COUNTER < 1) && (ON_COUNTER > 1)) {
          // Pass, don't update for the first cycle after rough angle is found
          }
        else {
          PID_ERROR[1]  = OMEGA - SET_OMEGA; // Proportional
          PID_ERROR[2] += PID_ERROR[1];      // Integral
                                             // Differential
          PID_ERROR[4]  = (PID_ERROR[1] - PID_ERROR[4]);
          PID_ERROR[3]  = (abs(PID_ERROR[4])<1E-3) ? 0 : PID_ERROR[4];
          PID_ERROR[4]  = PID_ERROR[1];
  
          DIM_TIME += 0.06*PID_ERROR[1] + 0.011*PID_ERROR[2] + 0.06*PID_ERROR[3];
          DIM_TIME  = constrain(DIM_TIME, 5, 8000);
          }
        
//        Serial.print(DIM_TIME);
//        Serial.print(" ");
        Serial.println(OMEGA);

        SPEED_READ_COUNTER += 1;
        START_COUNT = ReadCount();
        START_TIME  = millis();

      }
    
      // Update TRIAC if needed
      if (ZERO_STATE[1] == 1E3) {
        ZERO_STATE[3] = micros() - ZERO_STATE[2];
        if (ZERO_STATE[3] >= DIM_TIME) {
          digitalWrite(AC_LOAD, HIGH);   // triac On
          if (ZERO_STATE[3] >= (DIM_TIME + 10)) {
            digitalWrite(AC_LOAD, LOW);  // triac Off
            ZERO_STATE[1]=0;
          }
        }
      }
    else if (TARGET == 0) {
      // pass, no call to setSpeed()
      }
    }
  }
  
  if (BUTTON_FLAG) {
    Serial.print("Time on: ");
    Serial.println(millisSince(START_ROUTINE_TIME));
    displayMessage(display, "STOPPED", GAP_TIME/60);
    }
  else if (!BUTTON_FLAG) {
    displayMessage(display, "COMPLETED", TOTAL_TIME);
    }

  BUTTON_FLAG = false;
  while (!BUTTON_FLAG) {
    digitalWrite(LED_PIN, HIGH);
    delay(1);
    digitalWrite(LED_PIN, LOW);
    delay(20);
    }
  
  BUTTON_FLAG = false;
}
