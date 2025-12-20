library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

-- uMem interface
entity uMem is
  port (
    uAddr : in unsigned(6 downto 0);
    uData : out unsigned(23 downto 0));
end uMem;

architecture Behavioral of uMem is
-- IR: OP-kod_GRx_M_ADR/CONST
--     0000_00_00_0000000
--     k1      k2 
-- 00: 2A 
-- 01: 2C
--
-- 
-- micro Memory

-- Kolla exakt vad det här är, vi har numera lagt sammma storlek som uData.
-- Dvs kolla u_mem_t(storlek)
type u_mem_t is array (0 to 45) of unsigned(23 downto 0);
constant u_mem_c : u_mem_t :=
  --ALU_TB_FB_PC_SEQ_uAddr
  --IR = 0010
  --ASR = 0011
  --PM = 1011
  --PC = 0100
  --GRx = 0101
  --uM = 0111
  --AR = 0110
  -- SR = 1001
  -- Player0 = 1000
  -- Player1 = 1110
  -- UART = 1010
  --VideoRam = 0001
  --

  (
  b"0000_0100_0011_0_0000_0000000", -- ASR:=PC        0x0
--  b"0000_0000_0000_0_0000_0000000", -- wait until PM gets result
  b"0000_1011_0010_1_0000_0000000", -- IR:=PM, PC:=PC+1 0x1
  
  b"0000_0000_0000_0_0010_0000000", -- uPC := k2 0x2
  
  b"0000_0010_0011_0_0001_0000000", -- ASR := IR, uPC := k1, Direktadressering 0x3

  b"0000_0100_0011_1_0001_0000000", -- ASR := PC, uPC := k1, PC++ , Immediate 0x4

  b"0000_0100_0011_1_0000_0000000", -- ASR := PC, PC ++  ; Absolut adressering 0x5
  b"0000_1011_0011_0_0001_0000000", -- ASR := PM, uPC := k1 

  b"0001_0010_0000_0_0000_0000000", -- AR:= IR  ; Indexerad adressering 0x7
  b"1000_0101_0000_0_0000_0000000", -- AR:= gr3+AR
  b"0000_0110_0011_0_0001_0000000", -- ASR := AR, uPC := k1

                                    -- Kom ihåg att implementera specifikt att de sista 8 bitarna
                                    -- i IR ska vara värdet och inte den adress som värdet ligger i
  b"0001_0010_0000_0_0000_0000000", -- LDI; AR := IR(ADR/CONST) 0xA 
  b"1111_0110_0101_0_0011_0000000", -- GRx := AR

  b"0001_1011_0000_0_0000_0000000", -- LD; AR := PM(A) 0xC
  b"0000_0110_0101_0_0011_0000000", -- GRx := AR 

  
  b"0000_0101_1011_0_0011_0000000", -- ST; PM(A) := GRx 0xE

  b"0001_0101_0000_0_0000_0000000", -- ADD; AR := GRx 0xF
  b"0100_1011_0000_0_0000_0000000", -- AR := AR + PM(A)
  b"0000_0110_0101_0_0011_0000000", -- GRx := AR
  
  b"0001_0101_0000_0_0000_0000000", -- SUB; AR := AR + GRx 0x12
  b"0101_1011_0101_0_0000_0000000", -- AR-PM(A) 
  b"0000_0110_0101_0_0011_0000000", -- GRx := AR
  
  b"0001_0101_0000_0_0000_0000000", -- AND; AR := GR(x) 0x15
  b"0110_1011_0101_0_0000_0000000", -- AR and PM(A)
  b"0011_0110_0101_0_0011_0000000", -- GR(x) := AR
  
  
  b"0001_0101_0000_0_0000_0000000", -- LSR; AR := GR(x) 0x18
  b"1101_0000_0000_0_0000_0000000", -- GR(x) skiftas logiskt höger
  b"0000_0110_0101_0_0011_0000000", -- GR(x) := AR
  
  b"0000_0000_0000_1_0000_0000000", --  BRA; 0, PC ++ 0x1B
  b"0001_1011_0000_0_0000_0000000", --  AR := PM(A)
  b"0011_0110_0100_0_0011_0000000", --  PC := AR
  
  
  b"0000_0000_0000_0_1000_0011011", -- BEQ; OM Z=1 0x1E Hoppar till BRA dvs adress 0x1B ovan 
  b"0000_0000_0000_0_0011_0000000", -- PC ++
  
  b"0001_0101_0000_0_0000_0000000", -- CMP; AR := GR(x) 0x20
  b"0101_1011_0000_0_0011_0000000", -- AR - PM(A)
  
  
  b"0000_0000_1111_0_0000_0000000", -- HALT; 0x22
  
  b"0001_0101_0000_0_0000_0000000", -- OR; AR := GR(x) 0x23
  b"0111_1011_0101_0_0000_0000000", -- AR OR PM(A)
  b"0011_0110_0101_0_0011_0000000", -- GR(x) := AR
  
                                    -- Kom ihåg att addressmod i IR är GR(y)
                                    -- GRx är GR(x)
  b"0001_0110_0000_0_0000_0000000", -- MOV; AR := GR(y) 0x26
  b"0000_0110_0101_0_0011_0000000", --  GR(x) := GR(y) 

  b"0000_0000_0000_0_1001_0011011", -- BPL; OM N=1 0x28 Hoppar till BRA dvs adress 0x1B ovan
  b"0000_0000_0000_1_0011_0000000", -- PC ++ 

  b"0000_1000_0101_0_0011_0000000", -- KEY0; GR(x) := KBD_Data 0x2A

  b"0000_1110_0101_0_0011_0000000", -- KEY1; GR(x) := KBD_Data 0x2B
  
  b"0000_1011_1111_0_0011_0000000", -- PLAY; Piezo := PM(ADR) 0x2C. Adress som innehåller ettor och nollor för ljud.
                                    -- Detta borde loopas

  b"0000_1010_0101_0_0011_0000000" -- UART; GR(x) := UART_DATA 0x2D. TB_URART = 1010 
  );
  
signal u_mem : u_mem_t := u_mem_c;

begin  -- Behavioral
  uData <= u_mem(to_integer(uAddr));

end Behavioral;
