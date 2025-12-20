; Test LSR instruction
        LDI R0, #128    ; Load 128 (1000 0000) into R0
        LSR R0          ; Shift right once (should become 64 - 0100 0000)
        LSR R0          ; Shift right again (should become 32 - 0010 0000)
        LSR R0          ; Shift right again (should become 16 - 0001 0000)
        STORE R0, #0    ; Store result in memory location 0