#define BUTTON_PIN 5

#define STATE_IDLE 0
#define STATE_DOWN 1
#define STATE_LONG_FIRED 2

#define ACTION_IDLE 0
#define ACTION_PRESS 1
#define ACTION_LONG_PRESS 2

#define LONG_PRESS_THRESHOLD 3000

unsigned int tButtonDown = 0;

int state = STATE_IDLE;
bool triggered = false;

int nButtonRead;

int buttonLoop() {
  nButtonRead = digitalRead(BUTTON_PIN);

  if (nButtonRead == LOW && (state == STATE_IDLE || state == STATE_LONG_FIRED)) {
    state = STATE_DOWN;
    tButtonDown = millis();
    return ACTION_IDLE;
  }
  
  if (nButtonRead == LOW && state == STATE_DOWN) {
    unsigned int span = (millis() - tButtonDown);
    if (span > LONG_PRESS_THRESHOLD) {
      state = STATE_LONG_FIRED;
      return ACTION_LONG_PRESS;
    }

    return ACTION_IDLE;
  }

  if (nButtonRead == HIGH && state == STATE_DOWN) {
      state = STATE_IDLE;
      return ACTION_PRESS;
  }
  return ACTION_IDLE;
}
