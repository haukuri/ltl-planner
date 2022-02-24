import math
import itertools
import tkinter as tk

from tkinter import ttk

def draw_labels(canvas, offset_x, offset_y, width, height, colors):
    num_colors = len(colors)
    num_rows = int(math.sqrt(num_colors))
    num_cols = math.ceil(num_colors / num_rows)
    dot_size = min(height // num_rows, width // num_cols)
    dot_size = int(dot_size * 0.8) # shrink a little to leave a margin
    padding_x = (width - num_cols * dot_size) // 2
    padding_y = (height - num_rows * dot_size) // 2
    grid_indices = itertools.product(range(num_rows), range(num_cols))
    for (row, col), color in zip(grid_indices, colors):
        print(row, col)
        x = offset_x + padding_x + dot_size * col
        y = offset_y + padding_y + dot_size * row
        canvas.create_oval(
            x, y, x + dot_size - 4, y + dot_size - 4,
            fill=color
        )


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

    for row_num, row in enumerate(world):
        for col_num, colors in enumerate(row):
            x = padding_x + cell_size * col_num
            y = padding_y + cell_size * row_num
            if colors is None:
                canvas.create_rectangle(
                    x, y, x + cell_size, y + cell_size,
                    fill="gray", outline=None
                )
            else:
                canvas.create_rectangle(
                    x, y, x + cell_size, y + cell_size,
                    fill="white", outline=None
                )
                draw_labels(canvas, x, y, cell_size, cell_size, colors)


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
        [None, ["blue", "green"], None],
        [None, ["yellow"], ["blue", "red"]],
        [None, ["blue", "yellow"], None],
        [["green"], None, None]
    ]

    render_world(canvas, world)

    app.mainloop()

if __name__ == "__main__":
    main()
