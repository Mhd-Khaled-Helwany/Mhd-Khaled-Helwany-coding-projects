
; Instruction test bench

;------------------------------------------------

Load_Immediate:
    LDI R0, #42       ; Load decimal value 42 into R0
    LDI R1, #0xFF     ; Load hex value 0xFF (255) into R1
    LDI R2, #0        ; Load 0 into R2
    LDI R3, #127      ; Load max positive 8-bit signed value

;------------------------------------------------

Load:
    LD R0, test_data1     ; Load value from memory into R0 .byte 25
    LD R1, test_data2     ; Load another value from memory into R1 .byte 75
    LD R2, empty_cell     ; Load from empty memory location

;------------------------------------------------

Store:
    LDI R0, #55           ; Initialize R0 with 55
    STORE R0, output1     ; Store R0 to memory
    LDI R1, #99           ; Initialize R1 with 99
    STORE R1, output2     ; Store R1 to memory
    ; Verify by loading back:
    LD R2, output1        ; Should load 55
    LD R3, output2        ; Should load 99

;------------------------------------------------

Add:
    LDI R0, #5            ; Initialize R0 with 5
    ADD R0, five          ; Add 5 to get 10
    ADD R0, fifteen       ; Add 15 to get 25
    
    ; Test overflow
    LDI R1, #250          ; Initialize with large value
    ADD R1, ten           ; Add to cause overflow (depending on register size)
    
    ; Test zero flag
    LDI R2, #0            ; Initialize with 0
    ADD R2, zero          ; Add zero to test Z flag
    
    ; Test negative flag
    LDI R3, #10           ; Initialize with 10
    ADD R3, neg_fifteen   ; Add -15 to get -5, setting N flag

;------------------------------------------------

Sub:
    LDI R0, #20           ; Initialize R0 with 20
    SUB R0, five          ; Subtract 5 to get 15
    SUB R0, ten           ; Subtract 10 to get 5
    
    ; Test zero flag
    LDI R1, #5            ; Initialize with 5
    SUB R1, five          ; Subtract 5 to get 0, setting Z flag
    
    ; Test negative flag
    LDI R2, #10           ; Initialize with 10
    SUB R2, fifteen       ; Subtract 15 to get -5, setting N flag
    
    ; Test borrow (carry)
    LDI R3, #0            ; Initialize with 0
    SUB R3, one           ; Subtract 1 to test C flag

;------------------------------------------------

Or:
    LDI R0, #0x55         ; Initialize with 01010101
    OR R0, pattern_aa     ; OR with 10101010 to get 11111111
    
    ; Test zero flag
    LDI R1, #0            ; Initialize with 0
    OR R1, zero           ; OR with 0 to test Z flag
    
    ; Test with different patterns
    LDI R2, #0x03         ; Initialize with 00000011
    OR R2, pattern_30     ; OR with 00110000 to get 00110011

;------------------------------------------------

Logical_Shift_Right:
    LDI R0, #8            ; Initialize with 8 (00001000)
    LSR R0, shift_one     ; Shift right by 1 to get 4 (00000100)
    LSR R0, shift_one     ; Shift right by 1 again to get 2 (00000010)
    LSR R0, shift_one     ; Shift right by 1 again to get 1 (00000001)
    LSR R0, shift_one     ; Shift right by 1 again to get 0 (00000000), sets Z flag
    
    ; Test carry flag
    LDI R1, #1            ; Initialize with 1
    LSR R1, shift_one     ; Shift right, last bit goes to carry

;------------------------------------------------

Compare:
    LDI R0, #10           ; Initialize with 10
    CMP R0, ten           ; Compare with 10, sets Z=1
    
    LDI R1, #15           ; Initialize with 15
    CMP R1, ten           ; Compare with 10, sets C=1 (15>10)
    
    LDI R2, #5            ; Initialize with 5
    CMP R2, ten           ; Compare with 10, sets N=1 (5<10)

;------------------------------------------------

Branch:
    LDI R0, #0            ; Initialize counter
bra_loop:
    ADD R0, one           ; Increment counter
    CMP R0, five          ; Check if counter = 5
    BEQ bra_exit          ; If counter = 5, exit loop
    BRA bra_loop          ; Otherwise, branch back to loop
bra_exit:

;------------------------------------------------

