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
import numpy as np
import pygame
import serial
from serial.tools import list_ports

MUSIC_DIR = Path(__file__).parent / "music"

# Направление джойстика -> (файл в music/, название на экране).
# Название латиницей (LCD не знает кириллицу), до 32 символов —
# если длиннее 13, на экране оно побежит строкой.
SLOTS = {
    "L": ("left.mp3",  "FACE - LIZA"),
    "R": ("right.mp3", "FRENDZONA - BOYCHIK"),
    "U": ("up.mp3",    "ROXETTE - LISTEN TO YOUR HEART"),
}

VOLUME_STEPS = [100, 75, 50, 25, 0]  # джойстик вниз переключает по кругу

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


def analyze(track: Path) -> dict:
    """Находит темп и биты трека, кэширует рядом, чтобы не считать каждый раз."""
    cache = track.with_suffix(".beats.json")
    if cache.exists() and cache.stat().st_mtime >= track.stat().st_mtime:
        data = json.loads(cache.read_text())
        if isinstance(data, dict):
            return data

    print(f"  анализирую биты: {track.name} ...")
    y, sr = librosa.load(track, sr=22050, mono=True)
    tempo, frames = librosa.beat.beat_track(y=y, sr=sr)
    data = {
        "tempo": round(float(np.atleast_1d(tempo)[0])),
        "beats": librosa.frames_to_time(frames, sr=sr).tolist(),
    }
    cache.write_text(json.dumps(data))
    return data


def main():
    parser = argparse.ArgumentParser(description="Cat Jukebox player")
    parser.add_argument("--port", help="COM-порт Arduino, например COM5")
    args = parser.parse_args()

    tracks = {}
    for key, (filename, _) in SLOTS.items():
        track = MUSIC_DIR / filename
        if track.exists():
            tracks[key] = analyze(track)
            print(f"  {filename}: {tracks[key]['tempo']} BPM")
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
    vol_i = 0
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

                if cmd in tracks:
                    filename, title = SLOTS[cmd]
                    info = tracks[cmd]
                    pygame.mixer.music.load(MUSIC_DIR / filename)
                    pygame.mixer.music.play()
                    pygame.mixer.music.set_volume(VOLUME_STEPS[vol_i] / 100)
                    beats, next_beat, playing = info["beats"], 0, True
                    send(f"N:{title}")
                    send(f"T:{info['tempo']}")
                    print(f"▶ {title} ({info['tempo']} BPM)")
                elif cmd == "D":
                    vol_i = (vol_i + 1) % len(VOLUME_STEPS)
                    pygame.mixer.music.set_volume(VOLUME_STEPS[vol_i] / 100)
                    send(f"V:{VOLUME_STEPS[vol_i]}")
                    print(f"🔊 {VOLUME_STEPS[vol_i]}%")
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
