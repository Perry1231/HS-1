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

def get_model(path):
    if os.path.exists(path):
        return path
    else:
        return 'cube'

sh_model = get_model('Models/shoulder.obj')
fo_model = get_model('Models/forearm.obj')
ha_model = get_model('Models/hand.obj')

# Масштабування деталей з мм у м
sh_scale = 0.001 if sh_model != 'cube' else (0.3, 1.2, 0.3)
fo_scale = 0.001 if fo_model != 'cube' else (0.8, 0.9, 0.8)
ha_scale = 0.001 if ha_model != 'cube' else (0.8, 0.5, 0.8)

# 1. Розносимо об'єкти на 3 метри один від одного по осі X
shoulder = Entity(model=sh_model, color=color.azure, scale=sh_scale, position=(-3.0, 0, 0))
forearm = Entity(model=fo_model, color=color.orange, scale=fo_scale, position=(0, 0, 0))
hand = Entity(model=ha_model, color=color.lime, scale=ha_scale, position=(3.0, 0, 0))

# 2. Віддаляємо камеру, щоб охопити всі 3 об'єкти
camera.position = (0, 0, -20)

EditorCamera()
Entity(model=Grid(30, 30), color=color.gray)

Text(text="Клікни на 3D-вікно! Q/A - Shoulder | W/S - Forearm | E/D - Hand", position=(-0.85, 0.45), scale=1.2)

def update():
    # Автоматичне постійне обертання для перевірки рендеру
    shoulder.rotation_y += 0.5
    forearm.rotation_y += 0.5
    hand.rotation_y += 0.5

    # Додаткове керування з клавіатури (натисни мишкою на 3D-вікно перед цим)
    if held_keys['q']: shoulder.rotation_z += 2
    if held_keys['a']: shoulder.rotation_z -= 2
    
    if held_keys['w']: forearm.rotation_z += 2
    if held_keys['s']: forearm.rotation_z -= 2
    
    if held_keys['e']: hand.rotation_z += 2
    if held_keys['d']: hand.rotation_z -= 2

    # Читання даних з ESP32
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