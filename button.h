#define STATE_IDLE 0
#define STATE_DOWN 1
#define STATE_LONG_FIRED 2

#define ACTION_IDLE 0
#define ACTION_PRESS 1
#define ACTION_LONG_PRESS 2

#define EVENT_IDLE 0
#define EVENT_BUTTON_ONE 1
#define EVENT_BUTTON_ONE_LONG 2
#define EVENT_BUTTON_TWO 3
#define EVENT_BUTTON_TWO_LONG 4
#define EVENT_BUTTON_THREE 5
#define EVENT_BUTTON_THREE_LONG 6
#define EVENT_BUTTON_ONE_TWO_LONG 50

#define LONG_PRESS_THRESHOLD 2000

class ButtonController {

  struct button_state {
    int pin = -1;
    int current = STATE_IDLE;
    unsigned int tButtonDown = 0;
  };
  
  public:
    ButtonController(int button_one_pin, int button_two_pin, int button_three_pin);
    int current_event;
    void loop();
    void setup();
  
  private:
    int updateCurrentEvent();
    int getButton(button_state& bState);
    button_state button_one;
    button_state button_two;
    button_state button_three;
  
};

ButtonController::ButtonController(int button_one_pin, int button_two_pin, int button_three_pin) {
  button_one.pin = button_one_pin;
  button_two.pin = button_two_pin;
  button_three.pin = button_three_pin;
}

int ButtonController::getButton(button_state& bState) {
  int nButtonRead = digitalRead(bState.pin);

  if (nButtonRead == LOW && (bState.current == STATE_IDLE)) {
    bState.current = STATE_DOWN;
    bState.tButtonDown = millis();
    return ACTION_IDLE;
  }
  
  if (nButtonRead == LOW && bState.current == STATE_DOWN) {
    unsigned int span = (millis() - bState.tButtonDown);
    if (span > LONG_PRESS_THRESHOLD) {
      bState.current = STATE_LONG_FIRED;
      return ACTION_LONG_PRESS;
    }

    return ACTION_IDLE;
  }

  if (nButtonRead == HIGH ) {
    if (bState.current == STATE_DOWN) {
      bState.current = STATE_IDLE;
      return ACTION_PRESS;
    } else if (bState.current == STATE_LONG_FIRED) {
      bState.current = STATE_IDLE;
      return ACTION_IDLE;
    }
  }
  return ACTION_IDLE;
}

int ButtonController::updateCurrentEvent() {
  int b1 = getButton(button_one);
  int b2 = getButton(button_two);
  int b3 = getButton(button_three);

  //If one button triggers a long press, and the other is also down they are both now in long-press mode
  if (b1 == ACTION_LONG_PRESS && button_two.current == STATE_DOWN) {
    button_two.current = STATE_LONG_FIRED;
    b2 = ACTION_LONG_PRESS;
  } else if (b2 == ACTION_LONG_PRESS && button_one.current == STATE_DOWN) {
    button_one.current = STATE_LONG_FIRED;
    b1 = ACTION_LONG_PRESS;
  }
  
  if (b1 == ACTION_LONG_PRESS && b2 == ACTION_LONG_PRESS) {
    return EVENT_BUTTON_ONE_TWO_LONG;
  }

  if (b1) {
    return b1;
  } else if (b2) {
    return b2 + 2;
  } else if (b3) {
    return b3 + 4;
  } else {
    return EVENT_IDLE;
  }
}

void ButtonController::loop() {
  current_event = updateCurrentEvent(); 
}

void ButtonController::setup() {
  if(button_one.pin != -1) {
    pinMode(button_one.pin, INPUT_PULLUP);
  }
  if(button_two.pin != -1) {
    pinMode(button_two.pin, INPUT_PULLUP);
  }
  if(button_three.pin != -1) {
    pinMode(button_three.pin, INPUT_PULLUP);
  }
}
