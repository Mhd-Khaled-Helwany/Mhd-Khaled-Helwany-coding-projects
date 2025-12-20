--------------------------------------------------------------------------------
-- KEYBOARD_MANAGER
-- Version 1.0: See KBD_ENC.
-- Description:
-- * This reads scancodes, and writes to the appropriate player key registers

-- library declaration
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;            -- basic IEEE library
use IEEE.NUMERIC_STD.ALL;               -- IEEE library for the unsigned type
                                        -- and various arithmetic operations
-- entity
entity KBD_MNGR is
	port (
		clk      : in std_logic;   -- system clock (100 MHz)
		rst_kbd_mngr      : in std_logic;   -- reset signal
		ScanCode : in std_logic_vector(7 downto 0);   -- scancode byte
		make_op  : in std_logic;                      -- one-pulsed scancode-enable
		player0  : out std_logic_vector(7 downto 0); 
		player1  : out std_logic_vector(7 downto 0); 
		we       : out std_logic);                    -- write enable
end KBD_MNGR;

architecture behavioral of KBD_MNGR is
	
	type wr_type is (STANDBY, WRINDEX, WRCUR); -- declare state types for write cycle
	signal Wrst_kbd_mngrate : wr_type;                  -- write cycle state
	signal player0_int : std_logic_vector(7 downto 0); -- Internal signal for player0
    signal player1_int : std_logic_vector(7 downto 0); -- Internal signal for player1

begin


    -- Assign internal signals to the outputs
    player0 <= player0_int;
    player1 <= player1_int;

    -- Update player0 and player1 based on scancode and make_op
    -- This section mimics the behavior of the previous implementation and keeps it outside the clocked process
    process(clk)
    begin
        if rising_edge(clk) then
            if rst_kbd_mngr = '1' then
                player0_int <= (others => '0');
                player1_int <= (others => '0');
            elsif make_op = '1' then
                -- Player 0 scancode condition
                if (ScanCode = x"1D" or ScanCode = x"1C" or ScanCode = x"1B" or ScanCode = x"23") then
                    player0_int <= ScanCode;
                end if;
                -- Player 1 scancode condition
                if (ScanCode = x"43" or ScanCode = x"3B" or ScanCode = x"42" or ScanCode = x"4B") then
                    player1_int <= ScanCode;
                end if;
            end if;
        end if;
    end process;
	
 
	-- write state
	process(clk)
	begin
		if rising_edge(clk) then
			if rst_kbd_mngr='1' then
				Wrst_kbd_mngrate <= STANDBY;
			else
				case Wrst_kbd_mngrate is
					when STANDBY =>
						if make_op = '1' then
							Wrst_kbd_mngrate <= WRINDEX;
						else
							Wrst_kbd_mngrate <= STANDBY;
						end if;
					when WRINDEX =>
						Wrst_kbd_mngrate <= WRCUR;
					when WRCUR =>
						Wrst_kbd_mngrate <= STANDBY;
					when others =>
						Wrst_kbd_mngrate <= STANDBY;
				end case;
			end if;
		end if;
	end process;
	
	
	-- we will be enabled ('1') for two consecutive clock cycles during WRINDEX and WRCUR states
	-- and disabled ('0') otherwise at STANDBY state
	we <= '0' when (Wrst_kbd_mngrate = STANDBY) else '1';
	
end behavioral;
			
	