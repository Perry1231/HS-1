import os
import trimesh

models = ['shoulder.obj', 'forearm.obj', 'hand.obj']

for m in models:
    path = os.path.join('Models', m)
    if os.path.exists(path):
        print(f"Конвертуємо {m} у трикутну сітку...")
        mesh = trimesh.load(path, force='mesh')
        mesh.export(path) # Перезаписуємо з правильними трикутниками
        print(f"[OK] {m} успішно виправлено!")