from ursina import *
import serial
import json

app = Ursina()

# --- 1. ПІДЛОГА ТА СІТКА ---
Entity(model='grid', scale=50, color=color.gray)

# --- 2. СУГЛОБИ ТА МОДЕЛІ ---
# --- 1. ПЛЕЧЕ (Блакитна деталь вгорі) ---
shoulder_pivot = Entity(position=(0, 6, 0))
shoulder_model = Entity(
    parent=shoulder_pivot,
    model='Models/Biceps_2_0.stl',
    scale=1.0,
    color=color.azure,
    double_sided=True,
    position=(0, -2.5, 0) # Зсув центру деталі для точного Pivot у лікті
)

# --- 2. ПЕРЕДПЛІЧЧЯ (Зелена деталь посередині) ---
# Розміщується точно під біцепсом
forearm_pivot = Entity(parent=shoulder_pivot, position=(0, -235, 0)) 
forearm_model = Entity(
    parent=forearm_pivot,
    model='Models/Forearm_2_0.stl',
    scale=1.0,
    color=color.lime,
    double_sided=True,
    position=(0, -2.0, 0) # Зсув деталі під ліктьовий суглоб
)

# --- 3. КИСТЬ (Рожева деталь знизу) ---
# Розміщується точно під передпліччям
hand_pivot = Entity(parent=forearm_pivot, position=(0, -150, -38))
hand_model = Entity(
    parent=hand_pivot,
    model='Models/Hand_2_0.stl',
    scale=1.0,
    color=color.magenta,
    double_sided=True,
    position=(0, -1.2, 0) # Зсув деталі під зап'ястя
)

# Жовтий маркер у суглобі зап'ястя
Entity(parent=hand_pivot, model='sphere', scale=1, color=color.yellow)

# --- 3. КАМЕРА ТА ОСВІТЛЕННЯ ---
cam = EditorCamera()
cam.position = (0, 0, -30)

DirectionalLight(y=3, z=-5, rotation=(45, -45, 0))
AmbientLight(color=color.rgba(150, 150, 150, 0.8))

# --- 4. SERIAL ЗВ'ЯЗОК ---
try:
    ser = serial.Serial('COM3', 115200, timeout=0.05)
except Exception as e:
    print(f"Помилка COM-порту: {e}")
    ser = None

use_serial = True if ser else False

# Швидкості обертання та переміщення
rotation_speed = 5.0
move_speed = 0.1

def update():
    global use_serial
    
    # -------------------------------------------------------------
    # А. АВТОМАТИЧНЕ ОБЕРТАННЯ ВІД SENSORS (BNO055 / UART JSON)
    # -------------------------------------------------------------
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

    # -------------------------------------------------------------
    # Б. РУЧНЕ КЕРУВАННЯ З КЛАВІАТУРИ
    # -------------------------------------------------------------
    
    # --- 1. ПІДЙОМ / ОПУСКАННЯ (Переміщення по вертикалі) ---
    # T / G — Підняти / Опустити ВСЮ РУКУ (shoulder_pivot)
    if held_keys['t']: shoulder_pivot.y += move_speed
    if held_keys['g']: shoulder_pivot.y -= move_speed
    
    # Y / H — Підняти / Опустити ПЕРЕДПЛІЧЧЯ (forearm_pivot відносно плеча)
    if held_keys['y']: forearm_pivot.y += move_speed
    if held_keys['h']: forearm_pivot.y -= move_speed

    # N / M — Підняти / Опустити КИСТЬ (hand_pivot відносно передпліччя)
    if held_keys['n']: hand_pivot.y += move_speed
    if held_keys['m']: hand_pivot.y -= move_speed

    # --- 2. ОБЕРТАННЯ СУГЛОБІВ ---
    # ПЛЕЧЕ (W/S - Pitch, A/D - Yaw, Q/E - Roll)
    if held_keys['w']: shoulder_pivot.rotation_x += rotation_speed
    if held_keys['s']: shoulder_pivot.rotation_x -= rotation_speed
    if held_keys['a']: shoulder_pivot.rotation_y += rotation_speed
    if held_keys['d']: shoulder_pivot.rotation_y -= rotation_speed
    if held_keys['q']: shoulder_pivot.rotation_z += rotation_speed
    if held_keys['e']: shoulder_pivot.rotation_z -= rotation_speed

    # ПЕРЕДПЛІЧЧЯ (Стрілки)
    if held_keys['up arrow']:    forearm_pivot.rotation_x += rotation_speed
    if held_keys['down arrow']:  forearm_pivot.rotation_x -= rotation_speed
    if held_keys['left arrow']:  forearm_pivot.rotation_y += rotation_speed
    if held_keys['right arrow']: forearm_pivot.rotation_y -= rotation_speed

    # КИСТЬ (I/K - Pitch, J/L - Yaw, U/O - Roll)
    if held_keys['i']: hand_pivot.rotation_x += rotation_speed
    if held_keys['k']: hand_pivot.rotation_x -= rotation_speed
    if held_keys['j']: hand_pivot.rotation_y += rotation_speed
    if held_keys['l']: hand_pivot.rotation_y -= rotation_speed
    if held_keys['u']: hand_pivot.rotation_z += rotation_speed
    if held_keys['o']: hand_pivot.rotation_z -= rotation_speed

def input(key):
    global use_serial
    # Скидання кутів у 0
    if key == 'space':
        shoulder_pivot.rotation = (0, 0, 0)
        forearm_pivot.rotation = (0, 0, 0)
        hand_pivot.rotation = (0, 0, 0)
    # Перемикач режимів Датчики / Клавіатура
    if key == 'tab':
        use_serial = not use_serial

app.run()