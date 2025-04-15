import tkinter as tk
from tkinter import messagebox
import json
import time
import threading
import os
import re

JSON_FILE = "../data/cloud_transmission.json"
REFRESH_INTERVAL = 60  # seconds
ERROR_THRESHOLD = 180  # 3 minutes

class PacketBatchViewer:
    def __init__(self, root):
        self.root = root
        self.root.title("Smart Energy Batch Packet Viewer")
        self.root.geometry("880x550")
        self.current_index = 0
        self.batches = self.load_batches()
        self.last_received_time = None

        self.create_widgets()
        self.schedule_batch_display()

    def create_widgets(self):
        self.packet_list = tk.Listbox(self.root, font=("Consolas", 10), width=120, height=20)
        self.packet_list.pack(padx=10, pady=10)

        self.status_label = tk.Label(
            self.root, text="Waiting for data...", fg="blue", font=("Segoe UI", 10, "bold")
        )
        self.status_label.pack(pady=(0, 10))

        controls = tk.Frame(self.root)
        controls.pack(pady=5)
        tk.Button(controls, text="Refresh", command=self.refresh, width=10).pack(side="left", padx=5)
        tk.Button(controls, text="Exit", command=self.root.quit, width=10).pack(side="left", padx=5)

    def load_batches(self):
        if not os.path.exists(JSON_FILE):
            messagebox.showerror("Error", f"File {JSON_FILE} not found.")
            return []

        with open(JSON_FILE, "r") as f:
            raw = f.read()

        matches = re.findall(r'(\{.*?"transmission_time"\s*:\s*"\d+"\s*\})', raw, re.DOTALL)
        parsed = []
        for m in matches:
            try:
                parsed.append(json.loads(m))
            except json.JSONDecodeError:
                continue
        return parsed

    def schedule_batch_display(self):
        def stream_batches():
            while True:
                now = time.time()

                if self.current_index < len(self.batches):
                    self.display_batch(self.batches[self.current_index])
                    self.last_received_time = now
                    self.current_index += 1
                else:
                    if self.last_received_time is None:
                        self.update_status("Waiting for data...", "blue")
                    elif (now - self.last_received_time) > ERROR_THRESHOLD:
                        self.update_status("Missing data! No batch received in the last 3 minutes.", "red")
                    else:
                        self.update_status("Waiting for next batch...", "orange")

                time.sleep(REFRESH_INTERVAL)

        threading.Thread(target=stream_batches, daemon=True).start()

    def display_batch(self, batch):
        batch_number = self.current_index + 1
        sent_time = int(batch["transmission_time"])
        sent_time_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(sent_time))

        # Status shows only batch number and sent time
        self.update_status(
            f"Received batch number {batch_number} (sent at {sent_time_str})",
            "green"
        )
        self.root.update_idletasks()

        for pkt in batch["batch"]:
            line = f"{pkt['timestamp']} | {pkt['device_id']:<15} | {pkt['power_watts']:>6.2f} W"
            self.packet_list.insert(tk.END, line)

        self.packet_list.insert(tk.END, "-" * 100)
        self.packet_list.yview_moveto(1)

    def refresh(self):
        new_batches = self.load_batches()
        if len(new_batches) > len(self.batches):
            new_data = new_batches[len(self.batches):]
            for batch in new_data:
                self.display_batch(batch)
            self.batches.extend(new_data)
            self.update_status(f"Refreshed: {len(new_data)} new batch(es) found", "blue")
        else:
            self.update_status("No new batches found on refresh.", "gray")

    def update_status(self, text, color):
        self.status_label.config(text=text, fg=color)

if __name__ == "__main__":
    root = tk.Tk()
    app = PacketBatchViewer(root)
    root.mainloop()
