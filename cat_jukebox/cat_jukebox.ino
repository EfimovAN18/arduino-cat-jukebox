/*
 * Cat Jukebox — кошкомальчик, который танцует под твою музыку.
 *
 * Arduino отвечает за джойстик и экран, музыку играет player.py на компьютере.
 * Протокол по Serial (9600 бод, строки):
 *   Arduino -> ПК:  L / R / U — выбрать трек, D — громкость, S — стоп
 *   ПК -> Arduino:  N:<название> — начался трек (длинное название бежит строкой)
 *                   T:<bpm>      — темп трека, от него зависит танец
 *                   B            — бит, следующий кадр танца
 *                   V:<0..100>   — громкость, показать шкалу
 *                   X            — музыка остановилась
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

const uint8_t CAT_COL = 14;              // кошкомальчик в колонке 14, нотка в 15
const uint8_t TEXT_W  = 13;              // ширина текстовой области
const unsigned long SCROLL_MS   = 350;   // скорость бегущей строки
const unsigned long VOL_SHOW_MS = 1500;  // сколько показывать шкалу громкости

enum Dance : uint8_t { SWAY, JUMP, PARTY };
const char* const DANCE_NAMES[] = { "SWAY", "JUMP", "PARTY" };

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);  // RS, E, D4, D5, D6, D7

// джойстик
char lastDir = 0;
bool lastBtn = false;
String rx;

// состояние экрана
bool playing = false;
String title;
uint8_t scrollPos = 0;
unsigned long lastScroll = 0;
int bpm = 0;
Dance dance = JUMP;
uint8_t frame = 0;
int volume = 100;
bool volShown = false;
unsigned long volUntil = 0;

// ---------- отрисовка ----------

void putAt(uint8_t col, uint8_t row, uint8_t ch) {
  lcd.setCursor(col, row);
  lcd.write(ch);
}

void printPadded(uint8_t row, const String& s) {
  lcd.setCursor(0, row);
  for (uint8_t i = 0; i < TEXT_W; i++)
    lcd.write(i < s.length() ? (uint8_t)s[i] : (uint8_t)' ');
}

void drawTitle() {
  if (title.length() <= TEXT_W) {
    printPadded(0, title);
    return;
  }
  String ring = title + "   " + title;   // зацикленная лента
  printPadded(0, ring.substring(scrollPos, scrollPos + TEXT_W));
}

void drawInfo() {
  if (volShown) {
    String bar = "VOL ";
    uint8_t filled = (volume * 9 + 50) / 100;  // 0..9 делений
    for (uint8_t i = 0; i < 9; i++) bar += (i < filled) ? (char)0xFF : '-';
    printPadded(1, bar);
  } else if (playing) {
    printPadded(1, String(bpm) + "BPM " + DANCE_NAMES[dance]);
  } else {
    printPadded(1, "");
  }
}

void drawCat() {
  uint8_t top = ' ', bottom = CAT_STAND;

  if (playing) {
    switch (dance) {
      case SWAY:   // медленно: качается влево-вправо
        bottom = (frame % 2) ? CAT_LEAN_R : CAT_LEAN_L;
        break;
      case JUMP:   // средний темп: прыгает
        if (frame % 2) { top = CAT_JUMP; bottom = ' '; }
        break;
      case PARTY:  // быстро: руки вверх — прыжок — руки вверх — стойка
        switch (frame % 4) {
          case 0: case 2: bottom = CAT_ARMS_UP; break;
          case 1: top = CAT_JUMP; bottom = ' '; break;
        }
        break;
    }
  }

  putAt(CAT_COL, 0, top);
  putAt(CAT_COL, 1, bottom);

  bool noteUp = frame % 2 == 0;
  putAt(CAT_COL + 1, 0, playing && noteUp  ? MUSIC_NOTE : ' ');
  putAt(CAT_COL + 1, 1, playing && !noteUp ? MUSIC_NOTE : ' ');
}

void showIdle() {
  playing = false;
  frame = 0;
  title = "";
  printPadded(0, "MOVE STICK!");
  drawInfo();
  drawCat();
}

// ---------- команды от компьютера ----------

void handleCommand(const String& cmd) {
  if (cmd == "B") {
    if (playing) { frame++; drawCat(); }
  } else if (cmd == "X") {
    showIdle();
  } else if (cmd.startsWith("N:")) {
    playing = true;
    title = cmd.substring(2);
    scrollPos = 0;
    lastScroll = millis();
    frame = 0;
    drawTitle();
    drawInfo();
    drawCat();
  } else if (cmd.startsWith("T:")) {
    bpm = cmd.substring(2).toInt();
    dance = bpm < 90 ? SWAY : (bpm <= 130 ? JUMP : PARTY);
    drawInfo();
  } else if (cmd.startsWith("V:")) {
    volume = constrain(cmd.substring(2).toInt(), 0, 100);
    volShown = true;
    volUntil = millis() + VOL_SHOW_MS;
    drawInfo();
  }
}

void readSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      rx.trim();
      if (rx.length()) handleCommand(rx);
      rx = "";
    } else if (rx.length() < 40) {
      rx += c;
    }
  }
}

// ---------- джойстик ----------

void handleJoystick() {
  int x = analogRead(PIN_JOY_X);
  int y = analogRead(PIN_JOY_Y);

  char dir = 0;
  if      (x < JOY_LOW)  dir = 'L';
  else if (x > JOY_HIGH) dir = 'R';
  else if (y < JOY_LOW)  dir = 'U';
  else if (y > JOY_HIGH) dir = 'D';

  if (dir && dir != lastDir) Serial.println(dir);  // одна команда на наклон
  lastDir = dir;

  bool btn = digitalRead(PIN_JOY_SW) == LOW;
  if (btn && !lastBtn) Serial.println('S');
  lastBtn = btn;
}

// ---------- фоновые обновления ----------

void updateScroll() {
  if (!playing || title.length() <= TEXT_W) return;
  if (millis() - lastScroll < SCROLL_MS) return;
  lastScroll = millis();
  scrollPos = (scrollPos + 1) % (title.length() + 3);
  drawTitle();
}

void updateVolumeBar() {
  if (volShown && millis() > volUntil) {
    volShown = false;
    drawInfo();
  }
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.createChar(CAT_STAND,   SPRITE_CAT_STAND);
  lcd.createChar(CAT_JUMP,    SPRITE_CAT_JUMP);
  lcd.createChar(MUSIC_NOTE,  SPRITE_NOTE);
  lcd.createChar(CAT_LEAN_L,  SPRITE_CAT_LEAN_L);
  lcd.createChar(CAT_LEAN_R,  SPRITE_CAT_LEAN_R);
  lcd.createChar(CAT_ARMS_UP, SPRITE_CAT_ARMS_UP);
  pinMode(PIN_JOY_SW, INPUT_PULLUP);
  showIdle();
}

void loop() {
  readSerial();
  handleJoystick();
  updateScroll();
  updateVolumeBar();
  delay(10);
}
