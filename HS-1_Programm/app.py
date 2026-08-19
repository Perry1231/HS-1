import json
import os
import serial
from ursina import *

SERIAL_PORT = 'COM4'
BAUD_RATE = 115200

# 1. Безпечне підключення до COM-порту
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.01)
    print(f"[OK] Порт {SERIAL_PORT} успішно відкрито")
except Exception as e:
    print(f"[ІНФО] COM-порт не підключено ({e}). Візуалізатор працює у тестовому режимі.")
    ser = None

app = Ursina(title="HS-1 Kinematic Suit - Realtime Visualizer")

# 2. Функція перевірки наявності 3D-файлу
def get_model(path):
    if os.path.exists(path):
        print(f"[OK] Знайдено 3D-модель: {path}")
        return path
    else:
        print(f"[УВАГА] Файл {path} не знайдено! Використовуємо тимчасовий куб.")
        return 'cube'

sh_model = get_model('Models/shoulder.obj')
fo_model = get_model('Models/forearm.obj')
ha_model = get_model('Models/hand.obj')

# 3. Налаштування масштабу залежно від типу моделі
sh_scale = 0.001 if sh_model != 'cube' else (0.3, 1.2, 0.3)
fo_scale = 1 if fo_model != 'cube' else (0.8, 0.9, 0.8)
ha_scale = 1 if ha_model != 'cube' else (0.8, 0.5, 0.8)

fo_pos = (0, -0.28, 0) if sh_model != 'cube' else (0, -1.1, 0)
ha_pos = (0, -0.24, 0) if fo_model != 'cube' else (0, -0.9, 0)

# 4. Створення об'єктів
shoulder = Entity(model=sh_model, color=color.azure, scale=sh_scale, position=(0, 0, 0))
forearm = Entity(model=fo_model, color=color.orange, scale=fo_scale, parent=shoulder, position=fo_pos)
hand = Entity(model=ha_model, color=color.lime, scale=ha_scale, parent=forearm, position=ha_pos)

EditorCamera()
Grid(color=color.gray, size=10)

def update():
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