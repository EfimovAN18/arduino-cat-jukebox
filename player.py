"""
Cat Jukebox — плеер на компьютере.

Слушает джойстик с Arduino, играет mp3 из папки music/ и в такт
битам трека отправляет команду "прыгай" кошкомальчику на LCD.

Запуск:  python player.py            (порт ищется автоматически)
         python player.py --port COM5
"""

import argparse
import json
import sys
import time
from pathlib import Path

import librosa
import pygame
import serial
from serial.tools import list_ports

MUSIC_DIR = Path(__file__).parent / "music"

# Направление джойстика -> (файл в music/, название на экране, до 13 символов латиницей)
SLOTS = {
    "L": ("left.mp3",  "LIZA"),
    "R": ("right.mp3", "BOYCHIK"),
    "U": ("up.mp3",    "LISTEN2HEART"),
}

LATENCY_S = 0.05  # сдвиг прыжка, если кажется что котик опаздывает/спешит


def find_port() -> str:
    for p in list_ports.comports():
        desc = f"{p.description} {p.manufacturer}".lower()
        if any(k in desc for k in ("ch340", "arduino", "usb-serial", "wch")):
            return p.device
    ports = [p.device for p in list_ports.comports()]
    if not ports:
        sys.exit("Arduino не найдена. Подключи плату или укажи --port.")
    return ports[0]


def load_beats(track: Path) -> list[float]:
    """Находит биты трека и кэширует их рядом, чтобы не считать каждый раз."""
    cache = track.with_suffix(".beats.json")
    if cache.exists() and cache.stat().st_mtime >= track.stat().st_mtime:
        return json.loads(cache.read_text())

    print(f"  анализирую биты: {track.name} ...")
    y, sr = librosa.load(track, sr=22050, mono=True)
    _, frames = librosa.beat.beat_track(y=y, sr=sr)
    beats = librosa.frames_to_time(frames, sr=sr).tolist()
    cache.write_text(json.dumps(beats))
    return beats


def main():
    parser = argparse.ArgumentParser(description="Cat Jukebox player")
    parser.add_argument("--port", help="COM-порт Arduino, например COM5")
    args = parser.parse_args()

    beats_by_slot = {}
    for key, (filename, _) in SLOTS.items():
        track = MUSIC_DIR / filename
        if track.exists():
            beats_by_slot[key] = load_beats(track)
        else:
            print(f"  нет файла music/{filename} — слот {key} пустой")

    port = args.port or find_port()
    ser = serial.Serial(port, 9600, timeout=0)
    time.sleep(2)  # Arduino перезагружается при открытии порта
    print(f"Подключено к {port}. Двигай джойстик! (Ctrl+C — выход)")

    pygame.mixer.init()
    beats: list[float] = []
    next_beat = 0
    playing = False
    buf = b""

    def send(cmd: str):
        ser.write((cmd + "\n").encode())

    try:
        while True:
            # --- команды от джойстика ---
            buf += ser.read(64)
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                cmd = line.decode(errors="ignore").strip()

                if cmd in beats_by_slot:
                    filename, title = SLOTS[cmd]
                    pygame.mixer.music.load(MUSIC_DIR / filename)
                    pygame.mixer.music.play()
                    beats, next_beat, playing = beats_by_slot[cmd], 0, True
                    send(f"N:{title}")
                    print(f"▶ {title}")
                elif cmd == "S" and playing:
                    pygame.mixer.music.stop()
                    playing = False
                    send("X")
                    print("■ стоп")

            # --- прыжки в такт ---
            if playing:
                if not pygame.mixer.music.get_busy():
                    playing = False
                    send("X")
                else:
                    pos = pygame.mixer.music.get_pos() / 1000 + LATENCY_S
                    while next_beat < len(beats) and beats[next_beat] <= pos:
                        send("B")
                        next_beat += 1

            time.sleep(0.005)
    except KeyboardInterrupt:
        send("X")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
