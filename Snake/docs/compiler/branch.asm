; Simple BRA test
start:
    LDI R0, #0       ; Initialize R0
    BRA target       ; Branch forward
    
    LDI R0, #5       ; This should be skipped
    
target:
    LDI R1, #1       ; R1 = 1
    BRA start        ; Branch back to start