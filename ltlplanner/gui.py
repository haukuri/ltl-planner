import math
import itertools
import tkinter as tk

from tkinter import ttk
from pathlib import Path

def colorspace():
    color_file = Path(__file__).parent / "tk_colors.txt"
    with color_file.open("r") as f:
        return f.readlines()

def grid_layout_params(num_rows, num_cols, height, width, fill_ratio=1.0):
    cell_size = min(height // num_rows, width // num_cols) - 2
    cell_size = int(cell_size * fill_ratio)  # shrink a little to leave a margin
    padding_x = (width - num_cols * cell_size) // 2
    padding_y = (height - num_rows * cell_size) // 2
    return cell_size, padding_x, padding_y

def draw_labels(canvas, offset_x, offset_y, width, height, colors):
    num_colors = len(colors)
    num_rows = int(math.sqrt(num_colors))
    num_cols = math.ceil(num_colors / num_rows)
    cell_size, padding_x, padding_y = grid_layout_params(
        num_rows, num_cols, height, width, fill_ratio=0.8
    )
    grid_indices = itertools.product(range(num_rows), range(num_cols))
    for (row, col), color in zip(grid_indices, colors):
        x = offset_x + padding_x + cell_size * col
        y = offset_y + padding_y + cell_size * row
        canvas.create_oval(x, y, x + cell_size - 4, y + cell_size - 4, fill=color)

def draw_agent(canvas, offset_x, offset_y, width, height):
    agent_width = width * 0.3
    agent_height = width * 0.3
    padding_x = (width - agent_width) / 2
    padding_y = (height - agent_height) / 2
    canvas.create_polygon(
        offset_x + width / 2, offset_y + padding_y,
        offset_x + padding_x, offset_y + height - padding_y,
        offset_x + width - padding_x, offset_y + height - padding_y,
    )

def iter_2d(rows):
    for row_num, row in enumerate(rows):
        for col_num, item in enumerate(row):
            yield row_num, col_num, item

def render_world(canvas, world, agent_position):
    """
    Renders a 2D "world" board onto a canvas.

    Assumes that `world` is of the form

    world = [
        [None, ["blue", "green"], None],
        [None, ["yellow"], ["blue", "red", "salmon", "orange", "khaki"]],
        [None, ["blue", "yellow"], None],
        [["green"], None, None],
    ]
    """
    canvas_width = int(canvas["width"])
    canvas_height = int(canvas["height"])
    num_rows = len(world)
    num_cols = max(len(r) for r in world)
    cell_size, padding_x, padding_y, = grid_layout_params(
        num_rows, num_cols, canvas_height, canvas_width
    )

    x, y = padding_x, padding_y

    for row_num, col_num, colors in iter_2d(world):
        x = padding_x + cell_size * col_num
        y = padding_y + cell_size * row_num
        if colors is None:
            # draw obstacle
            canvas.create_rectangle(
                x, y, x + cell_size, y + cell_size, fill="gray", outline=None
            )
        else:
            # draw regular cell
            canvas.create_rectangle(
                x, y, x + cell_size, y + cell_size, fill="white", outline=None
            )
            draw_labels(canvas, x, y, cell_size, cell_size, colors)
        if (row_num, col_num) == agent_position:
            draw_agent(canvas, x, y, cell_size, cell_size)

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
        [None, ["yellow"], ["blue", "red", "salmon", "orange", "khaki"]],
        [None, ["blue", "yellow"], None],
        [["green"], None, None],
    ]
    agent_position = (0, 1)
    render_world(canvas, world, agent_position)

    app.mainloop()


if __name__ == "__main__":
    main()
