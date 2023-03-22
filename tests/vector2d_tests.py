import math
import numbers

from ltlplanner.utils import Vector2D


def test_vector2d_subtraction():
    v1 = Vector2D(4, 4)
    v2 = Vector2D(1, 2)
    v3 = v1 - v2
    assert v3.x == 3
    assert v3.y == 2

    agent_position = Vector2D(1, 0)
    target_position = Vector2D(1, 1)
    delta = agent_position - target_position
    assert delta.x == 0
    assert delta.y == -1


def test_vector2d_addition():
    v1 = Vector2D(4, 4)
    v2 = Vector2D(1, 2)
    v3 = v1 + v2
    assert v3.x == 5
    assert v3.y == 6


def test_vector2d_multiplication_with_another_vector2d():
    v1 = Vector2D(4, 4)
    v2 = Vector2D(1, 2)
    product = v1 * v2

    assert isinstance(product, numbers.Real)
    assert math.isclose(product, 12)


def test_vector2d_multiplication_with_a_real():
    v1 = Vector2D(4, 4)
    product = v1 * 2

    assert isinstance(product, Vector2D)
    assert math.isclose(product.x, 8)
    assert math.isclose(product.y, 8)
