import os
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from PIL import Image, ImageTk, ImageChops

class IconSlicerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("HackerHMI - Sprite Sheet Icon Slicer")
        self.root.geometry("1100x700")
        self.root.configure(bg="#121214")

        # Styles
        style = ttk.Style()
        style.theme_use("clam")
        style.configure("TLabel", background="#121214", foreground="#ffffff", font=("Segoe UI", 10))
        style.configure("TCheckbutton", background="#121214", foreground="#ffffff", font=("Segoe UI", 10))
        style.configure("TButton", background="#312e81", foreground="#ffffff", font=("Segoe UI", 10, "bold"))
        style.map("TButton", background=[("active", "#4338ca")])

        # State Variables
        self.img_path = None
        self.original_img = None  # PIL Image
        self.display_img = None   # Scaled image for display
        self.tk_display_img = None
        self.scale_factor = 1.0
        self.out_dir = None

        # Grid config
        self.cols_var = tk.IntVar(value=4)
        self.rows_var = tk.IntVar(value=3)
        self.margin_x_var = tk.IntVar(value=10)
        self.margin_y_var = tk.IntVar(value=10)
        self.pad_x_var = tk.IntVar(value=10)
        self.pad_y_var = tk.IntVar(value=10)
        
        # Options
        self.auto_center_var = tk.BooleanVar(value=True)
        self.transparent_var = tk.BooleanVar(value=True)
        self.prefix_var = tk.StringVar(value="icon")

        self.setup_ui()

    def setup_ui(self):
        # Left Panel (Controls) - Width 320
        control_frame = tk.Frame(self.root, bg="#1a1a1e", width=340, padx=15, pady=15)
        control_frame.pack(side=tk.LEFT, fill=tk.Y)
        control_frame.pack_propagate(False)

        # Title
        title_label = tk.Label(control_frame, text="NEON ICON SLICER", bg="#1a1a1e", fg="#a855f7", font=("Segoe UI", 14, "bold"))
        title_label.pack(anchor=tk.W, pady=(0, 15))

        # 1. Load File
        btn_load = ttk.Button(control_frame, text="Load Sprite Sheet", command=self.load_image)
        btn_load.pack(fill=tk.X, pady=(0, 15))

        # 2. Grid Parameters Section
        grid_group = tk.LabelFrame(control_frame, text=" Grid Settings ", bg="#1a1a1e", fg="#06b6d4", font=("Segoe UI", 10, "bold"), padx=10, pady=10)
        grid_group.pack(fill=tk.X, pady=(0, 15))

        # Cols
        tk.Label(grid_group, text="Columns:", bg="#1a1a1e", fg="#e2e8f0").grid(row=0, column=0, sticky=tk.W, pady=5)
        cols_spin = tk.Spinbox(grid_group, from_=1, to=20, textvariable=self.cols_var, width=5, bg="#27272a", fg="#ffffff", insertbackground="white", command=self.update_grid_view)
        cols_spin.grid(row=0, column=1, sticky=tk.E, pady=5)
        self.cols_var.trace_add("write", lambda *args: self.update_grid_view())

        # Rows
        tk.Label(grid_group, text="Rows:", bg="#1a1a1e", fg="#e2e8f0").grid(row=1, column=0, sticky=tk.W, pady=5)
        rows_spin = tk.Spinbox(grid_group, from_=1, to=20, textvariable=self.rows_var, width=5, bg="#27272a", fg="#ffffff", insertbackground="white", command=self.update_grid_view)
        rows_spin.grid(row=1, column=1, sticky=tk.E, pady=5)
        self.rows_var.trace_add("write", lambda *args: self.update_grid_view())

        # Margin X
        tk.Label(grid_group, text="Margin X (px):", bg="#1a1a1e", fg="#e2e8f0").grid(row=2, column=0, sticky=tk.W, pady=5)
        margin_x_scale = tk.Scale(grid_group, from_=0, to=100, variable=self.margin_x_var, orient=tk.HORIZONTAL, bg="#1a1a1e", fg="#ffffff", troughcolor="#27272a", activebackground="#a855f7", showvalue=True, bd=0, highlightthickness=0, command=lambda x: self.update_grid_view())
        margin_x_scale.grid(row=2, column=1, sticky=tk.EW, pady=5)

        # Margin Y
        tk.Label(grid_group, text="Margin Y (px):", bg="#1a1a1e", fg="#e2e8f0").grid(row=3, column=0, sticky=tk.W, pady=5)
        margin_y_scale = tk.Scale(grid_group, from_=0, to=100, variable=self.margin_y_var, orient=tk.HORIZONTAL, bg="#1a1a1e", fg="#ffffff", troughcolor="#27272a", activebackground="#a855f7", showvalue=True, bd=0, highlightthickness=0, command=lambda x: self.update_grid_view())
        margin_y_scale.grid(row=3, column=1, sticky=tk.EW, pady=5)

        # Padding X
        tk.Label(grid_group, text="Padding X (px):", bg="#1a1a1e", fg="#e2e8f0").grid(row=4, column=0, sticky=tk.W, pady=5)
        pad_x_scale = tk.Scale(grid_group, from_=0, to=100, variable=self.pad_x_var, orient=tk.HORIZONTAL, bg="#1a1a1e", fg="#ffffff", troughcolor="#27272a", activebackground="#a855f7", showvalue=True, bd=0, highlightthickness=0, command=lambda x: self.update_grid_view())
        pad_x_scale.grid(row=4, column=1, sticky=tk.EW, pady=5)

        # Padding Y
        tk.Label(grid_group, text="Padding Y (px):", bg="#1a1a1e", fg="#e2e8f0").grid(row=5, column=0, sticky=tk.W, pady=5)
        pad_y_scale = tk.Scale(grid_group, from_=0, to=100, variable=self.pad_y_var, orient=tk.HORIZONTAL, bg="#1a1a1e", fg="#ffffff", troughcolor="#27272a", activebackground="#a855f7", showvalue=True, bd=0, highlightthickness=0, command=lambda x: self.update_grid_view())
        pad_y_scale.grid(row=5, column=1, sticky=tk.EW, pady=5)
        grid_group.columnconfigure(1, weight=1)

        # 3. Export Options Section
        opt_group = tk.LabelFrame(control_frame, text=" Export Options ", bg="#1a1a1e", fg="#06b6d4", font=("Segoe UI", 10, "bold"), padx=10, pady=10)
        opt_group.pack(fill=tk.X, pady=(0, 15))

        # Checkbox Auto-Center
        cb_center = ttk.Checkbutton(opt_group, text="Auto-Center & Scale (128x128)", variable=self.auto_center_var)
        cb_center.pack(anchor=tk.W, pady=5)

        # Checkbox Transparent Background
        cb_trans = ttk.Checkbutton(opt_group, text="Remove Black Background", variable=self.transparent_var)
        cb_trans.pack(anchor=tk.W, pady=5)

        # Prefix Name
        tk.Label(opt_group, text="File Name Prefix:", bg="#1a1a1e", fg="#e2e8f0").pack(anchor=tk.W, pady=(5, 2))
        prefix_entry = tk.Entry(opt_group, textvariable=self.prefix_var, bg="#27272a", fg="#ffffff", insertbackground="white", bd=1)
        prefix_entry.pack(fill=tk.X, pady=5)

        # 4. Action buttons
        btn_out_dir = ttk.Button(control_frame, text="Select Output Folder", command=self.select_output_folder)
        btn_out_dir.pack(fill=tk.X, pady=(0, 10))

        self.btn_slice = ttk.Button(control_frame, text="SLICE & SAVE", style="TButton", command=self.slice_and_save)
        self.btn_slice.pack(fill=tk.X, pady=(10, 0))

        # Status footer in control
        self.status_lbl = tk.Label(control_frame, text="Ready. Load an image.", bg="#1a1a1e", fg="#94a3b8", justify=tk.LEFT, wraplength=300)
        self.status_lbl.pack(side=tk.BOTTOM, fill=tk.X, pady=10)

        # Right Panel (Preview Canvas)
        preview_frame = tk.Frame(self.root, bg="#121214", padx=10, pady=10)
        preview_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

        self.canvas = tk.Canvas(preview_frame, bg="#1e1e2e", highlightthickness=1, highlightbackground="#3f3f46")
        self.canvas.pack(fill=tk.BOTH, expand=True)

    def load_image(self):
        file_path = filedialog.askopenfilename(
            title="Open Sprite Sheet Image",
            filetypes=[("Image Files", "*.png *.jpg *.jpeg *.bmp *.webp"), ("All Files", "*.*")]
        )
        if not file_path:
            return
            
        try:
            self.img_path = file_path
            self.original_img = Image.open(file_path).convert("RGB")
            self.update_canvas_display()
            self.status_lbl.configure(text=f"Loaded: {os.path.basename(file_path)}\n({self.original_img.width}x{self.original_img.height})", fg="#22c55e")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to open image:\n{e}")

    def update_canvas_display(self):
        if not self.original_img: return

        # Calculate fitting size
        canvas_w = self.canvas.winfo_width()
        canvas_h = self.canvas.winfo_height()
        
        # Fallback if canvas is not drawn yet
        if canvas_w < 10: canvas_w = 700
        if canvas_h < 10: canvas_h = 600

        img_w, img_h = self.original_img.size
        
        # Scale factor
        self.scale_factor = min(canvas_w / img_w, canvas_h / img_h, 1.0)
        
        new_w = int(img_w * self.scale_factor)
        new_h = int(img_h * self.scale_factor)
        
        self.display_img = self.original_img.resize((new_w, new_h), Image.Resampling.LANCZOS)
        self.tk_display_img = ImageTk.PhotoImage(self.display_img)
        
        self.canvas.delete("all")
        # Center image in canvas
        self.x_offset = (canvas_w - new_w) // 2
        self.y_offset = (canvas_h - new_h) // 2
        self.canvas.create_image(self.x_offset, self.y_offset, anchor=tk.NW, image=self.tk_display_img)
        
        self.update_grid_view()

    def update_grid_view(self):
        if not self.original_img: return
        
        self.canvas.delete("grid_line")
        
        img_w, img_h = self.original_img.size
        cols = max(1, self.cols_var.get())
        rows = max(1, self.rows_var.get())
        
        margin_x = self.margin_x_var.get()
        margin_y = self.margin_y_var.get()
        pad_x = self.pad_x_var.get()
        pad_y = self.pad_y_var.get()
        
        # Calculate cell size in original dimensions
        avail_w = img_w - (2 * margin_x) - ((cols - 1) * pad_x)
        avail_h = img_h - (2 * margin_y) - ((rows - 1) * pad_y)
        
        if avail_w <= 0 or avail_h <= 0:
            self.status_lbl.configure(text="Grid parameters are too large for the image!", fg="#ef4444")
            return
            
        cell_w = avail_w / cols
        cell_h = avail_h / rows
        
        # Draw grid lines on canvas
        for r in range(rows):
            for c in range(cols):
                # Cells in original coords
                x1_orig = margin_x + c * (cell_w + pad_x)
                y1_orig = margin_y + r * (cell_h + pad_y)
                x2_orig = x1_orig + cell_w
                y2_orig = y1_orig + cell_h
                
                # Scale to display coords
                x1_disp = self.x_offset + int(x1_orig * self.scale_factor)
                y1_disp = self.y_offset + int(y1_orig * self.scale_factor)
                x2_disp = self.x_offset + int(x2_orig * self.scale_factor)
                y2_disp = self.y_offset + int(y2_orig * self.scale_factor)
                
                # Draw cell rectangles
                self.canvas.create_rectangle(
                    x1_disp, y1_disp, x2_disp, y2_disp,
                    outline="#a855f7", width=1, dash=(4, 4), tags="grid_line"
                )
                
                # Optional: draw grid labels
                self.canvas.create_text(
                    x1_disp + 10, y1_disp + 10,
                    text=f"{r*cols + c + 1}", fill="#06b6d4",
                    font=("Segoe UI", 8, "bold"), tags="grid_line"
                )

    def select_output_folder(self):
        folder = filedialog.askdirectory(title="Select Output Folder")
        if folder:
            self.out_dir = folder
            self.status_lbl.configure(text=f"Out Folder: {os.path.basename(folder)}", fg="#e2e8f0")

    def slice_and_save(self):
        if not self.original_img:
            messagebox.showwarning("Warning", "Please load a sprite sheet image first!")
            return
            
        if not self.out_dir:
            self.select_output_folder()
            if not self.out_dir:
                return

        img_w, img_h = self.original_img.size
        cols = max(1, self.cols_var.get())
        rows = max(1, self.rows_var.get())
        
        margin_x = self.margin_x_var.get()
        margin_y = self.margin_y_var.get()
        pad_x = self.pad_x_var.get()
        pad_y = self.pad_y_var.get()
        
        avail_w = img_w - (2 * margin_x) - ((cols - 1) * pad_x)
        avail_h = img_h - (2 * margin_y) - ((rows - 1) * pad_y)
        
        if avail_w <= 0 or avail_h <= 0:
            messagebox.showerror("Error", "Grid parameters are too large for the image size!")
            return
            
        cell_w = avail_w / cols
        cell_h = avail_h / rows
        
        prefix = self.prefix_var.get().strip()
        if not prefix: prefix = "icon"
        
        auto_center = self.auto_center_var.get()
        remove_bg = self.transparent_var.get()
        
        saved_count = 0
        
        for r in range(rows):
            for c in range(cols):
                # Boundaries of current cell
                x1 = int(margin_x + c * (cell_w + pad_x))
                y1 = int(margin_y + r * (cell_h + pad_y))
                x2 = int(x1 + cell_w)
                y2 = int(y1 + cell_h)
                
                # Clip coordinates
                x1 = max(0, min(x1, img_w))
                y1 = max(0, min(y1, img_h))
                x2 = max(0, min(x2, img_w))
                y2 = max(0, min(y2, img_h))
                
                if (x2 - x1) <= 0 or (y2 - y1) <= 0:
                    continue
                
                # Crop raw cell
                cell_img = self.original_img.crop((x1, y1, x2, y2))
                
                # Apply auto center & scale
                if auto_center:
                    # Find bounding box by checking pixels exceeding threshold brightness
                    cc_w, cc_h = cell_img.size
                    pixels = cell_img.load()
                    min_x, min_y = cc_w, cc_h
                    max_x, max_y = 0, 0
                    threshold = 20
                    
                    for y in range(cc_h):
                        for x in range(cc_w):
                            rv, gv, bv = pixels[x, y]
                            if max(rv, gv, bv) > threshold:
                                if x < min_x: min_x = x
                                if y < min_y: min_y = y
                                if x > max_x: max_x = x
                                if y > max_y: max_y = y
                    
                    if max_x >= min_x and max_y >= min_y:
                        icon_cropped = cell_img.crop((min_x, min_y, max_x + 1, max_y + 1))
                        ic_w, ic_h = icon_cropped.size
                        
                        # Create final 128x128 canvas
                        canvas = Image.new("RGBA", (128, 128), (0, 0, 0, 0))
                        
                        # Scale to fit with padding (keep ratio)
                        scale = min(100 / ic_w, 100 / ic_h, 1.0)
                        new_w = int(ic_w * scale)
                        new_h = int(ic_h * scale)
                        
                        icon_resized = icon_cropped.resize((new_w, new_h), Image.Resampling.LANCZOS)
                        
                        # Apply transparent background (unmultiply alpha) if checked
                        if remove_bg:
                            rgba_icon = Image.new("RGBA", (new_w, new_h))
                            src_pix = icon_resized.load()
                            dest_pix = rgba_icon.load()
                            
                            for y_px in range(new_h):
                                for x_px in range(new_w):
                                    rv, gv, bv = src_pix[x_px, y_px]
                                    bright = max(rv, gv, bv)
                                    if bright < 15:
                                        dest_pix[x_px, y_px] = (0, 0, 0, 0)
                                    else:
                                        alpha = min(255, int(bright * 1.5))
                                        r_new = min(255, int(rv * 255 / bright))
                                        g_new = min(255, int(gv * 255 / bright))
                                        b_new = min(255, int(bv * 255 / bright))
                                        dest_pix[x_px, y_px] = (r_new, g_new, b_new, alpha)
                            icon_final = rgba_icon
                        else:
                            # Keep background but convert to RGBA
                            icon_final = icon_resized.convert("RGBA")
                            
                        # Paste centered
                        paste_x = (128 - new_w) // 2
                        paste_y = (128 - new_h) // 2
                        canvas.paste(icon_final, (paste_x, paste_y), icon_final)
                        final_out = canvas
                    else:
                        # Empty cell
                        continue
                else:
                    # Raw crop (no auto-centering), apply transparency directly to the whole grid cell if requested
                    if remove_bg:
                        rgba_img = Image.new("RGBA", cell_img.size)
                        src_pix = cell_img.load()
                        dest_pix = rgba_img.load()
                        w_px, h_px = cell_img.size
                        for y_px in range(h_px):
                            for x_px in range(w_px):
                                rv, gv, bv = src_pix[x_px, y_px]
                                bright = max(rv, gv, bv)
                                if bright < 15:
                                    dest_pix[x_px, y_px] = (0, 0, 0, 0)
                                else:
                                    alpha = min(255, int(bright * 1.5))
                                    r_new = min(255, int(rv * 255 / bright))
                                    g_new = min(255, int(gv * 255 / bright))
                                    b_new = min(255, int(bv * 255 / bright))
                                    dest_pix[x_px, y_px] = (r_new, g_new, b_new, alpha)
                        final_out = rgba_img
                    else:
                        final_out = cell_img
                
                # Save output file
                out_name = f"{prefix}_{r * cols + c + 1}.png"
                final_out.save(os.path.join(self.out_dir, out_name), "PNG")
                saved_count += 1
                
        messagebox.showinfo("Success", f"Successfully sliced and saved {saved_count} icons to:\n{self.out_dir}")
        self.status_lbl.configure(text=f"Saved {saved_count} icons successfully!", fg="#22c55e")

if __name__ == "__main__":
    root = tk.Tk()
    app = IconSlicerApp(root)
    
    # Simple event binding for canvas resizing
    def on_resize(event):
        # Only trigger redraw if the resize event target is the canvas
        if event.widget == app.canvas:
            app.update_canvas_display()
            
    app.canvas.bind("<Configure>", on_resize)
    root.mainloop()
