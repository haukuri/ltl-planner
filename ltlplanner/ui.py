import pygame
import sys

from dataclasses import dataclass, field

BLACK = (0, 0, 0)
WHITE = (200, 200, 200)
WINDOW_HEIGHT = 400
WINDOW_WIDTH = 400


@dataclass
class Cell:
    x: int
    y: int
    width: int
    height: int
    labels: set[str] = field(default_factory=set, init=False)

    def contains(self, x, y):
        start_x = self.x
        start_y = self.y
        end_x = start_x + self.width
        end_y = start_y + self.height
        return start_x <= x <= end_x and start_y <= y <= end_y


def main():
    pygame.init()
    screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
    clock = pygame.time.Clock()
    block_size = 20  # Set the size of the grid block
    screen.fill(BLACK)

    cells = []
    for x in range(0, WINDOW_WIDTH, block_size):
        for y in range(0, WINDOW_HEIGHT, block_size):
            cell = Cell(x=x, y=y, width=block_size, height=block_size)
            cells.append(cell)

    def draw_grid():
        for cell in cells:
            rect = pygame.Rect(cell.x, cell.y, cell.width, cell.height)
            width = 4 if "w" in cell.labels else 1
            pygame.draw.rect(screen, WHITE, rect, width=width)

    mouse_down = False

    def add_wall(cell: Cell):
        cell.labels.add("w")

    def remove_wall(cell: Cell):
        if "w" in cell.labels:
            cell.labels.remove("w")

    mouse_action = add_wall
    while True:
        draw_grid()
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            elif event.type == pygame.MOUSEBUTTONDOWN:
                mouse_down = True
                x, y = event.dict["pos"]
                for cell in cells:
                    if cell.contains(x, y):
                        mouse_action = remove_wall if "w" in cell.labels else add_wall
            elif event.type == pygame.MOUSEBUTTONUP:
                mouse_down = False
            if event.type == pygame.MOUSEMOTION and mouse_down:
                x, y = event.dict["pos"]
                for cell in cells:
                    if cell.contains(x, y):
                        mouse_action(cell)
            print(event)

        pygame.display.update()
        clock.tick(60)


if __name__ == "__main__":
    main()
