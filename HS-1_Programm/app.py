from ursina import *
import serial
import json

app = Ursina()

# --- 1. FLOOR AND GRID ---
Entity(model='grid', scale=50, color=color.gray)

# --- 2. HIERARCHY OF MODELS (BODY, ARMS AND LEGS) ---

# Base center (Torso / Body)
body_pivot = Entity(position=(0, -150, 0))
body_model = Entity(
    parent=body_pivot,
    model='Models/Body_2_0.stl',
    scale=1.0,
    color=color.orange,
    double_sided=True,
    position=(-90, -190, -285),
    rotation=(90, 0, 90)
)

# --- ARMS (Shoulder attached to the body) ---
shoulder_pivot = Entity(parent=body_pivot, position=(0, -150, -38))
shoulder_model = Entity(
    parent=shoulder_pivot,
    model='Models/Biceps_2_0.stl',
    scale=1.0,
    color=color.azure,
    double_sided=True,
    position=(0, -2.5, 0)
)

forearm_pivot = Entity(parent=shoulder_pivot, position=(0, -235, 0)) 
forearm_model = Entity(
    parent=forearm_pivot,
    model='Models/Forearm_2_0.stl',
    scale=1.0,
    color=color.lime,
    double_sided=True,
    position=(0, -2.0, 0)
)

hand_pivot = Entity(parent=forearm_pivot, position=(0, -150, -38))
hand_model = Entity(
    parent=hand_pivot,
    model='Models/Hand_2_0.stl',
    scale=1.0,
    color=color.magenta,
    double_sided=True,
    position=(0, -1.2, 0)
)
Entity(parent=hand_pivot, model='sphere', scale=1, color=color.yellow)


# --- LEGS (Attached lower to the body) ---

# 1. Upper Leg (Thigh / Leg_Higher)
leg_higher_pivot_1 = Entity(parent=body_pivot, position=(0, -3, 0))
leg_higher_model_1 = Entity(
    parent=leg_higher_pivot_1,
    model='Models/Leg_Higher_2_0.stl',
    scale=1.0,
    color=color.cyan,
    double_sided=True,
    position=(0, -910.0, -420),
    rotation=(270, 180, 90)
)



# 2. Lower Leg (Shin / Leg_Lower)
leg_lower_pivot = Entity(parent=leg_higher_pivot_1, position=(0, -4.0, 0))
leg_lower_model = Entity(
    parent=leg_lower_pivot,
    model='Models/Leg_Lower_2_0.stl',
    scale=1.0,
    color=color.brown,
    double_sided=True,
    position=(0, -500.0, 0)
)

# 3. Foot (Leg_Foot)
leg_foot_pivot = Entity(parent=leg_lower_pivot, position=(0, -4.0, 0))
leg_foot_model = Entity(
    parent=leg_foot_pivot,
    model='Models/Leg_Foot_2_0.stl',
    scale=1.0,
    color=color.red,
    double_sided=True,
    position=(0, -500.0, 0)
)
Entity(parent=leg_foot_pivot, model='sphere', scale=1, color=color.yellow)


# --- 3. CAMERA AND LIGHTING ---
cam = EditorCamera()
cam.position = (0, 0, -40)

DirectionalLight(y=3, z=-5, rotation=(45, -45, 0))
AmbientLight(color=color.rgba(150, 150, 150, 0.8))

# --- 4. SERIAL COMMUNICATION ---
try:
    ser = serial.Serial('COM3', 115200, timeout=0.05)
except Exception as e:
    print(f"COM Port Error: {e}")
    ser = None

use_serial = True if ser else False

# Rotation and movement speeds
rotation_speed = 5.0
move_speed = 0.1

