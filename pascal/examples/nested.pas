PROGRAM Test; 
VAR
    x, y, a, b, c : REAL;
BEGIN
    y := 2;
    BEGIN
        a := 3;
        a := a;
        b := 10 + a + 10 * y / 4;
        c := a - b
    END;
    x := 11;
END.
