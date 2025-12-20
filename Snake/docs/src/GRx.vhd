library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity GRx is
   port(
    clk     : in std_logic;
    rst_grx     : in std_logic;
    mux     : in unsigned(1 downto 0);
    GrALU : in unsigned(3 downto 0);
    FB      : in unsigned(3 downto 0);
    TB_Grx : in unsigned(3 downto 0);
    inData  : in unsigned(15 downto 0);
    utData  : out unsigned(15 downto 0)
   );
end entity;

architecture func of GRx is
    signal GR0, GR1, GR2, GR3 : unsigned(15 downto 0);
begin

    -- GR0
    process(clk)
    begin
        if rising_edge(clk) then
            if rst_grx = '1' then
                GR0 <= (others => '0');
            elsif (FB = "0101" and mux = "00") then  -- FIXED
                if GrALU = "1111" then
                    GR0 <= "00000000" & inData(7 downto 0);
                else
                    GR0 <= inData;
                end if;

            end if;
        end if;
    end process;

    -- GR1
    process(clk)
    begin
        if rising_edge(clk) then
            if rst_grx = '1' then
                GR1 <= (others => '0');
            elsif (FB = "0101" and mux = "01") then
                if GrALU = "1111" then
                    GR1 <= "00000000" & inData(7 downto 0);
                else
                    GR1 <= inData;
                end if;

            end if;
        end if;
    end process;

    -- GR2
    process(clk)
    begin
        if rising_edge(clk) then
            if rst_grx = '1' then
                GR2 <= (others => '0');
            elsif (FB = "0101" and mux = "10") then
                if GrALU = "1111" then
                    GR2 <= "00000000" & inData(7 downto 0);
                else
                    GR2 <= inData;
                end if;
            end if;
        end if;
    end process;

    -- GR3
    process(clk)
    begin
        if rising_edge(clk) then
            if rst_grx = '1' then
                GR3 <= (others => '0');
            elsif (FB = "0101" and mux = "11") then
                if GrALU = "1111" then
                    GR3 <= "00000000" & inData(7 downto 0);
                else
                    GR3 <= inData;
                end if;
            end if;
        end if;
    end process;

    -- Read from register
    utData <= GR3 when GrALU = "1000" else
        GR0 when mux = "00" else
            GR1 when mux = "01" else
            GR2 when mux = "10" else
            GR3;

end architecture;