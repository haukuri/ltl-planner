from ltlplanner.gui import Vector2D

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
