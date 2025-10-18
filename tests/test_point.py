import pytest

from plib import Point


@pytest.fixture
def point_1():
    return Point(0, 0)


@pytest.fixture
def point_2():
    return Point(2, 2)


class TestPoint:

    def test_creation(self, point_1):
        assert point_1.x == 0 and point_1.y == 0

        with pytest.raises(TypeError):
            Point(1.5, 1.5)

    def test_add(self, point_1, point_2):
        assert point_1 + point_2 == Point(2, 2)

    def test_iadd(self, point_1, point_2):
        point_1 += point_2
        assert point_1 == Point(2, 2)

    def test_sub(self, point_1, point_2):
        assert point_1 - point_2 == -Point(2, 2)
        assert point_2 - point_1 == Point(2, 2)

    def test_isub(self, point_1, point_2):
        point_2 -= point_1
        assert point_2 == Point(2, 2)

    def test_neg(self, point_2):
        assert -point_2 == Point(-2, -2)

    def test_distance_to(self, point_1):
        point2 = Point(2, 0)
        assert point_1.distance_to(point2) == 2

    @pytest.mark.parametrize(
        "point_1, point_2, distance",
        [(Point(0, 0), Point(0, 10), 10),
         (Point(0, 0), Point(10, 0), 10),
         (Point(0, 0), Point(1, 1), 1.414)]
    )
    def test_distance_all_axis(self, point_1, point_2, distance):
        assert point_1.distance_to(point_2) == pytest.approx(distance, 0.001)

    def test_str(self, point_1):
        assert isinstance(str(point_1), str)
        assert str(point_1) == "Point(0, 0)"

    def test_repr(self, point_2):
        assert repr(point_2) == "Point(2, 2)"

    def test_is_center(self, point_1, point_2):
        assert point_1.is_center() is True
        assert point_2.is_center() is False

    def test_json_conversion(self, point_2):
        json_string = point_2.to_json()
        expected_json = '{"x": 2, "y": 2}'

        assert json_string == expected_json

        new_point = Point.from_json(json_string)
        assert new_point == point_2