import json
import os
import serial
import threading
from ursina import *

# -------------------------------------------------------------
# 1. SERIAL CONFIG & THREADING (NON-BLOCKING)
# -------------------------------------------------------------
SERIAL_PORT = 'COM3'
BAUD_RATE = 115200

# Глобальний словник для збереження останніх кутів з ESP32
latest_data = {
    'shoulder': {'p': 0.0, 'y': 0.0, 'r': 0.0},
    'forearm':  {'p': 0.0, 'y': 0.0, 'r': 0.0},
    'hand':     {'p': 0.0, 'y': 0.0, 'r': 0.0}
}

def serial_reader_thread():
    global latest_data
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
        print(f"[OK] Serial thread started on {SERIAL_PORT}")
        while True:
            if ser.is_open and ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line.startswith('{') and line.endswith('}'):
                    try:
                        parsed = json.loads(line)
                        for key in ['shoulder', 'forearm', 'hand']:
                            if key in parsed:
                                latest_data[key] = parsed[key]
                        print(f"[RECV] {parsed}")
                    except json.JSONDecodeError:
                        pass
    except Exception as e:
        print(f"[INFO] Serial disconnected: {e}. Running manual test mode.")

# Запуск зчитування з COM-порту в фоновому потоці
t = threading.Thread(target=serial_reader_thread, daemon=True)
t.start()

# -------------------------------------------------------------
# 2. URSINA SCENE SETUP
# -------------------------------------------------------------
app = Ursina(title="HS-1 Kinematic Visualizer")

DirectionalLight(color=color.white, y=2, z=-3)
AmbientLight(color=color.rgba(140, 140, 140, 0.7))

# Батьківський вузол
arm_root = Entity(position=(0, -0.5, 0))

def load_cad_part(path, color_val):
    if os.path.exists(path):
        return Entity(model=path, color=color_val, scale=0.001, parent=arm_root)
    else:
        return Entity(model='cube', color=color_val, scale=(0.1, 0.4, 0.1), parent=arm_root)

# Завантажуємо всі 3 частини в один контейнер arm_root
shoulder = load_cad_part('Models/shoulder.obj', color.azure)
forearm  = load_cad_part('Models/forearm.obj', color.orange)
hand     = load_cad_part('Models/hand.obj', color.lime)

EditorCamera()
Entity(model=Grid(30, 30), color=color.gray)

Text(text="[1/2/3] Manual Test | WASD - Camera | Realtime UART active", position=(-0.85, 0.45), scale=1.0)

# -------------------------------------------------------------
# 3. UPDATE LOOP
# -------------------------------------------------------------
def update():
    # 1. Ручна перевірка клавішами
    if held_keys['1']: shoulder.rotation_x += 2
    if held_keys['2']: forearm.rotation_x += 2
    if held_keys['3']: hand.rotation_x += 2

    # 2. Застосування кутів з фонового потоку Serial
    s = latest_data['shoulder']
    f = latest_data['forearm']
    h = latest_data['hand']

    # Явне оновлення кутів через окремі деталі
    shoulder.rotation_x = s.get('p', 0)
    shoulder.rotation_y = s.get('y', 0)
    shoulder.rotation_z = -s.get('r', 0)

    forearm.rotation_x = f.get('p', 0)
    forearm.rotation_y = f.get('y', 0)
    forearm.rotation_z = -f.get('r', 0)

    hand.rotation_x = h.get('p', 0)
    hand.rotation_y = h.get('y', 0)
    hand.rotation_z = -h.get('r', 0)

app.run()