def update():
    global use_serial
    
    # -------------------------------------------------------------
    # A. AUTOMATIC ROTATION FROM SENSORS (UART JSON)
    # -------------------------------------------------------------
    if use_serial and ser and ser.in_waiting > 0:
        try:
            line = ser.readline().decode('utf-8').strip()
            if line.startswith('{') and line.endswith('}'):
                data = json.loads(line)
                if 'body' in data:
                    b = data['body']
                    body_pivot.rotation = Vec3(b['p'], b['y'], b['r'])
                if 'shoulder' in data:
                    s = data['shoulder']
                    shoulder_pivot.rotation = Vec3(s['p'], s['y'], s['r'])
                if 'forearm' in data:
                    f = data['forearm']
                    forearm_pivot.rotation = Vec3(f['p'], f['y'], f['r'])
                if 'hand' in data:
                    h = data['hand']
                    hand_pivot.rotation = Vec3(h['p'], h['y'], h['r'])
                if 'leg_higher' in data:
                    lh = data['leg_higher']
                    leg_higher_pivot_1.rotation = Vec3(lh['p'], lh['y'], lh['r'])
                if 'leg_lower' in data:
                    ll = data['leg_lower']
                    leg_lower_pivot.rotation = Vec3(ll['p'], ll['y'], ll['r'])
                if 'leg_foot' in data:
                    lf = data['leg_foot']
                    leg_foot_pivot.rotation = Vec3(lf['p'], lf['y'], lf['r'])
        except Exception:
            pass

    # -------------------------------------------------------------
    # B. MANUAL KEYBOARD CONTROL
    # -------------------------------------------------------------
    
    # --- Body Movement (Vertical) ---
    if held_keys['t']: body_pivot.y += move_speed
    if held_keys['g']: body_pivot.y -= move_speed

    # --- Body Rotation ---
    if held_keys['b']: body_pivot.rotation_y += rotation_speed
    if held_keys['v']: body_pivot.rotation_y -= rotation_speed

    # --- SHOULDER (W/S - Pitch, A/D - Yaw, Q/E - Roll) ---
    if held_keys['w']: shoulder_pivot.rotation_x += rotation_speed
    if held_keys['s']: shoulder_pivot.rotation_x -= rotation_speed
    if held_keys['a']: shoulder_pivot.rotation_y += rotation_speed
    if held_keys['d']: shoulder_pivot.rotation_y -= rotation_speed
    if held_keys['q']: shoulder_pivot.rotation_z += rotation_speed
    if held_keys['e']: shoulder_pivot.rotation_z -= rotation_speed

    # --- FOREARM (Arrow keys) ---
    if held_keys['up arrow']:    forearm_pivot.rotation_x += rotation_speed
    if held_keys['down arrow']:  forearm_pivot.rotation_x -= rotation_speed
    if held_keys['left arrow']:  forearm_pivot.rotation_y += rotation_speed
    if held_keys['right arrow']: forearm_pivot.rotation_y -= rotation_speed

    # --- HAND (I/K - Pitch, J/L - Yaw, U/O - Roll) ---
    if held_keys['i']: hand_pivot.rotation_x += rotation_speed
    if held_keys['k']: hand_pivot.rotation_x -= rotation_speed
    if held_keys['j']: hand_pivot.rotation_y += rotation_speed
    if held_keys['l']: hand_pivot.rotation_y -= rotation_speed
    if held_keys['u']: hand_pivot.rotation_z += rotation_speed
    if held_keys['o']: hand_pivot.rotation_z -= rotation_speed

    # --- LEGS (Keyboard Controls) ---
    # Thigh (Leg_Higher): R / F keys
    if held_keys['r']: leg_higher_pivot_1.rotation_x += rotation_speed
    if held_keys['f']: leg_higher_pivot_1.rotation_x -= rotation_speed

    # Shin (Leg_Lower): 1 / 2 keys
    if held_keys['1']: leg_lower_pivot.rotation_x += rotation_speed
    if held_keys['2']: leg_lower_pivot.rotation_x -= rotation_speed

    # Foot (Leg_Foot): 3 / 4 keys
    if held_keys['3']: leg_foot_pivot.rotation_x += rotation_speed
    if held_keys['4']: leg_foot_pivot.rotation_x -= rotation_speed

def input(key):
    global use_serial
    # Reset all angles to 0
    if key == 'space':
        body_pivot.rotation = (0, 0, 0)
        shoulder_pivot.rotation = (0, 0, 0)
        forearm_pivot.rotation = (0, 0, 0)
        hand_pivot.rotation = (0, 0, 0)
        leg_higher_pivot_1.rotation = (0, 0, 0)
        leg_lower_pivot.rotation = (0, 0, 0)
        leg_foot_pivot.rotation = (0, 0, 0)
        
    # Toggle Sensors / Keyboard mode
    if key == 'tab':
        use_serial = not use_serial

app.run()