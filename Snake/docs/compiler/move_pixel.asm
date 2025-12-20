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

        CMP     GR0, KEY_A
        BEQ     set_left
        CMP     GR0, KEY_D
        BEQ     set_right
        BRA     skip_key           ; ignore other keys

set_left:                          ; DIR = 0 (move left)
        LDI     GR1, #0
        STORE   GR1, DIR
        PLAY    tone0;
        BRA     skip_key

set_right:                         ; DIR = 1 (move right)
        LDI     GR1, #1
        STORE   GR1, DIR
        PLAY    tone1;

        ; fall-through

skip_key:
; 2 - delay loop ------------------------------------------------------
        LD GR2, DELAY_DIV_MAX
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
        CMP     GR2, ONE
        BEQ     try_right          ; DIR == 1, go right

; move left
try_left:
        CMP     GR3, LOW_BOUND
        BEQ     draw_tile          ; at left wall
        SUB     GR3, ONE           ; GR3 = GR3 - 1
        BRA     draw_tile

; move right
try_right:
        CMP     GR3, HIGH_BOUND
        BEQ     draw_tile          ; at right wall
        ADD     GR3, ONE           ; GR3 = GR3 + 1

; 5 - draw tile and loop ---------------------------------------------
draw_tile:
        LD      GR1, FIVE
        STORE   GR1, [0]
        BRA     main_loop
; ---------- VARIABLES ----------
POS         :   .byte   29954      ; initial tile address
DIR         :   .byte   1          ; 1 = right, 0 = left
KEY_PREV    :   .byte   0          ; last seen scan-code

; ---------- CONSTANTS ----------
tone0       :   .byte   200
tone0       :   .byte   800

KEY_A       :   .byte   0x1C
KEY_D       :   .byte   0x23
KEY_W       :   .byte   0x1D
KEY_S       :   .byte   0x1B
LOW_BOUND   :   .byte   29952      ; first visible tile
HIGH_BOUND  :   .byte   30251      ; last visible tile
DELAY_MAX   :   .byte   65000     ; bigger -> slower
DELAY_DIV_MAX: .byte 3
ONE         :   .byte   1
ZERO        :   .byte   0
FIVE        :   .byte   5          ; colour/value to plot
