import tkinter as tk
from tkinter import messagebox
import serial
import threading
import time
import sys

COM_PORT = 'COM7'
BAUD_RATE = 115200

try:
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=0.1)
except Exception as e:
    ser = None

def send_command(cmd):
    if ser and ser.is_open:
        ser.write(cmd.encode())
        status_label.config(text=f"Sent: {cmd}", fg="green")
    else:
        status_label.config(text="Disconnected!", fg="red")

root = tk.Tk()
root.title("PlantBot Remote Control UI")
root.geometry("400x500")
root.configure(bg="#2c3e50")
root.resizable(False, False)

btn_style = {"font": ("Arial", 14, "bold"), "bg": "#3498db", "fg": "white", "activebackground": "#2980b9", "width": 15, "pady": 10}
stop_style = {"font": ("Arial", 16, "bold"), "bg": "#e74c3c", "fg": "white", "activebackground": "#c0392b", "width": 13, "pady": 15}
action_style = {"font": ("Arial", 12, "bold"), "bg": "#2ecc71", "fg": "white", "activebackground": "#27ae60", "width": 20, "pady": 5}

title = tk.Label(root, text="🤖 GreenThread Remote UI", font=("Arial", 20, "bold"), bg="#2c3e50", fg="white", pady=20)
title.pack()

conn_text = f"Connected to {COM_PORT}" if ser else f"ERROR: Could not open {COM_PORT}"
conn_color = "#2ecc71" if ser else "#e74c3c"
conn_label = tk.Label(root, text=conn_text, font=("Arial", 10), bg="#2c3e50", fg=conn_color)
conn_label.pack()

drive_frame = tk.Frame(root, bg="#2c3e50", pady=20)
drive_frame.pack()

btn_forward = tk.Button(drive_frame, text="Drive Forward (W)", command=lambda: send_command('F'), **btn_style)
btn_forward.grid(row=0, column=0, pady=5)
btn_stop = tk.Button(drive_frame, text="STOP (Space)", command=lambda: send_command('S'), **stop_style)
btn_stop.grid(row=1, column=0, pady=10)
btn_backward = tk.Button(drive_frame, text="Drive Backward (S)", command=lambda: send_command('B'), **btn_style)
btn_backward.grid(row=2, column=0, pady=5)

tools_frame = tk.Frame(root, bg="#2c3e50", pady=20)
tools_frame.pack()
btn_drop = tk.Button(tools_frame, text="Scan Sensors (E)", command=lambda: send_command('D'), **action_style)
btn_drop.grid(row=0, column=0, padx=10)
btn_lift = tk.Button(tools_frame, text="Emergency Lift (Q)", command=lambda: send_command('U'), **action_style)
btn_lift.grid(row=0, column=1, padx=10)

status_label = tk.Label(root, text="Waiting for command...", font=("Arial", 12), bg="#2c3e50", fg="white", pady=10)
status_label.pack()

root.bind('<w>', lambda event: send_command('F'))
root.bind('<s>', lambda event: send_command('B'))
root.bind('<space>', lambda event: send_command('S'))
root.bind('<e>', lambda event: send_command('D'))
root.bind('<q>', lambda event: send_command('U'))

def read_serial():
    while ser and ser.is_open:
        try:
            if ser.in_waiting > 0:
                msg = ser.readline().decode('utf-8', errors='ignore').strip()
                if msg:
                    root.after(0, lambda: status_label.config(text=f"Bot: {msg[:30]}", fg="#f1c40f"))
        except:
            pass
        time.sleep(0.05)

if ser:
    t = threading.Thread(target=read_serial, daemon=True)
    t.start()

def on_closing():
    if ser:
        ser.close()
    root.destroy()

root.protocol("WM_DELETE_WINDOW", on_closing)

if not ser:
    messagebox.showerror("Connection Error", f"Could not connect to {COM_PORT}!\nMake sure Serial Monitor is closed.")
root.mainloop()
