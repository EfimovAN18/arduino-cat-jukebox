#pragma once
#include <Arduino.h>

// Пользовательские символы 5x8 для LCD 1602
enum Sprite : uint8_t { CAT_STAND = 0, CAT_JUMP = 1, MUSIC_NOTE = 2 };

byte SPRITE_CAT_STAND[8] = {
  0b10001, 0b11111, 0b10101, 0b11111,
  0b01110, 0b11111, 0b01010, 0b11011
};

byte SPRITE_CAT_JUMP[8] = {
  0b10001, 0b11111, 0b10101, 0b11111,
  0b11111, 0b01110, 0b10001, 0b00000
};

byte SPRITE_NOTE[8] = {
  0b00100, 0b00110, 0b00101, 0b00100,
  0b01100, 0b11100, 0b11000, 0b00000
};
