; Simple CMP test
start:
    LDI R0, #10      ; R0 = 10
    LDI R1, #5       ; R1 = 5
    STORE R1, value  ; Store 5 to memory
    
    CMP R0, value    ; Compare R0 with memory value
    BEQ equal        ; Branch if equal (shouldn't take)
    BPL positive     ; Branch if positive (should take)
    BRA start        ; Loop back if negative (shouldn't take)
    
equal:
    LDI R2, #1       ; This shouldn't execute
    BRA start
    
positive:
    LDI R3, #1       ; Should execute this
    BRA start

value: .byte 5       ; Test value