import json
import os
import shutil
import serial
import trimesh
from ursina import *

# -------------------------------------------------------------
# 1. MESH RE-CENTERING & VERTICAL ALIGNMENT (TRIMESH)
# -------------------------------------------------------------
MODELS = ['Models/shoulder.obj', 'Models/forearm.obj', 'Models/hand.obj']
EXISTING_MODELS = [m for m in MODELS if os.path.exists(m)]

VERTICAL_OFFSET = -1.2  

if EXISTING_MODELS:
    try:
        if os.path.exists('models_compressed'):
            shutil.rmtree('models_compressed')
            
        meshes = [trimesh.load(m, force='mesh') for m in EXISTING_MODELS]
        combined = trimesh.util.concatenate(meshes)
        center = combined.bounds.mean(axis=0)
        
        if sum(abs(center)) > 0.1:
            print(f"[INFO] Recentering mesh geometry from {center} to origin...")
            for path, mesh in zip(EXISTING_MODELS, meshes):
                mesh.vertices -= center
                mesh.export(path)
            print("[OK] All 3D models centered successfully on disk!")
    except Exception as e:
        print(f"[WARNING] Could not recenter via trimesh: {e}")

# -------------------------------------------------------------
# 2. SERIAL PORT CONFIGURATION
# -------------------------------------------------------------
SERIAL_PORT = 'COM4'
BAUD_RATE = 115200

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.05)
    print(f"[OK] Connected to serial port {SERIAL_PORT}")
except Exception as e:
    print(f"[INFO] Serial port not connected ({e}). Running in test mode.")
    ser = None

# -------------------------------------------------------------
# 3. URSINA INITIALIZATION & SCENE SETUP
# -------------------------------------------------------------
app = Ursina(title="HS-1 Kinematic Suit - Realtime Visualizer")

DirectionalLight(color=color.white, y=2, z=-3)
AmbientLight(color=color.rgba(140, 140, 140, 0.7))

# Parent container
arm_root = Entity(position=(0, VERTICAL_OFFSET, 0))

# 3D Parts (Parenting chain preserves relative joints correctly)
shoulder = Entity(model='Models/shoulder.obj', color=color.azure, scale=0.001, parent=arm_root)
forearm = Entity(model='Models/forearm.obj', color=color.orange, scale=0.001, parent=arm_root)
hand = Entity(model='Models/hand.obj', color=color.lime, scale=0.001, parent=arm_root)

# -------------------------------------------------------------
# 4. CAMERA & ENVIRONMENT
# -------------------------------------------------------------
EditorCamera()
Entity(model=Grid(20, 20), color=color.gray)

Text(text="[1] Shoulder | [2] Elbow | [3] Hand | Up/Down Arrows - Adjust Height", 
     position=(-0.85, 0.45), scale=1.0)

# -------------------------------------------------------------
# 5. MAIN UPDATE LOOP
# -------------------------------------------------------------
def update():
    # Real-time height adjustment
    if held_keys['down arrow']: arm_root.y -= 0.8 * time.dt
    if held_keys['up arrow']:   arm_root.y += 0.8 * time.dt

    # Test joint rotation keys
    if held_keys['1']: shoulder.rotation_z += 1
    if held_keys['2']: forearm.rotation_z += 1
    if held_keys['3']: hand.rotation_z += 1

    # Read latest serial JSON frame (skips ANSI HUD tables automatically)
    if ser and ser.is_open and ser.in_waiting:
        try:
            # Read until the latest available line to avoid lag
            lines = ser.readlines()
            for raw_bytes in reversed(lines):
                raw_line = raw_bytes.decode('utf-8', errors='ignore').strip()
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
                    
                    # Stop parsing once the newest valid JSON frame is processed
                    break
        except Exception:
            pass

app.run()