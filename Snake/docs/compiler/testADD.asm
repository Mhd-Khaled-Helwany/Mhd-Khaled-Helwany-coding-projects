SNAKE_ONE_CLEAR:                   ; Resets snake index to 3
        LDI     GR1, 3                 
        STORE   GR1, SNAKE_ONE_HEAD_INDEX
        LD     GR1, ONE                
        STORE   GR1, SNAKE_ONE_TAIL_INDEX
        LDI     GR1, 0
        LD      GR3, SNAKE_ONE_START
        LDI     GR1, 4
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
BRA SNAKE_ONE_CLEAR




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