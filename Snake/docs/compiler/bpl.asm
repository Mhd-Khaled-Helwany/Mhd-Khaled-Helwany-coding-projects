; Simple BPL test
start:
    LDI R0, #10      ; R0 = 10
    LDI R1, #5       ; R1 = 5
    
    CMP R0, R1       ; Compare R0 > R1 (positive result)
    BPL positive     ; Should branch
    
    LDI R2, #5       ; Should be skipped
    BRA start
    
positive:
    LDI R3, #1       ; Should execute
    BRA start