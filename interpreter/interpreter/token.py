from enum import Enum, auto

class TokenType(Enum):
    NUMBER = auto()
    OPERATOR = auto()

class Token():
    def __init__(self, type_: TokenType, value: str):
        self.type = type_
        self.value = value

    def __str__(self):
        return f"{self.__class__.__name__}({self.type}, {self.value})"

