LIBRARY ieee;
USE ieee.std_logic_1164.ALL;
USE IEEE.NUMERIC_STD.ALL;

ENTITY uprogCPU_tb IS
END uprogCPU_tb;

ARCHITECTURE func OF uprogCPU_tb IS

  --Component Declaration for the Unit Under Test (UUT)
  COMPONENT uprogCPU
  PORT(clk : IN std_logic;
       btnC : IN std_logic;
       PS2Clk   : in std_logic;                         -- PS2 clock
       PS2Data  : in std_logic);      
        -- reset, active high
  END COMPONENT;

  --Inputs
  signal clk : std_logic:= '0';
  signal rst : std_logic:= '0';
	signal PS2KeyboardCLK : std_logic;
	signal PS2KeyboardData : std_logic;

  --Clock period definitions
  constant clk_period : time:= 10 ns; -- 100 MHz

BEGIN
  -- Instantiate the Unit Under Test (UUT)
  uut: uprogCPU PORT MAP (
    clk => clk,
    btnC => rst,
    PS2Clk => PS2KeyboardCLK,
		PS2Data => PS2KeyboardData
  );
		
  -- Clock process definitions
  clk_process :process
  begin
    clk <= '0';
    wait for clk_period/2;
    clk <= '1';
    wait for clk_period/2;
  end process;

	rst <= '1', '0' after 17 ns;
END;

