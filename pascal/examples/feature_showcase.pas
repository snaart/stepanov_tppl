PROGRAM Showcase;
VAR
    i, j : INTEGER;
    x, y : REAL;
    isValid : BOOLEAN;
    greet, name, message : STRING;
BEGIN
    i := 10;
    j := 2 * i + 5;

    x := 3.14;
    y := x * 2.0;

    isValid := TRUE;
    
    greet := 'Hello';
    name := 'World';
    message := greet + ', ' + name + '!';

    BEGIN
        i := i + 1;
        isValid := FALSE;
    END;
END.
