

; ---------- CODE ----------
        LD      GR3, POS           ; GR3 = current tile pointer
        LD      GR1, FIVE
        STORE   GR1, [0]           ; draw first pixel

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
        BRA     skip_key           ; ignore other keys

set_up:                            ; DIR = 1 (move up)
        LDI     GR1, #1
        STORE   GR1, DIR
        BRA     skip_key

set_left:                          ; DIR = 2 (move left)
        LDI     GR1, #2
        STORE   GR1, DIR
        BRA     skip_key

set_down:                          ; DIR = 3 (move down)
        LDI     GR1, #3
        STORE   GR1, DIR
        BRA     skip_key

set_right:                         ; DIR = 4 (move right)
        LDI     GR1, #4
        STORE   GR1, DIR
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

; 3 - erase old tile --------------------------------------------------
        LDI     GR1, #0
        STORE   GR1, [0]

; 4 - move ------------------------------------------------------------
        LD      GR2, DIR
        CMP     GR2, ZERO
        BEQ     draw_tile
        CMP     GR2, ONE
        BEQ     try_up          ; DIR == 1, go up
        CMP     GR2, TWO
        BEQ     try_left        ; DIR == 2, go left
        CMP     GR2, THREE
        BEQ     try_down        ; DIR == 3, go down
        CMP     GR2, FOUR
        BEQ     try_right       ; DIR == 4, go right


; move up
try_up:
        LD      GR2, Y_POS
        CMP     GR2, ZERO
        BEQ     draw_tile
        SUB     GR2, ONE
        STORE   GR2, Y_POS
        BRA     draw_tile

; move left
try_left:
        LD      GR2, X_POS
        CMP     GR2, ZERO
        BEQ     draw_tile          ; at left wall
        SUB     GR2, ONE           ; GR3 = GR3 - 1
        STORE   GR2, X_POS
        BRA     draw_tile

; move down
try_down:
        LD      GR2, Y_POS
        CMP     GR2, MAX_Y
        BEQ     draw_tile          ; at bottom wall
        ADD     GR2, ONE           
        STORE   GR2, Y_POS
        BRA     draw_tile

; move right
try_right:
        LD      GR2, X_POS
        CMP     GR2, MAX_X
        BEQ     draw_tile          ; at right wall
        ADD     GR2, ONE           ; GR3 = GR3 + 1
        STORE   GR2, X_POS

; 5 - draw tile and loop ---------------------------------------------
draw_tile:
        LD      GR1, FIVE
        LD      GR3, LOW_BOUND
        LD      GR2, ZERO
        ADD     GR3, X_POS

Y_draw_lp:
        CMP     GR2, Y_POS
        BEQ     draw_store
        ADD     GR3, TWENTY
        ADD     GR2, ONE
        BRA     Y_draw_lp
        

draw_store:
        STORE   GR1, [0]
        BRA     main_loop

; ---------- VARIABLES ----------
POS         :   .byte   29954      ; initial tile address
X_POS       :   .byte   2          ; initial position X
Y_POS       :   .byte   0          ; initial position Y
DIR         :   .byte   0          ; 0 = still, 1 = up, 2 = left, 3 = down, 4 = right
KEY_PREV    :   .byte   0          ; last seen scan-code

; ---------- CONSTANTS ----------
KEY_A       :   .byte   0x1C
KEY_D       :   .byte   0x23
KEY_W       :   .byte   0x1D
KEY_S       :   .byte   0x1B
LOW_BOUND   :   .byte   29952      ; first visible tile
HIGH_BOUND  :   .byte   30251      ; last visible tile
DELAY_MAX   :   .byte   65000     ; bigger -> slower
DELAY_DIV_MAX:  .byte   3
MAX_X       :   .byte   19
MAX_Y       :   .byte   14
ZERO        :   .byte   0
ONE         :   .byte   1
TWO         :   .byte   2
THREE       :   .byte   3
FOUR        :   .byte   4
FIVE        :   .byte   5          ; colour/value to plot
TWENTY      :   .byte   20
