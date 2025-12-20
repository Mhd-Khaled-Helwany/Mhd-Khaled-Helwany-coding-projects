; Simple PLAY test
start:
    LDI R0, #20      ; Sound frequency value
    STORE R0, tone   ; Store to memory
    
    PLAY tone        ; Play the tone
    
    LDI R1, #50      ; Delay counter
delay:
    SUB R1, #1       ; Decrement counter
    BEQ start        ; If zero, play again
    BRA delay        ; Otherwise continue delay

tone: .byte 200       ; Tone frequency