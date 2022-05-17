class Periodic {
  private:
    long last_fire = 0;
    void (*func)();
    long period;
  public:
    Periodic(void (*trigger)(), long period);
    void loop();
};

Periodic::Periodic(void (*trigger)(), long period) {
  func = trigger;
  this->period = period;
}

void Periodic::loop() {
  if ((millis() - last_fire) > period) {
    func();
    last_fire = millis();
  }
}
