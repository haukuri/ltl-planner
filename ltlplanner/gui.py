import math
import itertools
import time
import tkinter as tk

from pathlib import Path

from PIL import ImageTk, Image

from ltlplanner.utils import Vector2D


class _Assets:
    def __init__(self) -> None:
        self._robot_image = {}

    def robot_image(self, height, width):
        size = (int(height), int(width))
        if size not in self._robot_image:
            icon_path = Path(__file__).parent / "robot.png"
            im = Image.open(icon_path)
            print(im.mode)
            resized = im.resize(size=size)
            tk_image = ImageTk.PhotoImage(resized)
            self._robot_image[size] = tk_image, resized
        return self._robot_image[size]


assets = _Assets()


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
    agent_width = width * 0.5
    agent_height = width * 0.5
    padding_x = (width - agent_width) / 2
    padding_y = (height - agent_height) / 2
    canvas.create_polygon(
        offset_x + width / 2,
        offset_y + padding_y,
        offset_x + padding_x,
        offset_y + height - padding_y,
        offset_x + width - padding_x,
        offset_y + height - padding_y,
    )


def draw_agent_robot(canvas, offset_x, offset_y, width, height):
    scale = 0.5
    agent_height = int(height * scale)
    agent_width = int(width * scale)
    padding_x = (width - agent_width) // 2
    padding_y = (height - agent_height) // 2
    tk_img = assets.robot_image(agent_height, agent_width)[0]
    x = offset_x + padding_x
    y = offset_y + padding_y
    canvas.create_image(x, y, image=tk_img, anchor=tk.NW)


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
    t_start = time.time()
    canvas.delete("all")  # erase everything
    canvas_width = int(canvas["width"])
    canvas_height = int(canvas["height"])
    num_rows = len(world)
    num_cols = max(len(r) for r in world)
    (
        cell_size,
        padding_x,
        padding_y,
    ) = grid_layout_params(num_rows, num_cols, canvas_height, canvas_width)

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

    agent_x = padding_x + cell_size * agent_position.x
    agent_y = padding_y + cell_size * agent_position.y
    draw_agent_robot(canvas, agent_x, agent_y, cell_size, cell_size)
    print("render_world elapsed time:", (time.time() - t_start) * 1_000, "ms")


class Animator:
    def __init__(self, duration, initial_value, target_value):
        self.duration = duration
        self.initial_value = initial_value
        self.target_value = target_value
        self.t_start = -1
        self.relative_elapsed = 0.0
        self.t_now = -1

    def start(self):
        self.t_start = time.time()
        self.schedule()

    def schedule(self):
        raise NotImplementedError

    def set_value(self, value):
        "Override this to set the interpolated value"
        raise NotImplementedError

    def interpolate(self):
        """
        Interpolate between the initial and target value given the relative time
        that has elapsed (between 0.0 and 1.0).
        """
        delta = self.target_value - self.initial_value
        return self.initial_value + delta * self.relative_elapsed

    def tick(self):
        self.t_now = time.time()
        elapsed = self.t_now - self.t_start
        self.relative_elapsed = elapsed / self.duration
        if self.relative_elapsed >= 1:
            self.set_value(self.target_value)
            return
        value = self.interpolate()
        self.set_value(value)
        self.schedule()


def main():
    app = tk.Tk()
    app.title("LTL Planner")

    canvas_width = 500
    canvas_height = 500
    canvas = tk.Canvas(app, width=canvas_width, height=canvas_height)
    canvas.pack()

    world = [
        [None, ["blue", "green"], None],
        [None, ["yellow"], ["blue", "red", "salmon", "orange", "khaki"]],
        [None, ["blue", "yellow"], None],
        [["green"], None, None],
    ]
    agent_position = Vector2D(1, 0)
    target_position = Vector2D(1, 1)

    class AgentAnimator(Animator):
        def __init__(self):
            super().__init__(1, agent_position, target_position)

        def schedule(self):
            app.after(16, self.tick)

        def set_value(self, value):
            nonlocal agent_position
            agent_position = value
            render_world(canvas, world, agent_position)

    animator = AgentAnimator()
    animator.start()
    app.mainloop()


if __name__ == "__main__":
    main()
