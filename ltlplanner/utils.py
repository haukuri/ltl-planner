import math

from dataclasses import dataclass


def get_item_or_initialize(collection, key, default_factory):
    """
    Tries to get an item from `collection`.

    If it is not present, initializes it with value obtained from `default_factory`
    and returns the initialized value.
    """
    try:
        item = collection[key]
    except KeyError:
        item = default_factory()
        collection[key] = item
    return item


@dataclass
class Vector2D:
    x: float
    y: float

    def magnitude(self):
        "Returns the magnitude/scale/length of the vector"
        return math.sqrt(self.x ** 2 + self.y ** 2)

    def angle(self):
        return math.atan2(self.y, self.x)

    def unit(self):
        angle = self.angle()
        magnitude = 1
        return Vector2D.from_polar(angle, magnitude)

    def __mul__(self, other):
        angle = self.angle()
        magnitude = self.magnitude()
        if other < 0:
            angle += math.pi
        magnitude *= abs(other)
        return Vector2D.from_polar(angle, magnitude)

    def __add__(self, other):
        try:
            x = self.x + other.x
            y = self.y + other.y
        except AttributeError:
            x = self.x + other
            y = self.y + other
        return Vector2D(x, y)

    def __sub__(self, other):
        dx = self.x - other.x
        dy = self.y - other.y
        return Vector2D(dx, dy)

    @classmethod
    def from_polar(cls, angle, magnitude):
        x = magnitude * math.cos(angle)
        y = magnitude * math.sin(angle)
        return cls(x=x, y=y)
