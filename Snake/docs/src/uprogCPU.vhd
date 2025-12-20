library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

--CPU interface
entity uprogCPU is
  port(
	clk: in std_logic;
	btnC : in std_logic;     -- reset button (middle of the five), active high
	Hsync    : out std_logic;                        -- horizontal sync
	Vsync    : out std_logic;                        -- vertical sync
	vgaRed   : out std_logic_vector(3 downto 0);     -- VGA red
	vgaGreen : out std_logic_vector(3 downto 0);     -- VGA green
	vgaBlue  : out std_logic_vector(3 downto 0);     -- VGA blue

	JB		 : out std_logic_vector(0 downto 0); -- peizo output

	PS2Clk  : in std_logic;                  -- PS2 clock
	PS2Data : in std_logic                 -- PS2 data

	);
end entity;

architecture func of uprogCPU is

	signal rst : std_logic;       -- synchronous clear

	component kbd_enc
	port (
		clk             : in std_logic;   -- system clock (100 MHz)
		rst_kbd_enc             : in std_logic;   -- reset signal
		PS2KeyboardClk  : in std_logic;   -- USB keyboard PS2 clock
		PS2KeyboardData : in std_logic;   -- USB keyboard PS2 data
		ScanCode        : out std_logic_vector(7 downto 0); -- scancode byte
		make_op         : out std_logic);                   -- one-pulsed scancode-enable
	end component;

	component kbd_mngr
	port (
		clk      : in std_logic;   -- system clock (100 MHz)
		rst_kbd_mngr      : in std_logic;   -- reset signal
		ScanCode : in std_logic_vector(7 downto 0);   -- scancode byte
		make_op  : in std_logic;                      -- one-pulsed scancode-enable
		player0  : out std_logic_vector(7 downto 0); 
		player1  : out std_logic_vector(7 downto 0); 
		we       : out std_logic);                     -- one-pulsed scancode-enable
	end component;

	-- micro Memory component
	component uMem
		port(
			uAddr : in unsigned(6 downto 0);
			uData : out unsigned(23 downto 0));
	end component;

	-- Unified Memory component
	component pMem
		port(
			clk       : in std_logic;
			vga_addr  : in unsigned(10 downto 0);
			vga_data_out : out std_logic_vector(7 downto 0);
			
			pAddr     : in unsigned(15 downto 0);
			pData_in  : in unsigned(15 downto 0);
			pData_out : out unsigned(15 downto 0);
			pFB       : in unsigned(3 downto 0)
		);
	end component;


	-- GRx
	component GRx
		port(
			clk : in std_logic;
			rst_grx : in std_logic;
			mux: in unsigned(1 downto 0);
			GrALU : unsigned(3 downto 0);
			FB : in unsigned(3 downto 0);
			TB_Grx : in unsigned(3 downto 0 );
			inData : in unsigned(15 downto 0);
			utData : out unsigned(15 downto 0)
			);
	end component;
	
	-- k1
	component K1
		port(
		K1Addr : in unsigned(3 downto 0);
		K1Data : out unsigned(6 downto 0));
	end component;

	-- k2
	component K2
		port( 
			K2Addr: in unsigned(1 downto 0);
			K2Data: out unsigned(6 downto 0)
			);
	end component;

	-- Piezo component
	component Piezo
		port(
			clk : in std_logic;
			rst_piezo : in std_logic;
			piezoData : in unsigned(15 downto 0);
			piezoOut : out std_logic
			);
	end component;

	component vga_motor
		port (
			clk      : in std_logic;
			VR_data  : in std_logic_vector(7 downto 0);
			VR_addr  : out unsigned(10 downto 0);
			clear    : in std_logic;
			vgaRed   : out std_logic_vector(3 downto 0);
			vgaGreen : out std_logic_vector(3 downto 0);
			vgaBlue  : out std_logic_vector(3 downto 0);
			Hsync    : out std_logic;
			Vsync    : out std_logic
			);
	end component;

	-- micro memory signals
	-- ALU_TB_FB_PC_SEQ_uAddr
	-- 4-4-4-1-4-7
	-- 23-20_19-16_15-12_11_10-7_6-0

	signal uM : unsigned(23 downto 0); -- micro Memory output
	alias ALU : unsigned(3 downto 0) is uM(23 downto 20);
	alias TB : unsigned(3 downto 0) is uM(19 downto 16);
	alias FB : unsigned(3 downto 0) is uM(15 downto 12);
	alias PCsig : std_logic is uM(11);  -- (0:PC=PC, 1:PC++)
	alias SEQ : unsigned(3 downto 0) is uM(10 downto 7);
	alias uAddr : unsigned(6 downto 0) is uM(6 downto 0);

	-- program memory signals
	signal PM : unsigned(15 downto 0); -- Program Memory output
	signal DM : unsigned(15 downto 0); -- DATA Memory output

	
	-- local registers
	signal uPC : unsigned(6 downto 0); -- micro Program Counter
	signal PC : unsigned(15 downto 0); -- Program Counter
	signal IR : unsigned(15 downto 0); -- Instruction Register
	alias opkod : unsigned(3 downto 0) is IR(15 downto 12);
	alias grxkod : unsigned(1 downto 0) is IR(11 downto 10);
	alias mKod : unsigned(1 downto 0) is IR(9 downto 8);
	alias addrC : unsigned(7 downto 0) is IR(7 downto 0);

	signal ASR_reg : unsigned(15 downto 0); -- Address Register
	signal ASR : unsigned(15 downto 0); -- Address Register
	
	

	signal AR : signed(15 downto 0); -- AR Register
	signal SR : signed(15 downto 0); -- SR Register

	-- Status registers
	signal Z : std_logic;
	signal N : std_logic;
	-- For ALU to check if a flag is going to be updated
	signal check_flag : std_logic;


	-- local combinatorials
	signal DATA_BUS : unsigned(23 downto 0); -- Data Bus
	signal ScanCode : std_logic_vector(7 downto 0);
	signal make_op : std_logic;
	signal we_s   : std_logic;                    -- write enable

	-- Unified memory signals
	signal pMem_data_out : unsigned(15 downto 0);
	signal vga_data : std_logic_vector(7 downto 0);
	signal vga_addr : unsigned(10 downto 0);


	--local keyRegisters
	signal player0 : std_logic_vector(7 downto 0);
	signal player1 : std_logic_vector(7 downto 0);

	--piezo signals
	signal piezo_data : unsigned(15 downto 0) := (others => '0');
	signal piezo_out : std_logic := '0';

	--K1 and K2 handlers
	signal opcode : unsigned(3 downto 0);
	signal addr_mode : unsigned(1 downto 0);
	signal k1_out    : unsigned(6 downto 0);
	signal k2_out    : unsigned(6 downto 0);

	signal grx_inData : unsigned(15 downto 0);
	signal grx_utData : unsigned(15 downto 0);


