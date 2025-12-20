library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

entity K1 is
    port(
        K1Addr : in unsigned(3 downto 0);
        K1Data : out unsigned(6 downto 0)
    );
end K1;
-- ============================================================================
-- Opcode to Microcode Start Address Mapping for K1 Component
-- ============================================================================
-- This table links each instruction opcode to the starting microcode address
-- in the control store (uMem). Use this mapping to implement the K1 decoder.
--  Opcode | Mnemonic | Description                      | Microcode Start Addr
-- --------------------------------------------------------------------------
--   0x00  | LDI      | Load immediate                  | 0x0A
--   0x01  | LD       | Load from memory                | 0x0C
--   0x02  | STORE    | Store register to memory        | 0x0E
--   0x03  | ADD      | Add memory to register          | 0x0F
--   0x04  | SUB      | Subtract memory from register   | 0x12
--   0x05  | OR       | Bitwise OR with memory          | 0x22
--   0x06  | LSR      | Logical shift right             | 0x18
--   0x07  | CMP      | Compare register with memory    | 0x1F
--   0x08  | BRA      | Branch always                   | 0x1A
--   0x09  | BEQ      | Branch if zero flag set         | 0x1D
--   0x0A  | ADD      | Move register to register       | 0x15
--   0x0B  | BPL      | Branch if positive (N=0)        | 0x27
--   0x0C  | KEY0     | Read key input from player 0    | 0x2A
--   0x0D  | PLAY     | Play piezo sound                | 0x2C
--   0x0E  | UART     | UART read/write                 | 0x2D
--   0x0F  | KEY1     | Read key input from player 1    | 0x2B
architecture func of K1 is
    type K1_mem_t is array (0 to 15) of unsigned(6 downto 0);
    constant K1_mem_c : K1_mem_t :=
    ( 
    "0001010",  -- 0x0: LDI      (Load immediate)             -> uMem addr 0x0A
    "0001100",  -- 0x1: LD       (Load from memory)           -> uMem addr 0x0C
    "0001110",  -- 0x2: STORE    (Store register to memory)   -> uMem addr 0x0E
    "0001111",  -- 0x3: ADD      (Add memory to register)     -> uMem addr 0x0F
    "0010010",  -- 0x4: SUB      (Subtract memory from reg)   -> uMem addr 0x12
    "0100011",  -- 0x5: OR       (Bitwise OR with memory)     -> uMem addr 0x22
    "0011000",  -- 0x6: LSR      (Logical shift right)        -> uMem addr 0x18
    "0100000",  -- 0x7: CMP      (Compare register with mem)  -> uMem addr 0x1F
    "0011011",  -- 0x8: BRA      (Branch always)              -> uMem addr 0x1A
    "0011110",  -- 0x9: BEQ      (Branch if zero flag set)    -> uMem addr 0x1D
    "0010101",  -- 0xA: AND      (And instruction)            -> uMem addr 0x15
    "0101000",  -- 0xB: BPL      (Branch if positive)         -> uMem addr 0x27
    "0101010",  -- 0xC: KEY0     (Read key input player 0)    -> uMem addr 0x2A
    "0101100",  -- 0xD: PLAY     (Play piezo sound)           -> uMem addr 0x2C
    "0101101",  -- 0xE: UART     (UART read/write)            -> uMem addr 0x2D
    "0101011"   -- 0xF: KEY1     (Read key input player 1)    -> uMem addr 0x2B  
    );

  --creates k1 mem of type K1 mem t with default value k1 mem c
  signal K1_mem : K1_mem_t := K1_mem_c;

begin
    k1Data <= K1_mem(to_integer(k1Addr));

end func;