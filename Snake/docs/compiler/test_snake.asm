
; Main program
START:
    ; Assume existing input handling sets DIR
    ; (e.g., KEY0 GR0 to read input, then store to DIR)

    ; Delay loop
    LDI GR0, #255
DELAY:
    SUB GR0, ONE
    CMP GR0, ZERO
    BEQ DELAY_END
    BRA DELAY
DELAY_END:

    ; Check if direction is set
    LD GR0, DIR
    CMP GR0, ZERO
    BEQ START

    ; Load current head position
    LD GR2, HEAD_IDX
    CMP GR2, ZERO
    BEQ LOAD_HEAD_0
    CMP GR2, ONE
    BEQ LOAD_HEAD_1
    CMP GR2, TWO
    BEQ LOAD_HEAD_2
    CMP GR2, THREE
    BEQ LOAD_HEAD_3
    CMP GR2, FOUR
    BEQ LOAD_HEAD_4

LOAD_HEAD_0:
    LD GR1, SNAKE_X0
    STORE GR1, CURRENT_X
    LD GR1, SNAKE_Y0
    STORE GR1, CURRENT_Y
    BRA LOAD_HEAD_DONE
LOAD_HEAD_1:
    LD GR1, SNAKE_X1
    STORE GR1, CURRENT_X
    LD GR1, SNAKE_Y1
    STORE GR1, CURRENT_Y
    BRA LOAD_HEAD_DONE
LOAD_HEAD_2:
    LD GR1, SNAKE_X2
    STORE GR1, CURRENT_X
    LD GR1, SNAKE_Y2
    STORE GR1, CURRENT_Y
    BRA LOAD_HEAD_DONE
LOAD_HEAD_3:
    LD GR1, SNAKE_X3
    STORE GR1, CURRENT_X
    LD GR1, SNAKE_Y3
    STORE GR1, CURRENT_Y
    BRA LOAD_HEAD_DONE
LOAD_HEAD_4:
    LD GR1, SNAKE_X4
    STORE GR1, CURRENT_X
    LD GR1, SNAKE_Y4
    STORE GR1, CURRENT_Y

LOAD_HEAD_DONE:
    ; Calculate new head position based on direction
    LD GR0, DIR
    CMP GR0, ONE        ; Up
    BEQ MOVE_UP
    CMP GR0, TWO        ; Right
    BEQ MOVE_RIGHT
    CMP GR0, THREE      ; Down
    BEQ MOVE_DOWN
    CMP GR0, FOUR       ; Left
    BEQ MOVE_LEFT
    BRA START           ; Invalid direction

MOVE_UP:
    LD GR1, CURRENT_X
    STORE GR1, NEW_X
    LD GR1, CURRENT_Y
    SUB GR1, ONE
    STORE GR1, NEW_Y
    BRA CHECK_BOUNDS
MOVE_RIGHT:
    LD GR1, CURRENT_X
    ADD GR1, ONE
    STORE GR1, NEW_X
    LD GR1, CURRENT_Y
    STORE GR1, NEW_Y
    BRA CHECK_BOUNDS
MOVE_DOWN:
    LD GR1, CURRENT_X
    STORE GR1, NEW_X
    LD GR1, CURRENT_Y
    ADD GR1, ONE
    STORE GR1, NEW_Y
    BRA CHECK_BOUNDS
MOVE_LEFT:
    LD GR1, CURRENT_X
    SUB GR1, ONE
    STORE GR1, NEW_X
    LD GR1, CURRENT_Y
    STORE GR1, NEW_Y

CHECK_BOUNDS:
    ; Check X bounds (0 <= NEW_X <= 19)
    LD GR1, NEW_X
    CMP GR1, ZERO
    BPL CHECK_X_MAX
    BRA START           ; NEW_X < 0
CHECK_X_MAX:
    LD GR1, MAX_X
    CMP GR1, NEW_X
    BPL CHECK_Y_BOUNDS
    BRA START           ; NEW_X > MAX_X

CHECK_Y_BOUNDS:
    ; Check Y bounds (0 <= NEW_Y <= 14)
    LD GR1, NEW_Y
    CMP GR1, ZERO
    BPL CHECK_Y_MAX
    BRA START           ; NEW_Y < 0
CHECK_Y_MAX:
    LD GR1, MAX_Y
    CMP GR1, NEW_Y
    BPL CHECK_COLLISION
    BRA START           ; NEW_Y > MAX_Y

