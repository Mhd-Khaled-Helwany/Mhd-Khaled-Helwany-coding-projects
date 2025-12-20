-- Instruction Set Reference:
-- Opcode | Mnemonic | Description
-- ----------------------------
-- 0x00   | LDI    | Load immediate value into register
-- 0x01   | LD     | Load from memory into register
-- 0x02   | STORE  | Store register to memory
-- 0x03   | ADD    | Add memory to register
-- 0x04   | SUB    | Subtract memory from register
-- 0x05   | OR     | Bitwise OR with memory
-- 0x06   | LSR    | Logical shift right register
-- 0x07   | CMP    | Compare register with memory
-- 0x08   | BRA    | Branch always to address
-- 0x09   | BEQ    | Branch if Z=1
-- 0x0A   | AND    | Bitwise AND memory with register
-- 0x0B   | BPL    | Branch if N=0
-- 0x0C   | KEY0   | Read key input from player 0
-- 0x0D   | PLAY   | Play piezo sound
-- 0x0E   | UART   | Read/write UART data
-- 0x0F   | KEY1   | Read key input from player 1

(
   x"1004",
   x"0405",
   x"2700",
   x"1F00",
   x"07D0"
);