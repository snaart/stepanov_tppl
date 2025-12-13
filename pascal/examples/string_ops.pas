PROGRAM StringOps;
VAR
    s1, s2, s3, res : STRING;
BEGIN
    s1 := 'Alpha';
    s2 := 'Beta';
    s3 := 'Gamma';
    
    res := s1 + s2;
    res := res + s3;
    
    s1 := 'One ' + 'Two ' + 'Three';
END.
