library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity uart is
    Port (
        clk : in std_logic;
        rst : in std_logic;
        data : out std_logic_vector(7 downto 0);
        rd : out std_logic;
        RsRx : in std_logic;
    );
end uart;

architecture behavioral of uart is
    constant FULL_BIT := 434;
    constant HALF_BIT := 868;

    signal rx1,rx2 : std_logic;
    signal clear : std_logic;
    signal sp : std_logic; -- skifta
    signal lp : std_logic; -- ladda
    signal sreg : std_logic_vector(9 downto 0) := B"0_00000000_0";  -- 10 bit shift register
    signal bit_count : unsigned(3 downto 0);
    signal s1 : std_logic;
    --temp utsignal
    signal tmp_data : std_logic_vector(7 downto 0);
    signal tmp_rd : std_logic;
begin
    clear <= rst;


    -- Synkvippor
    process(clk) begin
        if rising_edge(clk) then
          if clear = '1' then
            rx1 <= '0';
            rx2 <= '0';
          else
            rx1 <= RsRx;
            rx2 <= rx1;
          end if;
        end if;
      end process;

    
    -- styreneht
    process(clk) begin
      if rising_edge(clk) then
        sp <= '0';
        lp <= '0';
        
       
       if clear = '1' then
          s1 <= '0';
          counter <= (others => '0');
          bit_count <= (others => '0');
        else
          
          if s1 = '0' then
            -- Väntar på start bit
            if(rx2 = '1' and rx1='0') then
              s1 <= '1';
              counter <= (others => '0');
              bit_count <= (others => '0');
            end if;
            
         else -- count_enable
            counter <= counter +1;
            
         -- Läs av mitten på en bit
            if counter = HALF_BIT then
              sp <= '1';
              bit_count <= bit_count +1;
            end if;
            
           -- Bit avläst
            if counter = FULL_BIT then
              counter <= (others => '0');
            end if;
            
           -- Tagit emot 10 bitar, dvs 2 start/stop + 8 bitar
            if bit_count = 10 then
              lp <= '1';
              s1 <= '0';
              bit_count <= (others => '0');
              counter <= (others => '0');
            end if;
            
         end if;
        end if;
      end if;
    end process;
    

end Behavioral;