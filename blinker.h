#define LED 0
#define BLINK_CYCLE 500

unsigned long lastBlink = 0;
int blinkState = 0;


void ledOFF() {
  digitalWrite(LED, HIGH);
}
void ledON() {
  digitalWrite(LED, LOW);
}

void toggleLed() {
  int m = millis();
  if (m - lastBlink > BLINK_CYCLE) {
    digitalWrite(LED, blinkState);
    blinkState = (blinkState +1) % 2;
    lastBlink = m;
  }
}
