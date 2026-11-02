#pragma once
#include <Arduino.h>
#include "pitches.h"

// Нота: частота и длительность в "долях" (доля задаётся у песни)
struct Note {
  int  freq;
  byte units;
};

struct Song {
  const char* name;   // до 13 символов, латиница (LCD не знает кириллицу)
  const Note* notes;
  byte        length;
  int         unitMs; // длительность одной доли, мс
};

// Л. ван Бетховен — «Ода к радости»
const Note ODE_TO_JOY[] = {
  {NOTE_E4,2},{NOTE_E4,2},{NOTE_F4,2},{NOTE_G4,2},{NOTE_G4,2},{NOTE_F4,2},{NOTE_E4,2},{NOTE_D4,2},
  {NOTE_C4,2},{NOTE_C4,2},{NOTE_D4,2},{NOTE_E4,2},{NOTE_E4,3},{NOTE_D4,1},{NOTE_D4,4},
  {NOTE_E4,2},{NOTE_E4,2},{NOTE_F4,2},{NOTE_G4,2},{NOTE_G4,2},{NOTE_F4,2},{NOTE_E4,2},{NOTE_D4,2},
  {NOTE_C4,2},{NOTE_C4,2},{NOTE_D4,2},{NOTE_E4,2},{NOTE_D4,3},{NOTE_C4,1},{NOTE_C4,4}
};

// Русская народная — «Коробейники»
const Note KOROBEINIKI[] = {
  {NOTE_E5,2},{NOTE_B4,1},{NOTE_C5,1},{NOTE_D5,2},{NOTE_C5,1},{NOTE_B4,1},{NOTE_A4,2},{NOTE_A4,1},{NOTE_C5,1},
  {NOTE_E5,2},{NOTE_D5,1},{NOTE_C5,1},{NOTE_B4,3},{NOTE_C5,1},{NOTE_D5,2},{NOTE_E5,2},{NOTE_C5,2},{NOTE_A4,2},{NOTE_A4,4}
};

// Народная — «Twinkle Twinkle Little Star»
const Note TWINKLE[] = {
  {NOTE_C4,1},{NOTE_C4,1},{NOTE_G4,1},{NOTE_G4,1},{NOTE_A4,1},{NOTE_A4,1},{NOTE_G4,2},
  {NOTE_F4,1},{NOTE_F4,1},{NOTE_E4,1},{NOTE_E4,1},{NOTE_D4,1},{NOTE_D4,1},{NOTE_C4,2}
};

#define SONG(name, arr, unit) { name, arr, sizeof(arr) / sizeof(Note), unit }

const Song SONGS[] = {
  SONG("ODE TO JOY",   ODE_TO_JOY,  250),  // джойстик влево  -> слот для FACE «Лиза»
  SONG("KOROBEINIKI",  KOROBEINIKI, 170),  // джойстик вправо -> слот для Френдзона «Бойчик»
  SONG("TWINKLE STAR", TWINKLE,     400),  // джойстик вверх  -> слот для «Listen to Your Heart»
};
