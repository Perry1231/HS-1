from ursina import *
import serial
import json

app = Ursina()

# --- 1. ПІДЛОГА ТА СІТКА ---
Entity(model='grid', scale=50, color=color.gray)

# --- 2. СУГЛОБИ ТА МОДЕЛІ ---
shoulder_pivot = Entity(position=(0, 5, 0))
shoulder_model = Entity(
    parent=shoulder_pivot,
    model='Models/Biceps_2_0.stl',
    scale=1.0,
    color=color.azure,
    double_sided=True
)

forearm_pivot = Entity(parent=shoulder_pivot, position=(0, -5, 0))
forearm_model = Entity(
    parent=forearm_pivot,
    model='Models/Forearm_2_0.stl',
    scale=1.0,
    color=color.lime,
    double_sided=True
)

# Яскрава рожева модель кисті
# 3. КИСТЬ
hand_pivot = Entity(parent=forearm_pivot, position=(0, -5, 0)) # зміщення зап'ястя від ліктя

hand_model = Entity(
    parent=hand_pivot,
    model='Models/Hand_2_0.stl',
    scale=0.01,             # Перевірте масштаб (0.01, 0.1 або 1.0)
    color=color.magenta,
    double_sided=True,
    origin=(0, 0, 0)        # Встановлюємо центрування моделі відносно pivot
)

# Маркер-сфера в точці суглоба Кисті (допомагає знайти її у просторі)
hand_marker = Entity(parent=hand_pivot, model='sphere', scale=0.3, color=color.yellow)

# --- 3. КАМЕРА ТА СВІТЛО ---
cam = EditorCamera()
cam.position = (0, 0, -30)

DirectionalLight(y=3, z=-5, rotation=(45, -45, 0))
AmbientLight(color=color.rgba(150, 150, 150, 0.8))

# --- 4. SERIAL ---
try:
    ser = serial.Serial('COM3', 115200, timeout=0.05)
except Exception as e:
    print(f"Помилка COM-порту: {e}")
    ser = None

use_serial = True if ser else False

# --- 5. ІНТЕРАКТИВНЕ КЕРУВАННЯ ---
def update():
    global use_serial
    
    # А. Читання з Serial
    if use_serial and ser and ser.in_waiting > 0:
        try:
            line = ser.readline().decode('utf-8').strip()
            if line.startswith('{') and line.endswith('}'):
                data = json.loads(line)
                if 'shoulder' in data:
                    s = data['shoulder']
                    shoulder_pivot.rotation = Vec3(s['p'], s['y'], s['r'])
                if 'forearm' in data:
                    f = data['forearm']
                    forearm_pivot.rotation = Vec3(f['p'], f['y'], f['r'])
                if 'hand' in data:
                    h = data['hand']
                    hand_pivot.rotation = Vec3(h['p'], h['y'], h['r'])
        except Exception:
            pass

    # Б. Ручне обертання суглобів
    if held_keys['w']: shoulder_pivot.rotation_x += 2
    if held_keys['s']: shoulder_pivot.rotation_x -= 2
    if held_keys['a']: shoulder_pivot.rotation_y += 2
    if held_keys['d']: shoulder_pivot.rotation_y -= 2
    
    if held_keys['up arrow']:    forearm_pivot.rotation_x += 2
    if held_keys['down arrow']:  forearm_pivot.rotation_x -= 2
    if held_keys['left arrow']:  forearm_pivot.rotation_y += 2
    if held_keys['right arrow']: forearm_pivot.rotation_y -= 2

    if held_keys['i']: hand_pivot.rotation_x += 2
    if held_keys['k']: hand_pivot.rotation_x -= 2
    if held_keys['j']: hand_pivot.rotation_y += 2
    if held_keys['l']: hand_pivot.rotation_y -= 2

    # В. ПОШУК І ЗСУВ КИСТІ У ПРОСТОРІ (Клавіші 1, 2, 3, 4, 5, 6)
    if held_keys['1']: hand_pivot.y -= 0.1  # Рухати кисть ВНИЗ
    if held_keys['2']: hand_pivot.y += 0.1  # Рухати кисть ВГОРУ
    if held_keys['3']: hand_pivot.x -= 0.1  # Рухати ВЛІВО
    if held_keys['4']: hand_pivot.x += 0.1  # Рухати ВПРАВО
    if held_keys['5']: hand_pivot.z -= 0.1  # Рухати НАЗАД
    if held_keys['6']: hand_pivot.z += 0.1  # Рухати ВПЕРЕД

def input(key):
    global use_serial
    if key == 'space':
        shoulder_pivot.rotation = (0, 0, 0)
        forearm_pivot.rotation = (0, 0, 0)
        hand_pivot.rotation = (0, 0, 0)
    if key == 'tab':
        use_serial = not use_serial

app.run()