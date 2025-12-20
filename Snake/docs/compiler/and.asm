; Simple AND test
start:
    LDI R0, #0xFF    ; R0 = 11111111
    LDI R1, #0xAA    ; R1 = 10101010
    STORE R1, mask   ; Store mask to memory
    
    AND R0, mask     ; R0 AND mask (should get 10101010)
    
    CMP R0, test    ; Compare result
    BEQ correct      ; Branch if correct
    BRA start        ; Loop back if wrong
    
correct:
    LDI R2, #1       ; Set flag for correct result
    BRA start        ; Loop back

mask: .byte 0xAA     ; Test mask
test: .byte 0xAA     ; Test mask