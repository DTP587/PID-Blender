
#define led  4
#define butt 7

bool ledState = true;

void setup() {
  // put your setup code here, to run once:
  pinMode(led, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(butt), switch_led, RISING);
}

void switch_led() {
  if (ledState == true) {
    Serial.println("On");
    digitalWrite(led, HIGH);
    ledState = false;
    }
  else if (ledState == false) {
    Serial.println("Off");
    digitalWrite(led, LOW);
    ledState = true;
    }
}


void loop() {
  // put your main code here, to run repeatedly:
}