CHECK_COLLISION:
    ; Reset collision flag
    LDI GR1, #0
    STORE GR1, COLLISION_FLAG

    ; Check all snake positions
    LDI GR2, #0
COLLISION_LOOP:
    CMP GR2, ZERO
    BEQ COL_CHECK_0
    CMP GR2, ONE
    BEQ COL_CHECK_1
    CMP GR2, TWO
    BEQ COL_CHECK_2
    CMP GR2, THREE
    BEQ COL_CHECK_3
    CMP GR2, FOUR
    BEQ COL_CHECK_4

COL_CHECK_0:
    LD GR1, SNAKE_X0
    STORE GR1, TEMP_X
    LD GR1, SNAKE_Y0
    STORE GR1, TEMP_Y
    BRA DO_CHECK
COL_CHECK_1:
    LD GR1, SNAKE_X1
    STORE GR1, TEMP_X
    LD GR1, SNAKE_Y1
    STORE GR1, TEMP_Y
    BRA DO_CHECK
COL_CHECK_2:
    LD GR1, SNAKE_X2
    STORE GR1, TEMP_X
    LD GR1, SNAKE_Y2
    STORE GR1, TEMP_Y
    BRA DO_CHECK
COL_CHECK_3:
    LD GR1, SNAKE_X3
    STORE GR1, TEMP_X
    LD GR1, SNAKE_Y3
    STORE GR1, TEMP_Y
    BRA DO_CHECK
COL_CHECK_4:
    LD GR1, SNAKE_X4
    STORE GR1, TEMP_X
    LD GR1, SNAKE_Y4
    STORE GR1, TEMP_Y

DO_CHECK:
    LD GR1, TEMP_X
    CMP GR1, NEW_X
    BEQ CHECK_Y_COL
    BRA NEXT_COL
CHECK_Y_COL:
    LD GR1, TEMP_Y
    CMP GR1, NEW_Y
    BEQ SET_COLLISION
NEXT_COL:
    ADD GR2, ONE
    CMP GR2, FIVE
    BEQ COLLISION_DONE
    BRA COLLISION_LOOP
SET_COLLISION:
    LDI GR1, #1
    STORE GR1, COLLISION_FLAG
    BRA START

COLLISION_DONE:
    LD GR1, COLLISION_FLAG
    CMP GR1, ZERO
    BEQ MOVE_SNAKE
    BRA START           ; Collision detected

MOVE_SNAKE:
    ; Erase tail tile
    LD GR2, TAIL_IDX
    CMP GR2, ZERO
    BEQ ERASE_TAIL_0
    CMP GR2, ONE
    BEQ ERASE_TAIL_1
    CMP GR2, TWO
    BEQ ERASE_TAIL_2
    CMP GR2, THREE
    BEQ ERASE_TAIL_3
    CMP GR2, FOUR
    BEQ ERASE_TAIL_4

ERASE_TAIL_0:
    LD GR0, SNAKE_X0
    LD GR1, SNAKE_Y0
    BRA ERASE_TILE
ERASE_TAIL_1:
    LD GR0, SNAKE_X1
    LD GR1, SNAKE_Y1
    BRA ERASE_TILE
ERASE_TAIL_2:
    LD GR0, SNAKE_X2
    LD GR1, SNAKE_Y2
    BRA ERASE_TILE
ERASE_TAIL_3:
    LD GR0, SNAKE_X3
    LD GR1, SNAKE_Y3
    BRA ERASE_TILE
ERASE_TAIL_4:
    LD GR0, SNAKE_X4
    LD GR1, SNAKE_Y4

ERASE_TILE:
    LD GR3, LOW_BOUND
    ADD GR3, GR0        ; GR3 += X
    LD GR2, GR1         ; GR2 = Y
    CMP GR2, ZERO
    BEQ SET_ERASE_ADDR
ERASE_Y_LOOP:
    ADD GR3, TWENTY
    SUB GR2, ONE
    CMP GR2, ZERO
    BEQ SET_ERASE_ADDR
    BRA ERASE_Y_LOOP
SET_ERASE_ADDR:
    LDI GR1, #0
    STORE GR1, [0]      ; Erase tile

    ; Update snake positions (new head at next index)
    LD GR2, HEAD_IDX
    ADD GR2, ONE
    CMP GR2, FIVE
    BEQ WRAP_HEAD
    BRA SET_HEAD
