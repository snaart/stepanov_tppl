import pytest
from interpreter.interpreter import Interpreter

class TestInterpreter:
    interpreter = Interpreter()

    def test_simple_add(self):
        assert self.interpreter.eval("2+2") == 4