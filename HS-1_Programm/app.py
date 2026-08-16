from ursina import *
import serial
import json

# ⚠️ Вкажи свій COM-порт від Плеча (подивись у PlatformIO, наприклад 'COM3' або 'COM4')
PORT = 'COM4'
BAUD = 115200

# Підключення до USB-порту
try:
    ser = serial.Serial(PORT, BAUD, timeout=0.01)
    print(f"✅ УСПІШНО ПІДКТЮЧЕНО ДО КОСТЮМА НА {PORT}")
except Exception as e:
    print(f"⚠️ COM-порт не підключено або зайнятий: {e}")
    ser = None

# Створення 3D-вікна
app = Ursina(title="Kinematic Suit — 3D Arm Visualizer", borderless=False)

# --- 3D СЕГМЕНТИ РУКИ (З'єднані в каскад) ---
# 1. Плече (батьківський об'єкт)
shoulder = Entity(model='cube', color=color.azure, scale=(0.3, 1.2, 0.3), position=(0, 0, 0))

# 2. Передпліччя (прикріплене до низу плеча)
forearm = Entity(model='cube', color=color.orange, scale=(0.8, 0.9, 0.8), parent=shoulder, position=(0, -1.1, 0))

# 3. Кисть (прикріплена до низу передпліччя)
hand = Entity(model='cube', color=color.lime, scale=(0.8, 0.5, 0.8), parent=forearm, position=(0, -0.9, 0))

# Камера та координатна сітка
EditorCamera()
Grid(color=color.gray, size=10)

def update():
    if ser and ser.is_open and ser.in_waiting:
        try:
            line = ser.readline().decode('utf-8').strip()
            if line.startswith('{') and line.endswith('}'):
                data = json.loads(line)
                
                # Оновлення кутів плеча (Pitch, Yaw, Roll)
                sh = data.get('shoulder', {})
                shoulder.rotation = (sh.get('p', 0), sh.get('y', 0), -sh.get('r', 0))
                
                # Оновлення кутів передпліччя
                fa = data.get('forearm', {})
                forearm.rotation = (fa.get('p', 0), fa.get('y', 0), -fa.get('r', 0))
                
                # Оновлення кутів кисті
                hd = data.get('hand', {})
                hand.rotation = (hd.get('p', 0), hd.get('y', 0), -hd.get('r', 0))
        except Exception:
            pass

app.run()