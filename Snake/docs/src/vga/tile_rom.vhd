--------------------------------------------------------------------------------
-- tile_ROM with integrated palette
-- Version 4.0: 2025-04-23
-- Description:
-- * Single-port ROM containing tile images with integrated palette
-- * Uses 4-bit color indices internally and outputs 12-bit RGB values

-- library declaration
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;            -- basic IEEE library
use IEEE.NUMERIC_STD.ALL;               -- IEEE library for the unsigned type

-- entity
entity tile_rom is
    port (
        clk      : in std_logic;
        addr     : in unsigned(10 downto 0);
        data     : out std_logic_vector(11 downto 0));
end tile_rom;

-- architecture
architecture Behavioral of tile_rom is
    -- Internal 4-bit color indices
    type rom_t is array (0 to 2047) of std_logic_vector(3 downto 0);
    signal rom_data : rom_t;
    
    -- Temporary signal to hold the color index
    signal color_idx : std_logic_vector(3 downto 0);
    
begin
    -- Two-stage process: First read the index, then translate to RGB
    process(clk)
    begin
        if rising_edge(clk) then
            -- Stage 1: Read the index from ROM
            color_idx <= rom_data(to_integer(addr));
            
            -- Stage 2: Translate index to RGB value
            case color_idx is
                when x"0" => data <= x"000";  -- Black (background)
                when x"1" => data <= x"00F";  -- Blue (Snake0)
                when x"2" => data <= x"0F0";  -- Green (Snake1)
                when x"3" => data <= x"F00";  -- Red (Food)
                when x"4" => data <= x"888";  -- Light Gray (Wall outer)
                when x"5" => data <= x"666";  -- Dark Gray (Wall inner)
                when x"F" => data <= x"FFF";  -- White (Eyes, etc.)
                when others => data <= x"000"; -- Default black
            end case;
        end if;
    end process;
    
    -- ROM data stored as 4-bit indices
    rom_data <= (
        -- Empty space (black background) - Index 0
        x"0",x"0",x"0",x"0",x"0",x"0",x"0",x"0",
        x"0",x"0",x"0",x"0",x"0",x"0",x"0",x"0",
        x"0",x"0",x"0",x"0",x"0",x"0",x"0",x"0",
        x"0",x"0",x"0",x"0",x"0",x"0",x"0",x"0",
        x"0",x"0",x"0",x"0",x"0",x"0",x"0",x"0",
        x"0",x"0",x"0",x"0",x"0",x"0",x"0",x"0",
        x"0",x"0",x"0",x"0",x"0",x"0",x"0",x"0",
        x"0",x"0",x"0",x"0",x"0",x"0",x"0",x"0",
        
        -- Snake0 head UP (blue) - Index 1
        x"0",x"0",x"1",x"1",x"1",x"1",x"0",x"0",
        x"0",x"1",x"1",x"1",x"1",x"1",x"1",x"0",
        x"1",x"1",x"F",x"1",x"1",x"F",x"1",x"1",
        x"1",x"1",x"1",x"1",x"1",x"1",x"1",x"1",
        x"1",x"1",x"1",x"1",x"1",x"1",x"1",x"1",
        x"1",x"1",x"1",x"1",x"1",x"1",x"1",x"1",
        x"0",x"1",x"1",x"0",x"0",x"1",x"1",x"0",
        x"0",x"0",x"1",x"1",x"1",x"1",x"0",x"0",
        
        -- Snake0 head RIGHT (blue) - Index 2
        x"0",x"0",x"1",x"1",x"1",x"1",x"0",x"0",
        x"0",x"1",x"1",x"1",x"1",x"1",x"1",x"0",
        x"1",x"1",x"1",x"1",x"1",x"F",x"1",x"1",
        x"1",x"0",x"1",x"1",x"1",x"F",x"1",x"1",
        x"1",x"0",x"1",x"1",x"1",x"1",x"1",x"1",
        x"1",x"1",x"1",x"1",x"1",x"1",x"1",x"1",
        x"0",x"1",x"1",x"1",x"1",x"1",x"1",x"0",
        x"0",x"0",x"1",x"1",x"1",x"1",x"0",x"0",
        
        -- Snake0 head DOWN (blue) - Index 3
        x"0",x"0",x"1",x"1",x"1",x"1",x"0",x"0",
        x"0",x"1",x"1",x"0",x"0",x"1",x"1",x"0",
        x"1",x"1",x"1",x"1",x"1",x"1",x"1",x"1",
        x"1",x"1",x"1",x"1",x"1",x"1",x"1",x"1",
        x"1",x"1",x"1",x"1",x"1",x"1",x"1",x"1",
        x"1",x"1",x"F",x"1",x"1",x"F",x"1",x"1",
        x"0",x"1",x"1",x"1",x"1",x"1",x"1",x"0",
        x"0",x"0",x"1",x"1",x"1",x"1",x"0",x"0",
        
        -- Snake0 head LEFT (blue) - Index 4
        x"0",x"0",x"1",x"1",x"1",x"1",x"0",x"0",
        x"0",x"1",x"1",x"1",x"1",x"1",x"1",x"0",
        x"1",x"1",x"F",x"1",x"1",x"1",x"1",x"1",
        x"1",x"1",x"F",x"1",x"1",x"1",x"0",x"1",
        x"1",x"1",x"1",x"1",x"1",x"1",x"0",x"1",
        x"1",x"1",x"1",x"1",x"1",x"1",x"1",x"1",
        x"0",x"1",x"1",x"1",x"1",x"1",x"1",x"0",
        x"0",x"0",x"1",x"1",x"1",x"1",x"0",x"0",
        
        -- Snake0 body (blue square) - Index 5
        x"1",x"1",x"1",x"1",x"1",x"1",x"1",x"1",
        x"1",x"1",x"1",x"1",x"1",x"1",x"1",x"1",
        x"1",x"1",x"1",x"1",x"1",x"1",x"1",x"1",
        x"1",x"1",x"1",x"1",x"1",x"1",x"1",x"1",
        x"1",x"1",x"1",x"1",x"1",x"1",x"1",x"1",
        x"1",x"1",x"1",x"1",x"1",x"1",x"1",x"1",
        x"1",x"1",x"1",x"1",x"1",x"1",x"1",x"1",
        x"1",x"1",x"1",x"1",x"1",x"1",x"1",x"1",
        
        -- Food (red apple/circle) - Index 6
        x"0",x"0",x"3",x"3",x"3",x"3",x"0",x"0",
        x"0",x"3",x"3",x"3",x"3",x"3",x"3",x"0",
        x"3",x"3",x"3",x"3",x"3",x"3",x"3",x"3",
        x"3",x"3",x"3",x"3",x"3",x"3",x"3",x"3",
        x"3",x"3",x"3",x"3",x"3",x"3",x"3",x"3",
        x"3",x"3",x"3",x"3",x"3",x"3",x"3",x"3",
        x"0",x"3",x"3",x"3",x"3",x"3",x"3",x"0",
        x"0",x"0",x"3",x"3",x"3",x"3",x"0",x"0",
        
        -- Walls (gray blocks) - Index 7
        x"4",x"4",x"4",x"4",x"4",x"4",x"4",x"4",
        x"4",x"5",x"5",x"5",x"5",x"5",x"5",x"4",
        x"4",x"5",x"4",x"4",x"4",x"4",x"5",x"4",
        x"4",x"5",x"4",x"5",x"5",x"4",x"5",x"4",
        x"4",x"5",x"4",x"5",x"5",x"4",x"5",x"4",
        x"4",x"5",x"4",x"4",x"4",x"4",x"5",x"4",
        x"4",x"5",x"5",x"5",x"5",x"5",x"5",x"4",
        x"4",x"4",x"4",x"4",x"4",x"4",x"4",x"4",
        
        -- Snake1 head UP (green) - Index 8
        x"0",x"0",x"2",x"2",x"2",x"2",x"0",x"0",
        x"0",x"2",x"2",x"2",x"2",x"2",x"2",x"0",
        x"2",x"2",x"F",x"2",x"2",x"F",x"2",x"2",
        x"2",x"2",x"2",x"2",x"2",x"2",x"2",x"2",
        x"2",x"2",x"2",x"2",x"2",x"2",x"2",x"2",
        x"2",x"2",x"2",x"2",x"2",x"2",x"2",x"2",
        x"0",x"2",x"2",x"0",x"0",x"2",x"2",x"0",
        x"0",x"0",x"2",x"2",x"2",x"2",x"0",x"0",
        
        -- Snake1 head RIGHT (green) - Index 9
        x"0",x"0",x"2",x"2",x"2",x"2",x"0",x"0",
        x"0",x"2",x"2",x"2",x"2",x"2",x"2",x"0",
        x"2",x"2",x"2",x"2",x"2",x"F",x"2",x"2",
        x"2",x"0",x"2",x"2",x"2",x"F",x"2",x"2",
        x"2",x"0",x"2",x"2",x"2",x"2",x"2",x"2",
        x"2",x"2",x"2",x"2",x"2",x"2",x"2",x"2",
        x"0",x"2",x"2",x"2",x"2",x"2",x"2",x"0",
        x"0",x"0",x"2",x"2",x"2",x"2",x"0",x"0",
        
        -- Snake1 head DOWN (green) - Index 10
        x"0",x"0",x"2",x"2",x"2",x"2",x"0",x"0",
        x"0",x"2",x"2",x"0",x"0",x"2",x"2",x"0",
        x"2",x"2",x"2",x"2",x"2",x"2",x"2",x"2",
        x"2",x"2",x"2",x"2",x"2",x"2",x"2",x"2",
        x"2",x"2",x"2",x"2",x"2",x"2",x"2",x"2",
        x"2",x"2",x"F",x"2",x"2",x"F",x"2",x"2",
        x"0",x"2",x"2",x"2",x"2",x"2",x"2",x"0",
        x"0",x"0",x"2",x"2",x"2",x"2",x"0",x"0",
        
        -- Snake1 head LEFT (green) - Index 11
        x"0",x"0",x"2",x"2",x"2",x"2",x"0",x"0",
        x"0",x"2",x"2",x"2",x"2",x"2",x"2",x"0",
        x"2",x"2",x"F",x"2",x"2",x"2",x"2",x"2",
        x"2",x"2",x"F",x"2",x"2",x"2",x"0",x"2",
        x"2",x"2",x"2",x"2",x"2",x"2",x"0",x"2",
        x"2",x"2",x"2",x"2",x"2",x"2",x"2",x"2",
        x"0",x"2",x"2",x"2",x"2",x"2",x"2",x"0",
        x"0",x"0",x"2",x"2",x"2",x"2",x"0",x"0",
        
        -- Snake1 body (green square) - Index 12
        x"2",x"2",x"2",x"2",x"2",x"2",x"2",x"2",
        x"2",x"2",x"2",x"2",x"2",x"2",x"2",x"2",
        x"2",x"2",x"2",x"2",x"2",x"2",x"2",x"2",
        x"2",x"2",x"2",x"2",x"2",x"2",x"2",x"2",
        x"2",x"2",x"2",x"2",x"2",x"2",x"2",x"2",
        x"2",x"2",x"2",x"2",x"2",x"2",x"2",x"2",
        x"2",x"2",x"2",x"2",x"2",x"2",x"2",x"2",
        x"2",x"2",x"2",x"2",x"2",x"2",x"2",x"2",
        
        -- Fill the remaining ROM space with zeros (black)
        others => x"0"
    );
end Behavioral;