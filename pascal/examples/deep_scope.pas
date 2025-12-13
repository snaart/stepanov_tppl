PROGRAM Test; 
VAR
    level0, level1, level2, level3, sum_levels, level1_modified, final_val : REAL;
BEGIN
    level0 := 0;
    BEGIN
        level1 := 1;
        BEGIN
            level2 := 2;
            BEGIN
                level3 := 3;
                sum_levels := level0 + level1 + level2 + level3;
            END;
        END;
        level1_modified := level1 + 10;
    END;
    final_val := 100;
END.
