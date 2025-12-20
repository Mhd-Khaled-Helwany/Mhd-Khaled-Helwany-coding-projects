


SNAKE_ONE_CLEAR:                   ; Resets snake index to 3
        LDI     GR1, 3                 
        STORE   GR1, SNAKE_ONE_HEAD_INDEX
        LD     GR1, ONE                
        STORE   GR1, SNAKE_ONE_TAIL_INDEX
        LDI     GR1, 0
        LD      GR3, SNAKE_ONE_START
        LDI     GR1, 0
        STORE   GR1, SNAKE_ONE_DIR
SNAKE_ONE_CLEAR_LP:                ; Sets all positions as empty for snake one
        STORE GR1, [0]
        ADD GR3, ONE
        CMP GR3, SNAKE_ONE_END
        BEQ SNAKE_ONE_DEFAULT_DIRECTIONS
        BRA SNAKE_ONE_CLEAR_LP
SNAKE_ONE_DEFAULT_DIRECTIONS: ; Resets snake one to its default position
        LD      GR3, SNAKE_ONE_START
        LDI GR1, 4
        STORE   GR1, SNAKE_ONE_DIR
        ADD GR3, ONE
        STORE GR1, [0]
        ADD GR3, ONE
        STORE GR1, [0]
        ADD GR3, ONE
        STORE GR1, [0]
        LDI GR1, 3
        STORE GR1, SNAKE_ONE_X
        LDI GR1, 0
        STORE GR1, SNAKE_ONE_Y
        LDI GR1, 1
        STORE GR1, SNAKE_ONE_TAIL_X
        LDI GR1, 0
        STORE GR1, SNAKE_ONE_TAIL_Y
        
main_loop:
; 1 - keyboard polling -----------------------------------------------
        KEY0    GR0                ; GR0 holds latest scan-code
        CMP     GR0, KEY_PREV
        BEQ     skip_key


        

        STORE   GR0, KEY_PREV

        CMP     GR0, KEY_W
        BEQ     set_up
        CMP     GR0, KEY_A
        BEQ     set_left
        CMP     GR0, KEY_S
        BEQ     set_down
        CMP     GR0, KEY_D
        BEQ     set_right
        BRA     SNAKE_ONE_CLEAR           ; ignore other keys

set_up:                            ; SNAKE_ONE_DIR = 1 (move up)
        LD      GR1, THREE
        CMP     GR1, SNAKE_ONE_DIR
        BEQ     skip_key
        LDI     GR1, 1
        STORE   GR1, SNAKE_ONE_DIR
        BRA     skip_key

set_left:                          ; SNAKE_ONE_DIR = 2 (move left)
        LD      GR1, FOUR
        CMP     GR1, SNAKE_ONE_DIR
        BEQ     skip_key
        LDI     GR1, 2
        STORE   GR1, SNAKE_ONE_DIR
        BRA     skip_key

set_down:                          ; SNAKE_ONE_DIR = 3 (move down)
        LD      GR1, ONE
        CMP     GR1, SNAKE_ONE_DIR
        BEQ     skip_key
        LDI     GR1, 3
        STORE   GR1, SNAKE_ONE_DIR
        BRA     skip_key

set_right:                         ; SNAKE_ONE_DIR = 4 (move right)
        LD      GR1, TWO
        CMP     GR1, SNAKE_ONE_DIR
        BEQ     skip_key
        LDI     GR1, 4
        STORE   GR1, SNAKE_ONE_DIR
        ; fall-through

skip_key:
; 2 - delay loop ------------------------------------------------------
        LD      GR2, DELAY_DIV_MAX
start_lp:
        LD      GR1, DELAY_MAX
delay_lp:
        SUB     GR1, ONE
        CMP     GR1, ZERO
        BEQ     end_delay_lp
        BRA     delay_lp
end_delay_lp:
        SUB     GR2, ONE
        CMP     GR2, ZERO
        BEQ     end_delay
        BRA     start_lp
end_delay: 

; this whole tail block should be skipped if snake eat apple

update_tail_start:
        ; DRAW_TILE(TILE=0 (BACKGROUND), SNAKE_ONE_TAIL_X, SNAKE_ONE_TAIL_Y)
        LD GR0, ZERO
        LD GR1, SNAKE_ONE_TAIL_X
        LD GR2, SNAKE_ONE_TAIL_Y
        BRA draw_tile
