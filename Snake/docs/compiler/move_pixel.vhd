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
-- 0x0E   | BUA    | Branch if UART has data
-- 0x0F   | KEY1   | Read key input from player 1

(
   x"1C3A",
   x"1449",
   x"2700",
   x"C000",
   x"703C",
   x"9100",
   x"0018",
   x"203C",
   x"703F",
   x"9100",
   x"0010",
   x"7040",
   x"9100",
   x"0015",
   x"8100",
   x"0018",
   x"0400",
   x"243B",
   x"D03E",
   x"8100",
   x"0018",
   x"0401",
   x"243B",
   x"D000",
   x"1846",
   x"1445",
   x"4447",
   x"7448",
   x"9100",
   x"0020",
   x"8100",
   x"001A",
   x"4847",
   x"7848",
   x"9100",
   x"0026",
   x"8100",
   x"0019",
   x"0400",
   x"2700",
   x"183B",
   x"7847",
   x"9100",
   x"0032",
   x"7C43",
   x"9100",
   x"0036",
   x"4C47",
   x"8100",
   x"0036",
   x"7C44",
   x"9100",
   x"0036",
   x"3C47",
   x"1449",
   x"2700",
   x"8100",
   x"0003",
   x"7502",
   x"0001",
   x"0000",
   x"00C8",
   x"0320",
   x"001C",
   x"0023",
   x"001D",
   x"001B",
   x"7500",
   x"762B",
   x"FDE8",
   x"0003",
   x"0001",
   x"0000",
   x"0005",
   x"FFFF"
);