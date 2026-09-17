import tkinter as tk
from tkinter import messagebox, ttk
import serial
import threading
import time
import json
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

COM_PORT = 'COM7'
BAUD_RATE = 115200

# Try to connect to serial port
try:
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=0.1)
except Exception as e:
    ser = None

def send_command(cmd):
    if ser and ser.is_open:
        ser.write(cmd.encode())
        status_label.config(text=f"Sent Command: {cmd}", fg="#2ecc71")
    else:
        status_label.config(text="Disconnected!", fg="#e74c3c")

# --- UI Setup ---
root = tk.Tk()
root.title("GreenThread Mission Control")
root.geometry("1000x650")
root.configure(bg="#1e272e")

title = tk.Label(root, text="🚀 GreenThread Mission Control", font=("Arial", 24, "bold"), bg="#1e272e", fg="white", pady=15)
title.pack()

main_frame = tk.Frame(root, bg="#1e272e")
main_frame.pack(fill=tk.BOTH, expand=True, padx=20)

# Left Column: Controls & Telemetry
left_frame = tk.Frame(main_frame, bg="#2c3e50", width=350, padx=20, pady=20)
left_frame.pack(side=tk.LEFT, fill=tk.Y, padx=10)

conn_text = f"🟢 Connected to {COM_PORT}" if ser else f"🔴 ERROR: Could not open {COM_PORT}"
conn_color = "#2ecc71" if ser else "#e74c3c"
tk.Label(left_frame, text=conn_text, font=("Arial", 12, "bold"), bg="#2c3e50", fg=conn_color).pack(pady=10)

# Telemetry Display
telemetry_frame = tk.LabelFrame(left_frame, text="Live Telemetry", font=("Arial", 14, "bold"), bg="#2c3e50", fg="white", padx=10, pady=10)
telemetry_frame.pack(fill=tk.X, pady=10)

temp_label = tk.Label(telemetry_frame, text="🌡️ Temp: -- °C", font=("Arial", 16), bg="#2c3e50", fg="#f1c40f")
temp_label.pack(anchor="w")

hum_label = tk.Label(telemetry_frame, text="💧 Humidity: -- %", font=("Arial", 16), bg="#2c3e50", fg="#3498db")
hum_label.pack(anchor="w")

moist_label = tk.Label(telemetry_frame, text="🌱 Moisture: --", font=("Arial", 16), bg="#2c3e50", fg="#2ecc71")
moist_label.pack(anchor="w")

# Control Buttons
btn_style = {"font": ("Arial", 12, "bold"), "bg": "#34495e", "fg": "white", "width": 15, "pady": 5}
control_frame = tk.LabelFrame(left_frame, text="Rover Controls", font=("Arial", 14, "bold"), bg="#2c3e50", fg="white", padx=10, pady=10)
control_frame.pack(fill=tk.X, pady=10)

tk.Button(control_frame, text="Forward (W)", command=lambda: send_command('F'), bg="#3498db", fg="white", font=("Arial", 12, "bold")).pack(fill=tk.X, pady=5)
tk.Button(control_frame, text="STOP (Space)", command=lambda: send_command('S'), bg="#e74c3c", fg="white", font=("Arial", 12, "bold"), pady=10).pack(fill=tk.X, pady=5)
tk.Button(control_frame, text="Backward (S)", command=lambda: send_command('B'), bg="#3498db", fg="white", font=("Arial", 12, "bold")).pack(fill=tk.X, pady=5)

tk.Button(control_frame, text="🔬 Run Deep Scan (E)", command=lambda: send_command('D'), bg="#9b59b6", fg="white", font=("Arial", 12, "bold"), pady=10).pack(fill=tk.X, pady=15)
tk.Button(control_frame, text="Lift Sensor (Q)", command=lambda: send_command('U'), bg="#f39c12", fg="white", font=("Arial", 10, "bold")).pack(fill=tk.X)

status_label = tk.Label(left_frame, text="System Ready.", font=("Arial", 12), bg="#2c3e50", fg="white", pady=10)
status_label.pack(side=tk.BOTTOM)

# Right Column: Matplotlib Graph
right_frame = tk.Frame(main_frame, bg="#1e272e")
right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

fig, ax = plt.subplots(figsize=(7, 5), facecolor='#1e272e')
ax.set_facecolor('#2f3640')
channels = ['A','B','C','D','E','F','G','H','I','J','K','L','R','S','T','U','V','W']
bars = ax.bar(channels, [0]*18, color='#00a8ff')
ax.set_title("AS7265x Spectral Analysis", color='white', fontsize=16)
ax.set_xlabel("Wavelength Channels (UV -> IR)", color='white')
ax.set_ylabel("Intensity", color='white')
ax.tick_params(colors='white')
ax.grid(axis='y', linestyle='--', alpha=0.3)

canvas = FigureCanvasTkAgg(fig, master=right_frame)
canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

def update_graph(csv_data):
    try:
        values = [float(x) for x in csv_data.split(',')]
        if len(values) == 18:
            for bar, val in zip(bars, values):
                bar.set_height(val)
            ax.set_ylim(0, max(max(values)*1.2, 10)) # Auto-scale Y axis
            canvas.draw()
    except Exception as e:
        print("Graph Error:", e)

# Keybinds
root.bind('<w>', lambda event: send_command('F'))
root.bind('<s>', lambda event: send_command('B'))
root.bind('<space>', lambda event: send_command('S'))
root.bind('<e>', lambda event: send_command('D'))
root.bind('<q>', lambda event: send_command('U'))

def parse_data(line):
    try:
        data = json.loads(line)
        if data.get("type") == "telemetry":
            root.after(0, lambda: temp_label.config(text=f"🌡️ Temp: {data['temp']} °C"))
            root.after(0, lambda: hum_label.config(text=f"💧 Humidity: {data['hum']} %"))
            root.after(0, lambda: moist_label.config(text=f"🌱 Moisture: {data['moisture']}"))
        elif data.get("type") == "spectrum":
            root.after(0, lambda: status_label.config(text="Spectroscopy Data Received!", fg="#9b59b6"))
            root.after(0, lambda: update_graph(data["csv"]))
    except:
        pass # Not JSON, ignore or print to console
        print(f"Log: {line}")

def read_serial():
    while ser and ser.is_open:
        try:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line.startswith("{"):
                    parse_data(line)
        except:
            pass
        time.sleep(0.05)

if ser:
    threading.Thread(target=read_serial, daemon=True).start()

def on_closing():
    if ser:
        ser.close()
    root.quit()

root.protocol("WM_DELETE_WINDOW", on_closing)

if not ser:
    messagebox.showerror("Connection Error", f"Could not connect to {COM_PORT}!\nMake sure Serial Monitor is closed.")

root.mainloop()
