from ursina import *
import serial
import json

app = Ursina()

# --- 1. FLOOR AND GRID ---
# Замість стандартної сітки, яка може викликати попередження, зробимо базовий простір безпечним
Entity(scale=50, color=color.gray)

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

# --- ARM 1 (Left / First Arm) ---
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


# --- ARM 2 (Left / Second Arm) ---
# --- ARM 2 (Right / Second Arm) ---
shoulder_pivot_2 = Entity(parent=body_pivot, position=(0, -150, 38))
shoulder_model_2 = Entity(
    parent=shoulder_pivot_2,
    model='Models/Biceps_2_0.stl',
    scale=1.0,
    color=color.azure,
    double_sided=True,
    position=(0, -2.5, -570),     # Повертаємо нормальний зсув як у першої руки
    rotation=(0, 180, 0)       # Розвертаємо тільки по Y на 180 градусів, щоб дивилася в інший бік
)

forearm_pivot_2 = Entity(parent=shoulder_pivot_2, position=(0, -235, 0)) 
forearm_model_2 = Entity(
    parent=forearm_pivot_2,
    model='Models/Forearm_2_0.stl',
    scale=1.0,
    color=color.lime,
    double_sided=True,
    position=(0, -2.0, -570)
)

hand_pivot_2 = Entity(parent=forearm_pivot_2, position=(0, -150, 38))
hand_model_2 = Entity(
    parent=hand_pivot_2,
    model='Models/Hand_2_0.stl',
    scale=1.0,
    color=color.magenta,
    double_sided=True,
    position=(0, -1.2, -646)
)
Entity(parent=hand_pivot_2, model='sphere', scale=1, color=color.yellow)


# --- LEG 1 (First Leg) ---
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

leg_lower_pivot_1 = Entity(parent=leg_higher_pivot_1, position=(0, -4.0, 0))
leg_lower_model_1 = Entity(
    parent=leg_lower_pivot_1,
    model='Models/Leg_Lower_2_0.stl',
    scale=1.0,
    color=color.brown,
    double_sided=True,
    position=(0, -1350.0, -420),
    rotation=(270, 180, 90)
)

leg_foot_pivot_1 = Entity(parent=leg_lower_pivot_1, position=(0, -4.0, 0))
leg_foot_model_1 = Entity(
    parent=leg_foot_pivot_1,
    model='Models/Leg_Foot_2_0.stl',
    scale=1.0,
    color=color.red,
    double_sided=True,
    position=(-200, -1480.0, -440),
    rotation=(180, 90, 180)
)
Entity(parent=leg_foot_pivot_1, model='sphere', scale=1, color=color.yellow)


# --- LEG 2 (Second Leg) --- .RIGHT==============
leg_higher_pivot_2 = Entity(parent=body_pivot, position=(0, -3, 0))
leg_higher_model_2 = Entity(
    parent=leg_higher_pivot_2,
    model='Models/Leg_Higher_2_0.stl',
    scale=1.0,
    color=color.cyan,
    double_sided=True,
    position=(0, -910.0, -155),
    rotation=(270, 0, 90)
)

leg_lower_pivot_2 = Entity(parent=leg_higher_pivot_2, position=(0, -4.0, 0))
leg_lower_model_2 = Entity(
    parent=leg_lower_pivot_2,
    model='Models/Leg_Lower_2_0.stl',
    scale=1.0,
    color=color.brown,
    double_sided=True,
    position=(0, -1350.0, -155),
    rotation=(270, 180, 90)
)

leg_foot_pivot_2 = Entity(parent=leg_lower_pivot_2, position=(0, -4.0, 0))
leg_foot_model_2 = Entity(
    parent=leg_foot_pivot_2,
    model='Models/Leg_Foot_2_0.stl',
    scale=1.0,
    color=color.red,
    double_sided=True,
    position=(-200, -1480.0, -177),
    rotation=(180, 90, 180)
)
Entity(parent=leg_foot_pivot_2, model='sphere', scale=1, color=color.yellow)


# --- 3. CAMERA AND LIGHTING ---
cam = EditorCamera()
cam.position = (0, 0, -40)

DirectionalLight(y=3, z=-5, rotation=(45, -45, 0))
AmbientLight(color=color.rgba(150, 150, 150, 0.8))

# --- 4. SERIAL COMMUNICATION ---
ser = None
use_serial = False

try:
    ser = serial.Serial('COM3', 115200, timeout=0.05)
    use_serial = True
    print("Успішно підключено до COM3!")
except Exception as e:
    print(f"Попередження COM-порту: {e}")
    print("Програма працюватиме лише з клавіатури без сенсорів.")

rotation_speed = 5.0
move_speed = 0.1

