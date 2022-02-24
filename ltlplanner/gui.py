import tkinter as tk

from tkinter import ttk

def render_world(canvas, world):
    """
    Renders a 2D "world" board onto a canvas.

    Assumes that `world` is of the form

    world = [
        [None, "blue", None],
        [None, "yellow", "red"],
        ["white", "blue", None],
        ["green", None, None]
    ]
    """
    canvas_width = int(canvas["width"])
    canvas_height = int(canvas["height"])
    num_rows = len(world)
    num_cols = max(len(r) for r in world)
    cell_size = min(canvas_height // num_rows, canvas_width // num_cols) - 2
    padding_x = (canvas_width - num_cols * cell_size) // 2
    padding_y = (canvas_height - num_rows * cell_size) // 2

    x, y = padding_x, padding_y
    height = num_rows * cell_size
    width =  num_cols * cell_size
    canvas.create_rectangle(x, y, x + width, y + height,
        fill="gray", outline="gray")

    for row_num, row in enumerate(world):
        for col_num, fill_color in enumerate(row):
            x = padding_x + cell_size * col_num
            y = padding_y + cell_size * row_num
            if fill_color is None:
                fill_color = "gray"
            canvas.create_rectangle(
                x, y, x + cell_size, y + cell_size,
                fill=fill_color, outline=None
            )

def main():
    app = tk.Tk()
    app.title("LTL Planner")

    frame = ttk.LabelFrame(app, text="Transition System")

    canvas_width = 500
    canvas_height = 500
    canvas = tk.Canvas(frame, width=canvas_width, height=canvas_height)
    canvas.pack()
    frame.pack()

    world = [
        [None, "blue", None],
        [None, "yellow", "red"],
        ["white", "blue", None],
        ["green", None, None]
    ]

    render_world(canvas, world)

    app.mainloop()

if __name__ == "__main__":
    main()
