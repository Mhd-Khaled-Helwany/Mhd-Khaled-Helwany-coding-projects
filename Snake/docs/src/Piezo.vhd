library IEEE;
use IEEE.STD_LOGIC_1164.ALL;            -- basic IEEE library
use IEEE.NUMERIC_STD.ALL;   

entity Piezo is 
    port(
        clk     : in std_logic;
        rst_piezo     : in std_logic;
        piezoData: in unsigned(15 downto 0);
        piezoOut : out std_logic
    );
end Piezo;

architecture func of Piezo is
    signal counter : unsigned(15 downto 0) := (others=>'0');
    signal toggle : std_logic := '0';
    signal durationCounter : unsigned(31 downto 0) := (others=>'0');
    constant DURATION : unsigned(31 downto 0) := to_unsigned(50000, 32);
    signal is_playing : std_logic := '0';
    signal stored_freq : unsigned(15 downto 0) := (others=>'0');
begin
    process(clk)
    begin
        if rising_edge(clk) then
            if rst_piezo = '1' then
                counter <= (others=>'0');
                toggle <= '0';
                durationCounter <= (others=>'0');
                is_playing <= '0';
                stored_freq<=(others=>'0');
            elsif piezoData/=0 then
                if is_playing = '0' then
                    stored_freq<=piezoData;
                    is_playing <= '1';
                    counter <= (others=>'0');
                    durationCounter <= (others=>'0');
                else
                    if durationCounter < DURATION then
                        durationCounter <= durationCounter +1;
                        
                        if counter >= stored_freq then
                            counter <= (others=>'0');
                            toggle<=not toggle;
                        else
                            counter <=counter +1;
                        end if;
                    else
                    is_playing <= '0';
                    stored_freq <= (others=>'0');
                    end if;
                end if;
            else 
                toggle<='0';
                is_playing<='0';
            end if;

            if is_playing = '1' then
                piezoOut <= toggle;
            else
                piezoOut <= '0';
            end if;

        end if;
    end process;
end func;