def update():
    global use_serial
    
    # -------------------------------------------------------------
    # A. AUTOMATIC ROTATION FROM SENSORS (UART JSON)
    # -------------------------------------------------------------
    if use_serial and ser and ser.is_open:
        try:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                if line.startswith('{') and line.endswith('}'):
                    data = json.loads(line)
                    
                    if 'body' in data:
                        b = data['body']
                        body_pivot.rotation = Vec3(b['p'], b['y'], b['r'])
                    
                    # Arm 1
                    if 'shoulder' in data:
                        s = data['shoulder']
                        shoulder_pivot.rotation = Vec3(s['p'], s['y'], s['r'])
                    if 'forearm' in data:
                        f = data['forearm']
                        forearm_pivot.rotation = Vec3(f['p'], f['y'], f['r'])
                    if 'hand' in data:
                        h = data['hand']
                        hand_pivot.rotation = Vec3(h['p'], h['y'], h['r'])
                    
                    # Arm 2
                    if 'shoulder_2' in data:
                        s2 = data['shoulder_2']
                        shoulder_pivot_2.rotation = Vec3(s2['p'], s2['y'], s2['r'])
                    if 'forearm_2' in data:
                        f2 = data['forearm_2']
                        forearm_pivot_2.rotation = Vec3(f2['p'], f2['y'], f2['r'])
                    if 'hand_2' in data:
                        h2 = data['hand_2']
                        hand_pivot_2.rotation = Vec3(h2['p'], h2['y'], h2['r'])

                    # Leg 1
                    if 'leg_higher' in data:
                        lh = data['leg_higher']
                        leg_higher_pivot_1.rotation = Vec3(lh['p'], lh['y'], lh['r'])
                    if 'leg_lower' in data:
                        ll = data['leg_lower']
                        leg_lower_pivot_1.rotation = Vec3(ll['p'], ll['y'], ll['r'])
                    if 'leg_foot' in data:
                        lf = data['leg_foot']
                        leg_foot_pivot_1.rotation = Vec3(lf['p'], lf['y'], lf['r'])

                    # Leg 2
                    if 'leg_higher_2' in data:
                        lh2 = data['leg_higher_2']
                        leg_higher_pivot_2.rotation = Vec3(lh2['p'], lh2['y'], lh2['r'])
                    if 'leg_lower_2' in data:
                        ll2 = data['leg_lower_2']
                        leg_lower_pivot_2.rotation = Vec3(ll2['p'], ll2['y'], ll2['r'])
                    if 'leg_foot_2' in data:
                        lf2 = data['leg_foot_2']
                        leg_foot_pivot_2.rotation = Vec3(lf2['p'], lf2['y'], lf2['r'])

        except Exception as err:
            # Виведемо помилку сюди, щоб бачити, якщо формат JSON зламається
            print("Помилка читання даних:", err)

    # -------------------------------------------------------------
    # B. MANUAL KEYBOARD CONTROL
    # -------------------------------------------------------------
    if held_keys['t']: body_pivot.y += move_speed
    if held_keys['g']: body_pivot.y -= move_speed
    if held_keys['b']: body_pivot.rotation_y += rotation_speed
    if held_keys['v']: body_pivot.rotation_y -= rotation_speed

    # Arm 1 controls
    if held_keys['w']: shoulder_pivot.rotation_x += rotation_speed
    if held_keys['s']: shoulder_pivot.rotation_x -= rotation_speed
    if held_keys['a']: shoulder_pivot.rotation_y += rotation_speed
    if held_keys['d']: shoulder_pivot.rotation_y -= rotation_speed
    if held_keys['q']: shoulder_pivot.rotation_z += rotation_speed
    if held_keys['e']: shoulder_pivot.rotation_z -= rotation_speed

    if held_keys['up arrow']:    forearm_pivot.rotation_x += rotation_speed
    if held_keys['down arrow']:  forearm_pivot.rotation_x -= rotation_speed
    if held_keys['left arrow']:  forearm_pivot.rotation_y += rotation_speed
    if held_keys['right arrow']: forearm_pivot.rotation_y -= rotation_speed

    if held_keys['i']: hand_pivot.rotation_x += rotation_speed
    if held_keys['k']: hand_pivot.rotation_x -= rotation_speed
    if held_keys['j']: hand_pivot.rotation_y += rotation_speed
    if held_keys['l']: hand_pivot.rotation_y -= rotation_speed
    if held_keys['u']: hand_pivot.rotation_z += rotation_speed
    if held_keys['o']: hand_pivot.rotation_z -= rotation_speed

    # Leg 1 controls
    if held_keys['r']: leg_higher_pivot_1.rotation_x += rotation_speed
    if held_keys['f']: leg_higher_pivot_1.rotation_x -= rotation_speed
    if held_keys['1']: leg_lower_pivot_1.rotation_x += rotation_speed
    if held_keys['2']: leg_lower_pivot_1.rotation_x -= rotation_speed
    if held_keys['3']: leg_foot_pivot_1.rotation_x += rotation_speed
    if held_keys['4']: leg_foot_pivot_1.rotation_x -= rotation_speed

def input(key):
    global use_serial
    if key == 'space':
        body_pivot.rotation = (0, 0, 0)
        shoulder_pivot.rotation = (0, 0, 0)
        forearm_pivot.rotation = (0, 0, 0)
        hand_pivot.rotation = (0, 0, 0)
        shoulder_pivot_2.rotation = (0, 0, 0)
        forearm_pivot_2.rotation = (0, 0, 0)
        hand_pivot_2.rotation = (0, 0, 0)
        leg_higher_pivot_1.rotation = (0, 0, 0)
        leg_lower_pivot_1.rotation = (0, 0, 0)
        leg_foot_pivot_1.rotation = (0, 0, 0)
        leg_higher_pivot_2.rotation = (0, 0, 0)
        leg_lower_pivot_2.rotation = (0, 0, 0)
        leg_foot_pivot_2.rotation = (0, 0, 0)

    if key == 'tab':
        use_serial = not use_serial
        print(f"Режим Serial: {use_serial}")

app.run()