; Simple BEQ test
start:
    LDI R0, #10      ; R0 = 10
    LDI R1, #10      ; R1 = 10
    LDI R2, #0       ; R2 = 0
    
    CMP R0, R1       ; Compare R0 with R1 (equal)
    BEQ equal        ; Should branch
    
    LDI R2, #5       ; Should be skipped
    BRA start
    
equal:
    LDI R3, #1       ; Should execute
    BRA start