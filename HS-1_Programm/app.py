import json
import serial
from ursina import *

# COM port configuration (change the port to match your ESP32)SERIAL_PORT = 'COM4'
BAUD_RATE = 115200

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.01)
    print(f"[OK] Порт {SERIAL_PORT} успішно відкрито")
except Exception as e:
    print(f"[УВАГА] Не вдалося відкрити порт {SERIAL_PORT}: {e}")
    ser = None

app = Ursina(title="HS-1 Kinematic Suit - Realtime Visualizer")

# Loading 3D models
# scale=0.001 converts millimeters from Fusion 360 to meters for Ursina
# Завантаження 3D-моделей з підпапки Models
# scale=0.001 переводить міліметри з Fusion 360 у метри для Ursina
shoulder = Entity(model='Models/shoulder.obj', color=color.azure, scale=0.001, position=(0, 0, 0))
forearm = Entity(model='Models/forearm.obj', color=color.orange, scale=1, parent=shoulder, position=(0, -0.28, 0))
hand = Entity(model='Models/hand.obj', color=color.lime, scale=1, parent=forearm, position=(0, -0.24, 0))

EditorCamera()
Grid(color=color.gray, size=10)

def update():
    if ser and ser.is_open and ser.in_waiting:
        try:
            raw_line = ser.readline().decode('utf-8').strip()
            if raw_line.startswith('{') and raw_line.endswith('}'):
                data = json.loads(raw_line)
                
                #Pitch, Yaw, Roll
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