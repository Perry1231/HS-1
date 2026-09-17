from ursina import *
import serial
import json

app = Ursina()

# Створення координатної сітки (щоб бачити, де центр світу)
Entity(model='grid', scale=50, color=color.gray)

# --- 1. ПЛЕЧЕ (БІЦЕПС) ---
shoulder_pivot = Entity(position=(0, 5, 0))
shoulder_model = Entity(
    parent=shoulder_pivot,
    model='Models/Biceps_2_0.stl',
    scale=1.0,                 # Збільшено масштаб під нормальне відображення
    color=color.azure,          # Яскраво-блакитний
    double_sided=True
)

# --- 2. ПЕРЕДПЛІЧЧЯ ---
# Z/Y зміщення підігнано під ліктьовий суглоб
forearm_pivot = Entity(parent=shoulder_pivot, position=(0, -5, 0)) 
forearm_model = Entity(
    parent=forearm_pivot,
    model='Models/Forearm_2_0.stl',
    scale=1.0,
    color=color.lime,           # Яскраво-зелений
    double_sided=True
)

# --- 3. КИСТЬ (ХЕНД) ---
# Зміщення нижче по лінії передпліччя до зап'ястя
hand_pivot = Entity(parent=forearm_pivot, position=(0, -5, 0))
hand_model = Entity(
    parent=hand_pivot,
    model='Models/Hand_2_0.stl',
    scale=1.0,
    color=color.magenta,        # Рожевий/Маджента (щоб чітко бачити Кисть)
    double_sided=True
)

# --- КАМЕРА ТА ОСВІТЛЕННЯ ---
cam = EditorCamera()
cam.position = (0, 0, -30)      # Фокус камери точно на деталі

DirectionalLight(y=3, z=-5, rotation=(45, -45, 0))
AmbientLight(color=color.rgba(150, 150, 150, 0.8))

# --- ЗЧИТАННЯ SERIAL ---
try:
    ser = serial.Serial('COM3', 115200, timeout=0.05)
except Exception as e:
    print(f"Помилка відкриття COM-порту: {e}")
    ser = None

def update():
    if ser and ser.in_waiting > 0:
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

app.run()