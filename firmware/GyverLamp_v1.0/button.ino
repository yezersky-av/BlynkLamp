boolean brightDirection;

void buttonTick() {
  touch.tick();
  if (touch.isSingle()) {
    if (dawnFlag) {
      manualOff = true;
      dawnFlag = false;
    } else {
      if (ONflag) {
        ONflag = false;
        changePower();
      } else {
        ONflag = true;
        changePower();
      }
    }
  }

  if (touch.isDouble()) {
    if (++currentMode >= MODE_AMOUNT) currentMode = 0;
    loadingFlag = true;
    settChanged = true;
    eepromTimer = millis();
  }

  if (touch.isHolded()) {
    brightDirection = !brightDirection;
  }
  if (touch.isStep()) {
    if (brightDirection) {
      if (modes[currentMode].brightness < 250) modes[currentMode].brightness += 5;
      else modes[currentMode].brightness = 255;
    } else {
      if (modes[currentMode].brightness >= 5) modes[currentMode].brightness -= 5;
      else modes[currentMode].brightness = 0;
    }
    FastLED.setBrightness(modes[currentMode].brightness);
    settChanged = true;
    eepromTimer = millis();
  }
}
