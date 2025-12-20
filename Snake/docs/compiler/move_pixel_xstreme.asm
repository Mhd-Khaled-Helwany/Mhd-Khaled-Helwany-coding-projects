;Testbench for keyboard movement with A and D keys
;A (0x61) moves left, D (0x64) moves right
;Also supports arrow keys: left arrow (0x25) and right arrow (0x27)

pos:    .byte 29952             ; Starting position (middle)
min_pos: .byte 0             ; Minimum position (left edge)
max_pos: .byte 20            ; Maximum position (right edge)
key:    .byte 0              ; Storage for keyboard input
background : .byte 0
player : .byte 5

start:
    

loop:

        ; Read keyboard input
        KEY0 GR2
        STORE GR2, M, key    ; Store key value for debugging

        ; Check if D or right arrow pressed
        CMP GR2, #0x23       ; 'd' key
        BEQ move_right


        ; Check if A or left arrow pressed
        CMP GR2, #0x1C       ; 'a' key
        BEQ move_left

        ; No movement if other key or no key pressed
        BRA loop

move_right:
        LD GR3, M, pos
        LD GR0, background
        STORE GR0, [0]
        ADD GR3, #1          ; Move position right
                STORE GR3, M, pos

        BRA update

move_left:
        LD GR3, M, pos
        LD GR0, background
        STORE GR0, [0]
        SUB GR3, #1          ; Move position left
        STORE GR3, M, pos

        BRA update


update:
        LD GR2, player
        STORE GR2, [0]    ; Save new position
        BRA loop

