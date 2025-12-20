; Simple KEY0/KEY1 test
start:
    KEY0 R0          ; Read player 0 input into R0
    KEY1 R1          ; Read player 1 input into R1
    
    STORE R0, p0keys ; Store inputs to memory
    STORE R1, p1keys
    
    BRA start        ; Loop continuously

p0keys: .byte 0      ; Storage for player 0 keys
p1keys: .byte 0      ; Storage for player 1 keys