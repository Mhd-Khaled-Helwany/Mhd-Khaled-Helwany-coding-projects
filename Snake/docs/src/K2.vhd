library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

entity K2 is
    port(
        k2Addr : in unsigned(1 downto 0);
		k2Data : out unsigned(6 downto 0)
    );
end K2;

architecture func of K2 is
    type K2_mem_t is array (0 to 3) of unsigned(6 downto 0);
    constant K2_mem_c : K2_mem_t :=
    ( 
        "0000011",-- 0x0: Direct addressing, microcode start addr 0x03
        "0000100",-- 0x1: Immediate addressing, microcode start addr 0x04
        "0000101", -- 0x2: Absolute addressing, microcode start addr 0x05
        "0000111"-- 0x3: Indexed addressing, microcode start addr 0x07
  );

  --creates K2 mem of type K2 mem t with default value K2 mem c
  signal K2_mem : K2_mem_t := K2_mem_c;

begin
    K2Data <= K2_mem(to_integer(K2Addr));

end func;