Branch_Equal:
    LDI R0, #0            ; Initialize counter
beq_loop:
    ADD R0, one           ; Increment counter
    CMP R0, five          ; Compare with 5
    BEQ beq_exit          ; Branch if equal (when counter = 5)
    BRA beq_loop          ; Otherwise continue loop
beq_exit:

;------------------------------------------------

Move:
    LDI R0, #42           ; Load value into R0
    MOV R1, R0            ; Copy R0 to R1, R1 now has 42
    
    LDI R2, #0            ; Load 0 into R2
    MOV R3, R2            ; Copy R2 to R3, sets Z flag
    
    LDI R0, #0x80         ; Load negative value (assuming 8-bit signed)
    MOV R1, R0            ; Copy R0 to R1, sets N flag

;------------------------------------------------

Branch_If_Plus:
    LDI R0, #5            ; Initialize with positive value
    ADD R0, neg_ten       ; Add -10 to get -5
bpl_loop:
    ADD R0, one           ; Add 1 to R0
    CMP R0, zero          ; Compare with 0
    BPL bpl_exit          ; Branch if positive (N=0)
    BRA bpl_loop          ; Otherwise continue
bpl_exit:

;------------------------------------------------

Read_Key0:
    KEY0 R0               ; Read player 0 input into R0
    CMP R0, zero          ; Check if any key was pressed
    BEQ no_key0           ; Branch if no key pressed
    STORE R0, last_key0   ; Store the key pressed
no_key0:

;------------------------------------------------

Play_Sound:
    PLAY sound_data       ; Play sound stored at sound_data
    LDI R0, #100          ; Short delay
delay_loop:
    SUB R0, one
    BPL delay_loop

;------------------------------------------------

Read_Uart:
    UART R0, uart_read    ; Read UART data into R0
    LDI R1, #65           ; ASCII 'A'
    STORE R1, uart_write
    UART R1, uart_write   ; Send through UART

;------------------------------------------------

Read_Key1:
    KEY1 R0               ; Read player 1 input into R0
    CMP R0, zero          ; Check if any key was pressed
    BEQ no_key1           ; Branch if no key pressed
    STORE R0, last_key1   ; Store the key pressed
no_key1:

;------------------------------------------------

; Full Computer test

game_loop:
    ; Read player inputs
    KEY0 R0               ; Player 0 input
    KEY1 R1               ; Player 1 input
    
check_p0:
    CMP R0, zero          ; Check if key pressed
    BEQ check_p1          ; Skip if no key
    STORE R0, p0_pos      ; Update position
    
check_p1:
    CMP R1, zero          ; Check if key pressed
    BEQ continue_game     ; Skip if no key
    STORE R1, p1_pos      ; Update position
    
continue_game:
    ; Check for collision
    LD R2, p0_pos
    LD R3, p1_pos
    CMP R2, R3            ; Compare positions
    BEQ collision         ; Branch if collision
    
    BRA game_loop         ; Continue game loop
    
collision:
    PLAY collision_sound  ; Play collision sound
    BRA game_loop         ; Restart game loop

;------------------------------------------------

; Used Variables

last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1
last_key1:    .byte 0    ; Last key pressed by player 1




test_data1:   .byte 25
test_data2:   .byte 75
empty_cell:   .byte 0
output1:      .byte 0    ; Space for storing output
output2:      .byte 0    ; Space for storing output

zero:         .byte 0
one:          .byte 1
five:         .byte 5
ten:          .byte 10
fifteen:      .byte 15
neg_ten:      .byte 246  ; -10 in two's complement (assuming 8-bit)
neg_fifteen:  .byte 65521  ; -15 in two's complement (assuming 8-bit)

pattern_aa:   .byte 0xAA ; 10101010
pattern_30:   .byte 0x30 ; 00110000

shift_one:    .byte 1    ; Shift right by 1

sound_data:   .byte 60    ; Example sound data
collision_sound: .byte 120

uart_read:    .byte 0    ; Buffer for UART read
uart_write:   .byte 0    ; Buffer for UART write

p0_pos:       .byte 0    ; Player 0 position
p1_pos:       .byte 0    ; Player 1 position
last_key0:    .byte 0    ; Last key pressed by player 0
last_key1:    .byte 0    ; Last key pressed by player 1