update_tail_position: ; Checks what direction tail was moving in and then calculate its next position
        LD      GR3, SNAKE_ONE_START
        ADD     GR3, SNAKE_ONE_TAIL_INDEX
        LD      GR1, [0] ;finds direction of tail
        CMP     GR1, ONE
        BEQ     update_tail_position_up
        CMP     GR1, TWO
        BEQ     update_tail_position_left
        CMP     GR1, THREE
        BEQ     update_tail_position_down
        CMP     GR1, FOUR
        BEQ     update_tail_position_right
        BRA     SNAKE_ONE_CLEAR

update_tail_position_up:
        LD GR0, SNAKE_ONE_TAIL_Y
        SUB GR0, ONE
        STORE GR0, SNAKE_ONE_TAIL_Y
        BRA     update_tail_position_index

update_tail_position_left:
        LD GR0, SNAKE_ONE_TAIL_X
        SUB GR0, ONE
        STORE GR0, SNAKE_ONE_TAIL_X 
        BRA     update_tail_position_index

update_tail_position_down:
       LD GR0, SNAKE_ONE_TAIL_Y
        ADD GR0, ONE
        STORE GR0, SNAKE_ONE_TAIL_Y
        BRA     update_tail_position_index

update_tail_position_right:
        LD GR0, SNAKE_ONE_TAIL_X
        ADD GR0, ONE
        STORE GR0, SNAKE_ONE_TAIL_X 
        BRA     update_tail_position_index

update_tail_position_index: ;Updates tail index by 1 and if reaches max index it wraps to zero again
        LD GR0, SNAKE_ONE_TAIL_INDEX
        CMP GR0, MAX_INDEX
        BEQ wrap_tail_position_index
        ADD GR0, ONE
        STORE GR0, SNAKE_ONE_TAIL_INDEX
        BRA update_old_head
wrap_tail_position_index:
        LD GR0, ZERO
        STORE GR0, SNAKE_ONE_TAIL_INDEX

;draw neck here
update_old_head:
        ; DRAW_TILE(TILE=5 (BODY), SNAKE_ONE_X, SNAKE_ONE_Y)
        LD GR0, FIVE
        LD GR1, SNAKE_ONE_X
        LD GR2, SNAKE_ONE_Y
        BRA draw_tile
update_old_head_return: ;label that the draw tile function can return to

update_head_index:
        LD      GR2, SNAKE_ONE_DIR
        LD      GR3, SNAKE_ONE_START
        ADD     GR3, SNAKE_ONE_HEAD_INDEX
        STORE   GR2, [0]
        CMP     GR3, SNAKE_ONE_END
        BEQ     wrap_head_index
        LD      GR1, SNAKE_ONE_HEAD_INDEX
        ADD     GR1, ONE
        STORE   GR1, SNAKE_ONE_HEAD_INDEX
        BRA     try_directions
wrap_head_index:
        LD GR1, ZERO
        STORE GR1, SNAKE_ONE_HEAD_INDEX
try_directions:
        LD      GR3, SNAKE_ONE_START
        ADD     GR3, SNAKE_ONE_HEAD_INDEX
        ;STORE   GR2, [0]

        CMP     GR2, ZERO
        BEQ     SNAKE_ONE_CLEAR
        CMP     GR2, ONE
        BEQ     try_up          ; SNAKE_ONE_DIR == 1, go up
        CMP     GR2, TWO
        BEQ     try_left        ; SNAKE_ONE_DIR == 2, go left
        CMP     GR2, THREE
        BEQ     try_down        ; SNAKE_ONE_DIR == 3, go down
        CMP     GR2, FOUR
        BEQ     try_right       ; SNAKE_ONE_DIR == 4, go right


; move up
try_up:
    
        LD      GR2, SNAKE_ONE_Y
        CMP     GR2, ZERO
        BEQ     SNAKE_ONE_CLEAR


        SUB     GR2, ONE
        STORE   GR2, SNAKE_ONE_Y
        
        LD      GR0, ONE
        LD      GR1, SNAKE_ONE_X
        LD      GR2, SNAKE_ONE_Y
        BRA     draw_tile