WRAP_HEAD:
    LDI GR2, #0
SET_HEAD:
    CMP GR2, ZERO
    BEQ STORE_NEW_0
    CMP GR2, ONE
    BEQ STORE_NEW_1
    CMP GR2, TWO
    BEQ STORE_NEW_2
    CMP GR2, THREE
    BEQ STORE_NEW_3
    CMP GR2, FOUR
    BEQ STORE_NEW_4

STORE_NEW_0:
    LD GR1, NEW_X
    STORE GR1, SNAKE_X0
    LD GR1, NEW_Y
    STORE GR1, SNAKE_Y0
    BRA UPDATE_INDICES
STORE_NEW_1:
    LD GR1, NEW_X
    STORE GR1, SNAKE_X1
    LD GR1, NEW_Y
    STORE GR1, SNAKE_Y1
    BRA UPDATE_INDICES
STORE_NEW_2:
    LD GR1, NEW_X
    STORE GR1, SNAKE_X2
    LD GR1, NEW_Y
    STORE GR1, SNAKE_Y2
    BRA UPDATE_INDICES
STORE_NEW_3:
    LD GR1, NEW_X
    STORE GR1, SNAKE_X3
    LD GR1, NEW_Y
    STORE GR1, SNAKE_Y3
    BRA UPDATE_INDICES
STORE_NEW_4:
    LD GR1, NEW_X
    STORE GR1, SNAKE_X4
    LD GR1, NEW_Y
    STORE GR1, SNAKE_Y4

UPDATE_INDICES:
    ; Update HEAD_IDX
    STORE GR2, HEAD_IDX

    ; Update TAIL_IDX
    LD GR2, TAIL_IDX
    ADD GR2, ONE
    CMP GR2, FIVE
    BEQ WRAP_TAIL
    BRA SET_TAIL
WRAP_TAIL:
    LDI GR2, #0
SET_TAIL:
    STORE GR2, TAIL_IDX

    ; Draw new head
    LD GR0, NEW_X
    LD GR1, NEW_Y
    LD GR3, LOW_BOUND
    ADD GR3, GR0        ; GR3 += X
    LD GR2, GR1         ; GR2 = Y
    CMP GR2, ZERO
    BEQ SET_DRAW_ADDR
DRAW_Y_LOOP:
    ADD GR3, TWENTY
    SUB GR2, ONE
    CMP GR2, ZERO
    BEQ SET_DRAW_ADDR
    BRA DRAW_Y_LOOP
SET_DRAW_ADDR:
    LDI GR1, #5
    STORE GR1, [0]      ; Draw tile

    BRA START
; Constants
LOW_BOUND:      .byte 0x1000    ; Base address of tile memory
ZERO:           .byte 0          ; Constant 0
ONE:            .byte 1          ; Constant 1
TWO:            .byte 2          ; Constant 2
THREE:          .byte 3          ; Constant 3
FOUR:           .byte 4          ; Constant 4
FIVE:           .byte 5          ; Constant 5 (also tile value)
TWENTY:         .byte 20         ; For tile address calculation
MAX_X:          .byte 19         ; Max X coordinate (0 to 19)
MAX_Y:          .byte 14         ; Max Y coordinate (0 to 14)

; Variables
DIR:            .byte 0          ; Direction (0: none, 1: up, 2: right, 3: down, 4: left)
TEMP_X:         .byte 0          ; Temporary X for collision check
TEMP_Y:         .byte 0          ; Temporary Y for collision check
NEW_X:          .byte 0          ; New head X position
NEW_Y:          .byte 0          ; New head Y position
CURRENT_X:      .byte 0          ; Current head X
CURRENT_Y:      .byte 0          ; Current head Y
COLLISION_FLAG: .byte 0          ; 1 if collision detected

; Snake positions (circular buffer, length 5)
SNAKE_X0:       .byte 2          ; Initial snake X positions: 2,3,4,5,6
SNAKE_Y0:       .byte 0
SNAKE_X1:       .byte 3
SNAKE_Y1:       .byte 0
SNAKE_X2:       .byte 4
SNAKE_Y2:       .byte 0
SNAKE_X3:       .byte 5
SNAKE_Y3:       .byte 0
SNAKE_X4:       .byte 6
SNAKE_Y4:       .byte 0

HEAD_IDX:       .byte 4          ; Index of head (0 to 4)
TAIL_IDX:       .byte 0          ; Index of tail (0 to 4)
