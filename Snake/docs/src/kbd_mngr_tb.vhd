-- library declaration
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;            -- basic IEEE library
use IEEE.NUMERIC_STD.ALL;               -- IEEE library for the unsigned type
                                        -- and various arithmetic operations

-- Entity declaration for the testbench - no ports needed
entity kbd_mngr_tb is
end entity;

-- Architecture of the testbench
architecture func of kbd_mngr_tb is
    
    -- Component declaration for the keyboard manager
    component KBD_MNGR is
        port (
            clk      : in std_logic;   -- system clock (100 MHz)
            rst_kbd_mngr      : in std_logic;   -- reset signal
            ScanCode : in std_logic_vector(7 downto 0);   -- scancode byte
            make_op  : in std_logic;                      -- one-pulsed scancode-enable
            player0  : out std_logic_vector(7 downto 0); 
            player1  : out std_logic_vector(7 downto 0); 
            we       : out std_logic);                       -- write enable
    end component;

    -- Input signals
    signal clk      : std_logic := '0';
    signal rst      : std_logic := '0';
    signal ScanCode : std_logic_vector(7 downto 0) := (others => '0');
    signal make_op  : std_logic := '0';
    
    -- Output signals
    signal player0  : std_logic_vector(7 downto 0);
    signal player1  : std_logic_vector(7 downto 0);
    signal we       : std_logic;
    
    -- Clock period definition
    constant clk_period : time := 10 ns;
    
    -- Scancode definitions
    -- Player 0 keys
    constant KEY_W : std_logic_vector(7 downto 0) := x"1D";  -- W
    constant KEY_A : std_logic_vector(7 downto 0) := x"1C";  -- A
    constant KEY_S : std_logic_vector(7 downto 0) := x"1B";  -- S
    constant KEY_D : std_logic_vector(7 downto 0) := x"23";  -- D
    
    -- Player 1 keys
    constant KEY_I : std_logic_vector(7 downto 0) := x"43";  -- I
    constant KEY_J : std_logic_vector(7 downto 0) := x"3B";  -- J
    constant KEY_K : std_logic_vector(7 downto 0) := x"42";  -- K
    constant KEY_L : std_logic_vector(7 downto 0) := x"4B";  -- L
    
    -- Break code
    constant BREAK_CODE : std_logic_vector(7 downto 0) := x"F0";

begin

    -- Instantiate the Unit Under Test (UUT)
    -- Using direct instantiation instead of component instantiation to avoid library issues
    uut: entity work.KBD_MNGR
    port map(
        clk      => clk,
        rst_kbd_mngr      => rst,
        ScanCode => ScanCode,
        make_op  => make_op,
        player0  => player0,
        player1  => player1,
        we       => we
    );
        -- Clock process
    clk_process: process
    begin
        clk <= '0';
        wait for clk_period/2;
        clk <= '1';
        wait for clk_period/2;
    end process;

    -- Stimulus process
    stim_proc: process
    begin
        -- Reset the system
        rst <= '1';
        wait for clk_period*5;
        rst <= '0';
        wait for clk_period*5;
        
        -- Test Player 0 Keys
        -- Test KEY_W (Player 0)
        ScanCode <= KEY_W;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period*5;
        
        -- Test KEY_A (Player 0)
        ScanCode <= KEY_A;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period*5;
        
        -- Test KEY_S (Player 0)
        ScanCode <= KEY_S;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period*5;
        
        -- Test KEY_D (Player 0)
        ScanCode <= KEY_D;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period*5;
        
        -- Test Player 1 Keys
        -- Test KEY_I (Player 1)
        ScanCode <= KEY_I;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period*5;
        
        -- Test KEY_J (Player 1)
        ScanCode <= KEY_J;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period*5;
        
        -- Test KEY_K (Player 1)
        ScanCode <= KEY_K;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period*5;
        
        -- Test KEY_L (Player 1)
        ScanCode <= KEY_L;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period*5;
        
        -- Test mixed key sequence
        -- Player 0 presses W, then Player 1 presses I
        ScanCode <= KEY_W;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period*3;
        
        ScanCode <= KEY_I;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period*5;
        
        -- Test break codes (simulating key release)
        -- Player 0 releases W
        ScanCode <= BREAK_CODE;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period;
        
        ScanCode <= KEY_W;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period*3;
        
        -- Player 1 releases I
        ScanCode <= BREAK_CODE;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period;
        
        ScanCode <= KEY_I;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period*5;
        
        -- Test non-mapped keys (should have no effect)
        ScanCode <= x"2B";  -- F key
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period*5;
        
        -- Test rapid consecutive key presses
        -- Player 0 presses multiple keys rapidly
        ScanCode <= KEY_W;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period;
        
        ScanCode <= KEY_A;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period;
        
        ScanCode <= KEY_S;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period;
        
        ScanCode <= KEY_D;
        make_op <= '1';
        wait for clk_period;
        make_op <= '0';
        wait for clk_period*5;
        
        -- Reset in middle of operation
        ScanCode <= KEY_W;
        make_op <= '1';
        wait for clk_period/2;
        rst <= '1';
        wait for clk_period;
        rst <= '0';
        make_op <= '0';
        wait for clk_period*5;
        
        -- Finish test
        wait;
    end process;
end;