begin
	opcode <= opkod;
	addr_mode <= mKod;
	process(clk)
	begin
		if rising_edge(clk) then
			rst <= btnC; -- syncronize the reset signal
		end if;
	end process;

	-- mPC : micro Program Counter
	process(clk)
	begin
		if rising_edge(clk) then
			if (rst = '1') then
				uPC <= (others => '0');

			elsif (SEQ = "0000") then
				uPC <= uPc + 1;
			
			elsif (SEQ = "0001") then
				uPC <= k1_out;
			
			elsif (SEQ = "0010") then
				uPC <= k2_out;
			
			elsif (SEQ = "0011") then
				uPC <= (others => '0');

			elsif (SEQ = "0101") then
				uPC <= uAddr;
			
			elsif (SEQ = "1000") then
				if Z = '1' then
					uPC <= uAddr;
				else
				uPC <= uPc + 1;

				end if;
			elsif (SEQ = "1001") then
				if N = '0' then
					uPC <= uAddr;
					else
					uPC <= uPc + 1;

				end if;

			elsif (SEQ = "1111") then
				-- Halt
			end if;
		end if;
	end process;

	-- PC : Program Counter
	process(clk)
	begin
		if rising_edge(clk) then
			if (rst = '1') then
				PC <= (others => '0');

			elsif (FB = "0100") then
				PC <= DATA_BUS(15 downto 0);

				
			elsif (PCsig = '1') then
				PC <= PC + 1;
			end if;
		end if;
	end process;

	-- IR : Instruction Register
	process(clk)
	begin
		if rising_edge(clk) then
			if (rst = '1') then
				IR <= (others => '0');
			elsif (FB = "0010") then
				IR <= DATA_BUS(15 downto 0);
			end if;
		end if;
	end process;
	
	
	-- ASR : Address Register
	process(clk)
	begin
		if rising_edge(clk) then
			if (rst = '1') then
				ASR_reg <= (others => '0');
			elsif (FB = "0011") then
				ASR_reg <= DATA_BUS(15 downto 0);
			end if;
		end if;
	end process;
	ASR <= ASR_reg when FB /= "0011" else
		DATA_BUS(15 downto 0); -- register bypass

	-- Piezo
	process(clk)
	begin
		if rising_edge(clk) then
			if (rst = '1') then
				piezo_data <= (others => '0');
			elsif (FB = "1111") then
				piezo_data <= DATA_BUS(15 downto 0);
			end if;
		end if;
	end process;
	-- ALU : Arithmetic logic unic
	process(clk)
	begin
		if rising_edge(clk) then
			if (rst = '1') then
				AR <= (others => '0');
				check_flag <= '0';
				Z <= '0';
				N <= '0';
			elsif (ALU = "0000") then
				-- nop
			elsif (ALU = "0001") then
				AR <= signed(DATA_BUS(15 downto 0));
			
			elsif (ALU = "0010") then
				AR <= signed(NOT DATA_BUS(15 downto 0));
			
			elsif (ALU = "0011") then
				AR <= (others => '0');
			
			elsif (ALU = "0100") then
				AR <= AR + signed(DATA_BUS(15 downto 0));
				check_flag <= '1';
				
			elsif (ALU = "0101") then
				AR <= AR - signed(DATA_BUS(15 downto 0));
				check_flag <= '1';

			elsif (ALU = "0110") then
				AR <= AR AND signed(DATA_BUS(15 downto 0));
				
			elsif (ALU = "0111") then
				AR <= AR OR signed(DATA_BUS(15 downto 0));
			
			elsif (ALU = "1101") then
				AR <= AR SRL 1;
			elsif (ALU = "1000") then
				AR <= AR(7 downto 0) + signed(grx_utData);

			else
				--nop
			end if;
			
			

			-- Flaggor
			if check_flag = '1' then
				if AR  = 0 then
					Z <= '1';
					N <= '0';
				elsif AR > 0 then
					Z <= '0';
					
				end if;

				if AR(15) ='1' then
					N <= '1';
					Z <= '0';

				elsif AR(15) = '0' then
					N <= '0';
				end if;
				check_flag <= '0';
			end if;
		end if;
	
	end process;



	U_K1 : K1 port map(K1Addr=>opcode, K1Data=>k1_out);
	U_K2 : K2 port map(K2Addr=>addr_mode, K2Data=>k2_out);
	U_GRX : GRx port map(clk=>clk, rst_grx=>btnC, mux=>grxkod,  GrALU => ALU, FB=>FB, TB_Grx => TB, inData=>DATA_BUS(15 downto 0), utData=>grx_utData );

	-- micro memory component connection
	U0 : uMem port map(uAddr=>uPC, uData=>uM);

	-- unified memory component connection
	U1 : pMem port map(
		clk => clk,
		pAddr => ASR,
		pData_in => DATA_BUS(15 downto 0),
		pData_out => PM,
		pFB => FB,
		vga_addr => vga_addr,
		vga_data_out => vga_data
	);
	
	
	--Keyboard encoding and managment
	U2 : kbd_enc port map(clk=>clk, rst_kbd_enc=>btnC, PS2KeyboardCLK=>PS2Clk, PS2KeyboardData=>PS2Data, ScanCode=>ScanCode, make_op=>make_op);
	
	U3 : kbd_mngr port map(clk=>clk, rst_kbd_mngr=>btnC, ScanCode=>ScanCode, make_op=>make_op, player0=>player0, player1=>player1, we=>we_s );
	
	U5 : vga_motor port map(clk=>clk, clear=>btnC, VR_data=>vga_data, VR_addr=>vga_addr, vgaRed=>vgaRed, vgaGreen=>vgaGreen, vgaBlue=>vgaBlue, Hsync=>Hsync, Vsync=>Vsync);

	u6 : Piezo port map(clk=>clk, rst_piezo=>btnC, piezoData => piezo_data, piezoOut => piezo_out);

	JB(0) <= piezo_out; -- using JB[0] (pin 1)
	
	-- data bus assignment
	DATA_BUS
		<= resize(IR(7 downto 0), 24) when (TB = "0010") else
		resize(PM, 24) when (TB = "1011") else
		resize(PC, 24) when (TB = "0100") else
		--resize(PM, 24) when (TB = "0001") else <-- Detta är gamla DM dvs ska vara tileminne
		resize(unsigned(AR), 24) when (TB = "0110") else
		resize(unsigned(SR), 24) when (TB = "1001") else
		resize(grx_utData, 24) when (TB = "0101") else
		uM when (TB = "0111") else
		--UART when (TB = "1010") else
		resize(unsigned(player0), 24) when (TB="1000") else
		resize(unsigned(player1), 24) when (TB="1110") else
		(others => 'X');

end architecture;
