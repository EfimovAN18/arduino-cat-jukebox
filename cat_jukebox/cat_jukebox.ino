/*
 * Cat Jukebox — музыкальная шкатулка с танцующим котиком.
 * Джойстик выбирает мелодию, пищалка играет её,
 * а котик на LCD 1602 прыгает в такт каждой ноте.
 *
 * Автор: Алексей Ефимов (github.com/EfimovAN18)
 */

#include <LiquidCrystal.h>
#include "songs.h"
#include "sprites.h"

// ---------- пины ----------
const uint8_t PIN_BUZZER = 8;
const uint8_t PIN_JOY_X  = A0;
const uint8_t PIN_JOY_Y  = A1;
const uint8_t PIN_JOY_SW = 7;

// ---------- настройки ----------
const int JOY_LOW  = 200;   // порог наклона джойстика
const int JOY_HIGH = 800;
const uint8_t CAT_COL = 14; // колонка котика на экране

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);  // RS, E, D4, D5, D6, D7

int  currentSong = -1;      // -1 = тишина
byte noteIndex   = 0;
unsigned long noteEndsAt = 0;
bool catInAir = false;

void drawCat() {
  lcd.setCursor(CAT_COL, 0);
  lcd.write(catInAir ? (uint8_t)CAT_JUMP : (uint8_t)' ');
  lcd.setCursor(CAT_COL, 1);
  lcd.write(catInAir ? (uint8_t)' ' : (uint8_t)CAT_STAND);

  // нотка «летает» с противоположной от котика стороны
  lcd.setCursor(CAT_COL + 1, catInAir ? 1 : 0);
  lcd.write((uint8_t)MUSIC_NOTE);
  lcd.setCursor(CAT_COL + 1, catInAir ? 0 : 1);
  lcd.write((uint8_t)' ');
}

void showIdleScreen() {
  lcd.clear();
  lcd.print("MOVE THE STICK!");
  lcd.setCursor(CAT_COL, 1);
  lcd.write((uint8_t)CAT_STAND);
}

void startSong(int index) {
  if (index == currentSong) return;   // уже играет — не перезапускаем
  currentSong = index;
  noteIndex   = 0;
  noteEndsAt  = 0;

  lcd.clear();
  lcd.print(SONGS[index].name);
  lcd.setCursor(0, 1);
  lcd.print("meow~");
}

void stopSong() {
  currentSong = -1;
  catInAir = false;
  noTone(PIN_BUZZER);
  showIdleScreen();
}

void handleJoystick() {
  int x = analogRead(PIN_JOY_X);
  int y = analogRead(PIN_JOY_Y);

  if      (x < JOY_LOW)  startSong(0);
  else if (x > JOY_HIGH) startSong(1);
  else if (y < JOY_LOW)  startSong(2);

  if (digitalRead(PIN_JOY_SW) == LOW && currentSong >= 0) stopSong();
}

// Неблокирующее проигрывание: пока звучит нота, loop() продолжает работать
void updateMusic() {
  if (currentSong < 0 || millis() < noteEndsAt) return;

  const Song& song = SONGS[currentSong];
  if (noteIndex >= song.length) {
    stopSong();
    return;
  }

  Note note = song.notes[noteIndex++];
  unsigned long duration = (unsigned long)note.units * song.unitMs;
  tone(PIN_BUZZER, note.freq, duration * 9 / 10);  // короткая пауза между нотами
  noteEndsAt = millis() + duration;

  catInAir = !catInAir;
  drawCat();
}

void setup() {
  lcd.begin(16, 2);
  lcd.createChar(CAT_STAND,  SPRITE_CAT_STAND);
  lcd.createChar(CAT_JUMP,   SPRITE_CAT_JUMP);
  lcd.createChar(MUSIC_NOTE, SPRITE_NOTE);

  pinMode(PIN_JOY_SW, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);

  showIdleScreen();
}

void loop() {
  handleJoystick();
  updateMusic();
}
