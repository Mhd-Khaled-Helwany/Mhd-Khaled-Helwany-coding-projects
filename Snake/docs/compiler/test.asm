; Minimal counter (0-10)
    LDI R0, #4 ; 
loop:
    ADD R0, one   ; Add 1 to counter
     CMP R0, ten    ; Check if counter = 10
     BEQ reset         ; If 10, reset
    BRA loop          ; Else continue
    
reset:
    LDI R0, #0        ; Reset to 0
    BRA loop          ; Continue counting
one: .byte 1
ten: .byte 10         ; Maximum value
eleven: .byte 11