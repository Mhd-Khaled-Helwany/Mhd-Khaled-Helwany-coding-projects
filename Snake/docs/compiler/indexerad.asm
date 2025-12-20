START:
LD GR3, address
LDI GR1, #5
LDI GR0, #10
STORE GR1, [0]
LD GR2, [0]
BRA START

address: .byte 29957
