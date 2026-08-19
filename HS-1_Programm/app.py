import json
import os
import serial
from ursina import *

SERIAL_PORT = 'COM4'
BAUD_RATE = 115200

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.01)
    print(f"[OK] Порт {SERIAL_PORT} успішно відкрито")
except Exception as e:
    print(f"[ІНФО] COM-порт не підключено ({e}). Візуалізатор працює у тестовому режимі.")
    ser = None

app = Ursina(title="HS-1 Kinematic Suit - Realtime Visualizer")

# Освітлення для об'ємного рендеру 3D-деталей
DirectionalLight(color=color.white, y=2, z=-3)
AmbientLight(color=color.rgba(120, 120, 120, 0.6))

def load_part(path, color_val):
    if os.path.exists(path):
        return Entity(model=path, color=color_val, scale=0.001)
    else:
        return Entity(model='cube', color=color_val, scale=(0.2, 0.8, 0.2))

# 1. Завантаження деталей
shoulder = load_part('Models/shoulder.obj', color.azure)
forearm = load_part('Models/forearm.obj', color.orange)
hand = load_part('Models/hand.obj', color.lime)

# 2. Налаштування кінематичного зв'язку (Плече -> Передпліччя -> Кисть)
shoulder.position = (0, 0, 0)

# Прив'язуємо передпліччя до плеча та зсуваємо його в точку суглоба (ліктя)
forearm.parent = shoulder
forearm.position = (0, -0.28, 0)  # Відстань 28 см вниз від плеча (відкоригуйте під свій розмір)

# Прив'язуємо кисть до передпліччя та зсуваємо в точку зап'ястя
hand.parent = forearm
hand.position = (0, -0.24, 0)     # Відстань 24 см вниз від ліктя

EditorCamera()
Entity(model=Grid(20, 20), color=color.gray)

Text(text="Затисніть: [1] - Плече | [2] - Лікоть | [3] - Кисть", position=(-0.85, 0.45), scale=1.1)

def update():
    # Почергова перевірка обертання з клавіатури
    if held_keys['1']:
        shoulder.rotation_z += 1  # Повертає ВСЮ руку (плече, передпліччя і кисть)
    if held_keys['2']:
        forearm.rotation_z += 1   # Повертає тільки передпліччя та кисть
    if held_keys['3']:
        hand.rotation_z += 1      # Повертає тільки кисть

    # Прийом даних з ESP32 костюма HS-1
    if ser and ser.is_open and ser.in_waiting:
        try:
            raw_line = ser.readline().decode('utf-8').strip()
            if raw_line.startswith('{') and raw_line.endswith('}'):
                data = json.loads(raw_line)
                
                if 'shoulder' in data:
                    s = data['shoulder']
                    shoulder.rotation = (s.get('p', 0), s.get('y', 0), -s.get('r', 0))
                
                if 'forearm' in data:
                    f = data['forearm']
                    forearm.rotation = (f.get('p', 0), f.get('y', 0), -f.get('r', 0))
                    
                if 'hand' in data:
                    h = data['hand']
                    hand.rotation = (h.get('p', 0), h.get('y', 0), -h.get('r', 0))
        except Exception:
            pass

app.run()