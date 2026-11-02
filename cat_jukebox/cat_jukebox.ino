/*
 * Cat Jukebox — кошкомальчик, который танцует под твою музыку.
 *
 * Arduino отвечает за джойстик и экран, музыку играет player.py на компьютере.
 * Протокол по Serial (9600 бод, строки):
 *   Arduino -> ПК:  L / R / U  — наклон джойстика, S — нажатие (стоп)
 *   ПК -> Arduino:  N:<имя>    — началась песня, показать название
 *                   B          — бит, кошкомальчик прыгает
 *                   X          — музыка остановилась
 *
 * Автор: Алексей Ефимов (github.com/EfimovAN18)
 */

#include <LiquidCrystal.h>
#include "sprites.h"

const uint8_t PIN_JOY_X  = A0;
const uint8_t PIN_JOY_Y  = A1;
const uint8_t PIN_JOY_SW = 7;

const int JOY_LOW  = 200;
const int JOY_HIGH = 800;
const uint8_t CAT_COL = 14;

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);  // RS, E, D4, D5, D6, D7

char lastDir = 0;          // чтобы слать команду один раз за наклон
bool lastBtn = false;
bool catInAir = false;
String rx;

void drawCat() {
  lcd.setCursor(CAT_COL, 0);
  lcd.write(catInAir ? (uint8_t)CAT_JUMP : (uint8_t)' ');
  lcd.setCursor(CAT_COL, 1);
  lcd.write(catInAir ? (uint8_t)' ' : (uint8_t)CAT_STAND);
  lcd.setCursor(CAT_COL + 1, catInAir ? 1 : 0);
  lcd.write((uint8_t)MUSIC_NOTE);
  lcd.setCursor(CAT_COL + 1, catInAir ? 0 : 1);
  lcd.write((uint8_t)' ');
}

void showIdle() {
  catInAir = false;
  lcd.clear();
  lcd.print("MOVE THE STICK!");
  lcd.setCursor(CAT_COL, 1);
  lcd.write((uint8_t)CAT_STAND);
}

void showSong(const String& name) {
  catInAir = false;
  lcd.clear();
  lcd.print(name.substring(0, 13));
  lcd.setCursor(0, 1);
  lcd.print("meow~");
  drawCat();
}

void handleCommand(const String& cmd) {
  if (cmd == "B") {
    catInAir = !catInAir;
    drawCat();
  } else if (cmd == "X") {
    showIdle();
  } else if (cmd.startsWith("N:")) {
    showSong(cmd.substring(2));
  }
}

void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      rx.trim();
      if (rx.length()) handleCommand(rx);
      rx = "";
    } else if (rx.length() < 32) {
      rx += c;
    }
  }
}

void handleJoystick() {
  int x = analogRead(PIN_JOY_X);
  int y = analogRead(PIN_JOY_Y);

  char dir = 0;
  if      (x < JOY_LOW)  dir = 'L';
  else if (x > JOY_HIGH) dir = 'R';
  else if (y < JOY_LOW)  dir = 'U';

  if (dir && dir != lastDir) Serial.println(dir);
  lastDir = dir;

  bool btn = digitalRead(PIN_JOY_SW) == LOW;
  if (btn && !lastBtn) Serial.println('S');
  lastBtn = btn;
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.createChar(CAT_STAND,  SPRITE_CAT_STAND);
  lcd.createChar(CAT_JUMP,   SPRITE_CAT_JUMP);
  lcd.createChar(MUSIC_NOTE, SPRITE_NOTE);
  pinMode(PIN_JOY_SW, INPUT_PULLUP);
  showIdle();
}

void loop() {
  readSerial();
  handleJoystick();
  delay(10);
}
