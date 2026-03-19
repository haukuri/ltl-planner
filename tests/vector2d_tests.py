import math
import numbers

from ltlplanner.utils import Vector2D, get_item_or_initialize


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


def test_vector2d_magnitude():
    v = Vector2D(3, 4)
    assert math.isclose(v.magnitude(), 5.0)


def test_vector2d_magnitude_zero():
    v = Vector2D(0, 0)
    assert math.isclose(v.magnitude(), 0.0)


def test_vector2d_angle_along_x_axis():
    v = Vector2D(1, 0)
    assert math.isclose(v.angle(), 0.0)


def test_vector2d_angle_along_y_axis():
    v = Vector2D(0, 1)
    assert math.isclose(v.angle(), math.pi / 2)


def test_vector2d_unit_has_magnitude_one():
    v = Vector2D(3, 4)
    u = v.unit()
    assert math.isclose(u.magnitude(), 1.0)


def test_vector2d_from_polar_round_trips():
    angle = math.pi / 4
    magnitude = 5.0
    v = Vector2D.from_polar(angle, magnitude)
    assert math.isclose(v.magnitude(), magnitude)
    assert math.isclose(v.angle(), angle)


def test_vector2d_add_scalar():
    v = Vector2D(1, 2)
    result = v + 3
    assert isinstance(result, Vector2D)
    assert math.isclose(result.x, 4)
    assert math.isclose(result.y, 5)


# --- get_item_or_initialize ---

def test_get_item_or_initialize_returns_existing():
    d = {"key": 42}
    result = get_item_or_initialize(d, "key", list)
    assert result == 42


def test_get_item_or_initialize_creates_missing():
    d = {}
    result = get_item_or_initialize(d, "key", list)
    assert result == []
    assert d["key"] == []


def test_get_item_or_initialize_stores_new_value():
    d = {}
    get_item_or_initialize(d, "k", lambda: {"nested": True})
    assert d["k"] == {"nested": True}
