#define AC_LOAD  5   // Output to Opto-Triac GATE pin.
#define ZERO_PIN 1   // Interrupt to zero cross detection source.
#define LIGHT_GATE 0 // Light gate digital input

#define SET_SPEED 1000 // Target Speed

unsigned int SET = constrain(
    pow((11782 - SET_SPEED)/2.0849E-8, 0.33317),  // Approximate DIM_TIME
    5, 8000 // min and max
    );  // Dimming 9000 off 50 on
unsigned int control_signal, DIM_TIME;

// Without Control
//8100 - 500
//8000 - 1000
//7800 - 1500
//7000 - 5000
//6000 - 7300
//5000 - 9000
//4000 - 10000
//3000 - 11400
//2000 - 11500


volatile int count;

unsigned int START_COUNT = count;
unsigned int READ_COUNT;
unsigned int START_TIME  = millis();
unsigned int READ_TIME;
float  PID_ERROR[4];

unsigned int ZERO_START_TIME;
unsigned int ZERO_READ_TIME;

bool ZERO_CROSS = false;

float OMEGA;


void zero_cross_void() {
  digitalWrite(AC_LOAD, LOW);
  ZERO_START_TIME = micros();
  ZERO_CROSS = true;
}

void UpdateCount() {
  count++;
}

int ReadCount() {
  return count;
}


void setup() {
  Serial.begin(9600);
  
  pinMode(AC_LOAD, OUTPUT); // Set AC Load pin as output
  digitalWrite(AC_LOAD, LOW);   // triac On
  
  attachInterrupt( // Check whether signal is rising
    digitalPinToInterrupt(ZERO_PIN),
    zero_cross_void,
    RISING
  );

  attachInterrupt(
    digitalPinToInterrupt(LIGHT_GATE),
    UpdateCount,
    RISING
  );

}

void loop() {
  Serial.println("----");

  for (;;) {
    READ_TIME  = millis() - START_TIME;
    READ_COUNT = ReadCount() - START_COUNT;
  
    // Update speed measurement every xxx ms
    if (READ_TIME >= 1000) {
      OMEGA = ((60*1E3*READ_COUNT)/READ_TIME);
      
      // check for nan values
      if (!OMEGA) {
        OMEGA = 0;
        }
  
      // Serial.print(READ_COUNT);
      // Serial.print(" ");
      Serial.println(OMEGA);


      PID_ERROR[0]  = OMEGA - SET_SPEED; // Proportional
      PID_ERROR[1] += PID_ERROR[0]; // Integral
      PID_ERROR[3]  = (PID_ERROR[0] - PID_ERROR[3]);
      PID_ERROR[2]  = (abs(PID_ERROR[3])<1E-3) ? 0 : PID_ERROR[3]; // Differential

      control_signal = (1/READ_TIME)*(0*PID_ERROR[0] + 0*PID_ERROR[1] + 0*PID_ERROR[2]);
      DIM_TIME  = constrain(SET+control_signal, 5, 8000);
      
      START_COUNT = ReadCount();
      START_TIME  = millis();

      PID_ERROR[3] = PID_ERROR[0];
    }
  
    // Update TRIAC if needed
    if (ZERO_CROSS) {
      ZERO_READ_TIME = micros() - ZERO_START_TIME;
      if (ZERO_READ_TIME >= DIM_TIME) {
        digitalWrite(AC_LOAD, HIGH);   // triac On
        if (ZERO_READ_TIME >= (DIM_TIME + 10)) {
          digitalWrite(AC_LOAD, LOW);  // triac Off
          ZERO_CROSS = false;
        }
      }
    }
  }
}
