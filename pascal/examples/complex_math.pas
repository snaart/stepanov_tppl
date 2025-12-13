PROGRAM Test; 
VAR
    long_expr, parentheses, precedence, negation, complex : REAL;
BEGIN
    long_expr := 10 * 5 + 4 / 2 - 1;
    parentheses := (10 + 2) * (5 - 3) / 4;
    precedence := 5 + 5 * 5;
    negation := -10 + 20;
    complex := ((1 + 2) * 3) + (4 * (5 + 6));
END.
