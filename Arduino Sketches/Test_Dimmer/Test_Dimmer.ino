/*
Original Uno wiring
Gate = (7, Digital) Sync = (3, PWM)
Looks like the original code was hacked off:
https://www.instructables.com/Arduino-controlled-light-dimmer-The-circuit/
https://en.wikipedia.org/wiki/TRIAC
https://www.youtube.com/watch?v=cyeEzU6KmzY

 +V ▲
    │
    │
    │       ooo ──── 340 Vpp
    │    ooo   ooo
    │  oo         oo
    │ o             o
    │o               o                 20 ms
   0├──────────────────────────────────► t
    │           10 ms o               o
    │                  o             o
    │                   oo         oo
    │                     ooo   ooo
    │                        ooo
    │
    │
 -V ▼  50 Hz ~ 1s/50 = 20 ms

Zero crosses every 1/2 cycle, with each 1/2 cycle lasting 10 ms for 50 Hz
signal.

*/

#define AC_LOAD  5  // Output to Opto-Triac GATE pin.
#define ZERO_PIN 1  // Interrupt to zero cross detection source.

unsigned char dimming = 80;  // Dimming level (0-100) (0 - on, 95 - off)
                             // below 5 there is some flickering

void setup() {
  Serial.begin(9600);
  
  pinMode(AC_LOAD, OUTPUT); // Set AC Load pin as output
  digitalWrite(AC_LOAD, LOW);   // triac On
  
  attachInterrupt( // Check whether signal is rising
    digitalPinToInterrupt(ZERO_PIN),
    zero_crosss_int,
    RISING
  );


}


// function to be fired at the zero crossing to dim the light
void zero_crosss_int() {
  long dimtime = (100*dimming);   // For 60Hz =>65
  Serial.println(dimtime);
  delayMicroseconds(dimtime);    // Off cycle
  digitalWrite(AC_LOAD, HIGH);   // triac On
  delayMicroseconds(10);
  digitalWrite(AC_LOAD, LOW);    // triac Off
}



void loop() {
}