; move left
try_left:
        LD      GR2, SNAKE_ONE_X
        CMP     GR2, ZERO
        BEQ     SNAKE_ONE_CLEAR          ; at left wall
        SUB     GR2, ONE           ; GR3 = GR3 - 1
        STORE   GR2, SNAKE_ONE_X
        LD      GR0, FOUR
        LD      GR1, SNAKE_ONE_X
        LD      GR2, SNAKE_ONE_Y
        BRA     draw_tile

; move down
try_down:
        LD      GR2, SNAKE_ONE_Y
        CMP     GR2, MAX_Y
        BEQ     SNAKE_ONE_CLEAR          ; at bottom wall
        ADD     GR2, ONE           
        STORE   GR2, SNAKE_ONE_Y
        LD      GR0, THREE
        LD      GR1, SNAKE_ONE_X
        LD      GR2, SNAKE_ONE_Y
        BRA     draw_tile

; move right
try_right:
        LD      GR2, SNAKE_ONE_X
        CMP     GR2, MAX_X
        BEQ     SNAKE_ONE_CLEAR          ; at right wall
        ADD     GR2, ONE           ; GR3 = GR3 + 1
        STORE   GR2, SNAKE_ONE_X
        LD      GR0, TWO
        LD      GR1, SNAKE_ONE_X
        LD      GR2, SNAKE_ONE_Y

; DRAW FUNCTION (TILE , X, Y)
; GR0 = TILE (0, 5 or different head directions)
; GR1 = X_POS
; GR2 = Y_POS
; check where to return based on what tile we are currently rendering
draw_tile:
        LD      GR3, LOW_BOUND
        STORE   GR1, TEMP
        ADD     GR3, TEMP
        STORE   GR2, TEMP
        LD      GR2, ZERO
Y_draw_lp:
        CMP     GR2, TEMP
        BEQ     draw_store
        ADD     GR3, TWENTY
        ADD     GR2, ONE
        BRA     Y_draw_lp
draw_store:
        STORE   GR0, [0]
        CMP     GR0, ZERO ; IF TILE = BACKGROUND, we return to update_tail_position
        BEQ     update_tail_position
        CMP     GR0, FIVE ; IF TILE = BODY, we return update_old_head_return (we have rendered the neck)
        BEQ     update_old_head_return
        BRA     main_loop ; we know we are drawing head here, return correctly

; ---------- VARIABLES ----------
POS                     :   .byte   29954      ; initial tile address
KEY_PREV                :   .byte   0          ; last seen scan-code
TEMP                    :   .byte   0
; ---------- CONSTANTS ----------
KEY_A                   :   .byte   0x1C
KEY_D                   :   .byte   0x23
KEY_W                   :   .byte   0x1D
KEY_S                   :   .byte   0x1B
LOW_BOUND               :   .byte   29952      ; first visible tile
HIGH_BOUND              :   .byte   30251      ; last visible tile
DELAY_MAX               :   .byte   65000      ; bigger -> slower
DELAY_DIV_MAX           :   .byte   5         ;
MAX_X                   :   .byte   19
MAX_Y                   :   .byte   14
ZERO                    :   .byte   0
ONE                     :   .byte   1
TWO                     :   .byte   2
THREE                   :   .byte   3
FOUR                    :   .byte   4
FIVE                    :   .byte   5          ; colour/value to plot
TWENTY                  :   .byte   20
MAX_INDEX               :   .byte 300


SNAKE_ONE_START         :   .byte 28000
SNAKE_ONE_END           :   .byte 28300
SNAKE_ONE_HEAD_INDEX    :   .byte 3
SNAKE_ONE_TAIL_INDEX    :   .byte 1
SNAKE_ONE_X             :   .byte 3
SNAKE_ONE_Y             :   .byte 0
SNAKE_ONE_DIR           :   .byte 0 ;  0 = still, 1 = up, 2 = left, 3 = down, 4 = right
SNAKE_ONE_TAIL_X        :   .byte 1
SNAKE_ONE_TAIL_Y        :   .byte 0


; 28000 - 28300 is snake ones space, this space stores cached directions circularly
; We save snake head and tail positions,
; If snake has eaten during cycle, dont remove tail
; When we move, we set the tile on tail position to 0, updates tail position coords using the cached directions
; We draw the current head (before updating the head position) with a body tile 
; We update the head position using current direction
; We then draw the new head at the current coords
