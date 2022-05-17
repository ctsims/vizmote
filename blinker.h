#define LED 0

const int BLINKER_PATTERN_ON_OFF[] {500, 500, 500, 500, 500, 500};
const int BLINKER_PATTERN_SHORT_OFF[] {600, 200, 600, 200, 600, 200};
const int BLINKER_PATTERN_MEEP_MEEP[] {100, 100, 100, 1000, 0, 0};
const int BLINKER_PATTERN_MOSTLY_ON[] {2000, 100, 2000, 100, 2000, 100};
const int BLINKER_PATTERN_MOSTLY_OFF[] {100, 2000, 100, 2000, 100, 2000};

class LedBlinker {
  private:
    int pin;
    int on_state;
    int off_state;
      
    long lastBlink = millis();
    int nextBlinkState = 0;
    
    int blink_pattern_index = 0;
    
    const int* current_pattern = BLINKER_PATTERN_ON_OFF;
    bool isBlinking = false;
    void resetBlinkState();
    
  
  public: 
    LedBlinker(int led_pin, int on_state = LOW, int off_state = HIGH);
    void ledOFF();
    void ledON();
    void blink(const int pattern[] = BLINKER_PATTERN_ON_OFF);
    void setup();
    void loop();
};


LedBlinker::LedBlinker(int led_pin, int on_state_val, int off_state_val) {
  pin = led_pin;
  on_state = on_state_val;
  off_state = off_state_val;
}

void LedBlinker::ledOFF() {
  digitalWrite(pin, off_state);
  isBlinking = false;
}
void LedBlinker::ledON() {
  digitalWrite(pin, on_state);
  isBlinking = false;
}

void LedBlinker::resetBlinkState() {
  blink_pattern_index = 0;
  lastBlink = millis();
  //Always start ON
  digitalWrite(pin, on_state);
  nextBlinkState = off_state;
}

void LedBlinker::blink(const int pattern[]) {
  current_pattern = pattern;
  isBlinking = true;
  resetBlinkState();
}

void LedBlinker::setup() {
  pinMode(pin, OUTPUT);
}

void LedBlinker::loop() {
  if(isBlinking) {
    long m = millis();
    long elapsed = m - lastBlink;
    if (elapsed > current_pattern[blink_pattern_index]) {

      int current_index = blink_pattern_index;
      int next_index = (blink_pattern_index + 1) % 6;
      
      //catchup on blinks if needed
      if ( (elapsed - current_pattern[blink_pattern_index]) > current_pattern[next_index]) {
        current_index = next_index;
        next_index = (next_index + 1) % 6;
        nextBlinkState = (nextBlinkState +1) % 2;
      }
      
      digitalWrite(pin, nextBlinkState);
      
      nextBlinkState = (nextBlinkState +1) % 2;
      blink_pattern_index = next_index;
      lastBlink = m;
    }
  }
}
