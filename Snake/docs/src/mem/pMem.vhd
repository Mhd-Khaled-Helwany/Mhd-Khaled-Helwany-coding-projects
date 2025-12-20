library IEEE;
use IEEE.STD_LOGIC_1164.all;
use IEEE.NUMERIC_STD.all;

entity pMem is
  port(
    clk       : in std_logic;
    pAddr     : in unsigned(15 downto 0);
    pData_in  : in unsigned(15 downto 0);
    pData_out : out unsigned(15 downto 0);
    pFb       : in unsigned(3 downto 0);
    
    --vga port (read only)
    vga_addr  : in unsigned(10 downto 0);
    vga_data_out : out std_logic_vector(7 downto 0)
    
    );
end pMem;

architecture Behavioral of pMem is
  -- Constants for memory layout
  constant TILE_MEM_SIZE : integer := 2048;
  constant TILE_MEM_START : integer := 32000 - TILE_MEM_SIZE; 
  
  -- Program memory section (0 to 63487)
  type prog_mem_t is array (0 to TILE_MEM_START-1) of std_logic_vector(15 downto 0);
  shared variable PROG_RAM : prog_mem_t := (
    x"0403",          -- LDI     GR1, 3
   x"2600",          -- STORE   GR1, SNAKE_ONE_HEAD_INDEX
   x"0189",
   x"1600",          -- LD     GR1, ONE
   x"0180",
   x"2600",          -- STORE   GR1, SNAKE_ONE_TAIL_INDEX
   x"018A",
   x"0400",          -- LDI     GR1, 0
   x"1E00",          -- LD      GR3, SNAKE_ONE_START
   x"0187",
   x"0400",          -- LDI     GR1, 0
   x"2600",          -- STORE   GR1, SNAKE_ONE_DIR
   x"018D",
   x"2700",          -- STORE GR1, [0]
   x"3E00",          -- ADD GR3, ONE
   x"0180",
   x"7E00",          -- CMP GR3, SNAKE_ONE_END
   x"0188",
   x"9100",          -- BEQ SNAKE_ONE_DEFAULT_DIRECTIONS
   x"0016",
   x"8100",          -- BRA SNAKE_ONE_CLEAR_LP
   x"000D",
   x"1E00",          -- LD      GR3, SNAKE_ONE_START
   x"0187",
   x"0404",          -- LDI GR1, 4
   x"2600",          -- STORE   GR1, SNAKE_ONE_DIR
   x"018D",
   x"3E00",          -- ADD GR3, ONE
   x"0180",
   x"2700",          -- STORE GR1, [0]
   x"3E00",          -- ADD GR3, ONE
   x"0180",
   x"2700",          -- STORE GR1, [0]
   x"3E00",          -- ADD GR3, ONE
   x"0180",
   x"2700",          -- STORE GR1, [0]
   x"0403",
   x"2600",          -- LDI GR1, 3
   x"018B",          -- STORE GR1, SNAKE_ONE_X
   x"0400",
   x"2600",          -- LDI GR1, 0
   x"018C",          -- STORE GR1, SNAKE_ONE_Y
   x"0401",
   x"2600",          -- LDI GR1, 1
   x"018E",          -- STORE GR1, SNAKE_ONE_TAIL_X
   x"0400",
   x"2600",          -- LDI GR1, 0
   x"018F",          -- STORE GR1, SNAKE_ONE_TAIL_Y
   x"C000",          -- KEY0    GR0
   x"7200",          -- CMP     GR0, KEY_PREV
   x"0173",
   x"9100",          -- BEQ     skip_key
   x"0073",
   x"2200",          -- STORE   GR0, KEY_PREV
   x"0173",
   x"7200",          -- CMP     GR0, KEY_W
   x"0177",
   x"9100",          -- BEQ     set_up
   x"0049",
   x"7200",          -- CMP     GR0, KEY_A
   x"0175",
   x"9100",          -- BEQ     set_left
   x"0054",
   x"7200",          -- CMP     GR0, KEY_S
   x"0178",
   x"9100",          -- BEQ     set_down
   x"005F",
   x"7200",          -- CMP     GR0, KEY_D
   x"0176",
   x"9100",          -- BEQ     set_right
   x"006A",
   x"8100",          -- BRA     SNAKE_ONE_CLEAR
   x"0000",
   x"1600",          -- LD      GR1, THREE
   x"0182",
   x"7600",          -- CMP     GR1, SNAKE_ONE_DIR
   x"018D",
   x"9100",          -- BEQ     skip_key
   x"0073",
   x"0401",          -- LDI     GR1, 1
   x"2600",          -- STORE   GR1, SNAKE_ONE_DIR
   x"018D",
   x"8100",          -- BRA     skip_key
   x"0073",
   x"1600",          -- LD      GR1, FOUR
   x"0183",
   x"7600",          -- CMP     GR1, SNAKE_ONE_DIR
   x"018D",
   x"9100",          -- BEQ     skip_key
   x"0073",
   x"0402",          -- LDI     GR1, 2
   x"2600",          -- STORE   GR1, SNAKE_ONE_DIR
   x"018D",
   x"8100",          -- BRA     skip_key
   x"0073",
   x"1600",          -- LD      GR1, ONE
   x"0180",
   x"7600",          -- CMP     GR1, SNAKE_ONE_DIR
   x"018D",
   x"9100",          -- BEQ     skip_key
   x"0073",
   x"0403",          -- LDI     GR1, 3
   x"2600",          -- STORE   GR1, SNAKE_ONE_DIR
   x"018D",
   x"8100",          -- BRA     skip_key
   x"0073",
   x"1600",          -- LD      GR1, TWO
   x"0181",
   x"7600",          -- CMP     GR1, SNAKE_ONE_DIR
   x"018D",
   x"9100",          -- BEQ     skip_key
   x"0073",
   x"0404",          -- LDI     GR1, 4
   x"2600",          -- STORE   GR1, SNAKE_ONE_DIR
   x"018D",
   x"1A00",          -- LD      GR2, DELAY_DIV_MAX
   x"017C",
   x"1600",          -- LD      GR1, DELAY_MAX
   x"017B",
   x"4600",          -- SUB     GR1, ONE
   x"0180",
   x"7600",          -- CMP     GR1, ZERO
   x"017F",
   x"9100",          -- BEQ     end_delay_lp
   x"007F",
   x"8100",          -- BRA     delay_lp
   x"0077",
   x"4A00",          -- SUB     GR2, ONE
   x"0180",
   x"7A00",          -- CMP     GR2, ZERO
   x"017F",
   x"9100",          -- BEQ     end_delay
   x"0087",
   x"8100",          -- BRA     start_lp
   x"0075",
   x"1200",          -- LD GR0, ZERO
   x"017F",
   x"1600",          -- LD GR1, SNAKE_ONE_TAIL_X
   x"018E",
   x"1A00",          -- LD GR2, SNAKE_ONE_TAIL_Y
   x"018F",
   x"8100",          -- BRA draw_tile
   x"0153",
   x"1E00",          -- LD      GR3, SNAKE_ONE_START
   x"0187",
   x"3E00",          -- ADD     GR3, SNAKE_ONE_TAIL_INDEX
   x"018A",
   x"1700",          -- LD      GR1, [0]
   x"7600",          -- CMP     GR1, ONE
   x"0180",
   x"9100",          -- BEQ     update_tail_position_up
   x"00A6",
   x"7600",          -- CMP     GR1, TWO
   x"0181",
   x"9100",          -- BEQ     update_tail_position_left
   x"00AE",
   x"7600",          -- CMP     GR1, THREE
   x"0182",
   x"9100",          -- BEQ     update_tail_position_down
   x"00B6",
   x"7600",          -- CMP     GR1, FOUR
   x"0183",
   x"9100",          -- BEQ     update_tail_position_right
   x"00BE",
   x"8100",          -- BRA     SNAKE_ONE_CLEAR
   x"0000",
   x"1200",          -- LD GR0, SNAKE_ONE_TAIL_Y
   x"018F",
   x"4200",          -- SUB GR0, ONE
   x"0180",
   x"2200",          -- STORE GR0, SNAKE_ONE_TAIL_Y
   x"018F",
   x"8100",          -- BRA     update_tail_position_index
   x"00C6",
   x"1200",          -- LD GR0, SNAKE_ONE_TAIL_X
   x"018E",
   x"4200",          -- SUB GR0, ONE
   x"0180",
   x"2200",          -- STORE GR0, SNAKE_ONE_TAIL_X
   x"018E",
   x"8100",          -- BRA     update_tail_position_index
   x"00C6",
   x"1200",          -- LD GR0, SNAKE_ONE_TAIL_Y
   x"018F",
   x"3200",          -- ADD GR0, ONE
   x"0180",
   x"2200",          -- STORE GR0, SNAKE_ONE_TAIL_Y
   x"018F",
   x"8100",          -- BRA     update_tail_position_index
   x"00C6",
   x"1200",          -- LD GR0, SNAKE_ONE_TAIL_X
   x"018E",
   x"3200",          -- ADD GR0, ONE
   x"0180",
   x"2200",          -- STORE GR0, SNAKE_ONE_TAIL_X
   x"018E",
   x"8100",          -- BRA     update_tail_position_index
   x"00C6",
   x"1200",          -- LD GR0, SNAKE_ONE_TAIL_INDEX
   x"018A",
   x"7200",          -- CMP GR0, MAX_INDEX
   x"0186",
   x"9100",          -- BEQ wrap_tail_position_index
   x"00D2",
   x"3200",          -- ADD GR0, ONE
   x"0180",
   x"2200",          -- STORE GR0, SNAKE_ONE_TAIL_INDEX
   x"018A",
   x"8100",          -- BRA update_old_head
   x"00D6",
   x"1200",          -- LD GR0, ZERO
   x"017F",
   x"2200",          -- STORE GR0, SNAKE_ONE_TAIL_INDEX
   x"018A",
   x"1200",          -- LD GR0, FIVE
   x"0184",
   x"1600",          -- LD GR1, SNAKE_ONE_X
   x"018B",
   x"1A00",          -- LD GR2, SNAKE_ONE_Y
   x"018C",
   x"8100",          -- BRA draw_tile
   x"0153",
   x"1A00",          -- LD      GR2, SNAKE_ONE_DIR
   x"018D",
   x"1E00",          -- LD      GR3, SNAKE_ONE_START
   x"0187",
   x"3E00",          -- ADD     GR3, SNAKE_ONE_HEAD_INDEX
   x"0189",
   x"2B00",          -- STORE   GR2, [0]
   x"7E00",          -- CMP     GR3, SNAKE_ONE_END
   x"0188",
   x"9100",          -- BEQ     wrap_head_index
   x"00F1",
   x"1600",          -- LD      GR1, SNAKE_ONE_HEAD_INDEX
   x"0189",
   x"3600",          -- ADD     GR1, ONE
   x"0180",
   x"2600",          -- STORE   GR1, SNAKE_ONE_HEAD_INDEX
   x"0189",
   x"8100",          -- BRA     try_directions
   x"00F5",
   x"1600",          -- LD GR1, ZERO
   x"017F",
   x"2600",          -- STORE GR1, SNAKE_ONE_HEAD_INDEX
   x"0189",
   x"1E00",          -- LD      GR3, SNAKE_ONE_START
   x"0187",
   x"3E00",          -- ADD     GR3, SNAKE_ONE_HEAD_INDEX
   x"0189",
   x"7A00",          -- CMP     GR2, ZERO
   x"017F",
   x"9100",          -- BEQ     SNAKE_ONE_CLEAR
   x"0000",
   x"7A00",          -- CMP     GR2, ONE
   x"0180",
   x"9100",          -- BEQ     try_up
   x"010D",
   x"7A00",          -- CMP     GR2, TWO
   x"0181",
   x"9100",          -- BEQ     try_left
   x"011F",
   x"7A00",          -- CMP     GR2, THREE
   x"0182",
   x"9100",          -- BEQ     try_down
   x"0131",
   x"7A00",          -- CMP     GR2, FOUR
   x"0183",
   x"9100",          -- BEQ     try_right
   x"0143",
   x"1A00",          -- LD      GR2, SNAKE_ONE_Y
   x"018C",
   x"7A00",          -- CMP     GR2, ZERO
   x"017F",
   x"9100",          -- BEQ     SNAKE_ONE_CLEAR
   x"0000",
   x"4A00",          -- SUB     GR2, ONE
   x"0180",
   x"2A00",          -- STORE   GR2, SNAKE_ONE_Y
   x"018C",
   x"1200",          -- LD      GR0, ONE
   x"0180",
   x"1600",          -- LD      GR1, SNAKE_ONE_X
   x"018B",
   x"1A00",          -- LD      GR2, SNAKE_ONE_Y
   x"018C",
   x"8100",          -- BRA     draw_tile
   x"0153",
   x"1A00",          -- LD      GR2, SNAKE_ONE_X
   x"018B",
   x"7A00",          -- CMP     GR2, ZERO
   x"017F",
   x"9100",          -- BEQ     SNAKE_ONE_CLEAR
   x"0000",
   x"4A00",          -- SUB     GR2, ONE
   x"0180",
   x"2A00",          -- STORE   GR2, SNAKE_ONE_X
   x"018B",
   x"1200",          -- LD      GR0, FOUR
   x"0183",
   x"1600",          -- LD      GR1, SNAKE_ONE_X
   x"018B",
   x"1A00",          -- LD      GR2, SNAKE_ONE_Y
   x"018C",
   x"8100",          -- BRA     draw_tile
   x"0153",
   x"1A00",          -- LD      GR2, SNAKE_ONE_Y
   x"018C",
   x"7A00",          -- CMP     GR2, MAX_Y
   x"017E",
   x"9100",          -- BEQ     SNAKE_ONE_CLEAR
   x"0000",
   x"3A00",          -- ADD     GR2, ONE
   x"0180",
   x"2A00",          -- STORE   GR2, SNAKE_ONE_Y
   x"018C",
   x"1200",          -- LD      GR0, THREE
   x"0182",
   x"1600",          -- LD      GR1, SNAKE_ONE_X
   x"018B",
   x"1A00",          -- LD      GR2, SNAKE_ONE_Y
   x"018C",
   x"8100",          -- BRA     draw_tile
   x"0153",
   x"1A00",          -- LD      GR2, SNAKE_ONE_X
   x"018B",
   x"7A00",          -- CMP     GR2, MAX_X
   x"017D",
   x"9100",          -- BEQ     SNAKE_ONE_CLEAR
   x"0000",
   x"3A00",          -- ADD     GR2, ONE
   x"0180",
   x"2A00",          -- STORE   GR2, SNAKE_ONE_X
   x"018B",
   x"1200",          -- LD      GR0, TWO
   x"0181",
   x"1600",          -- LD      GR1, SNAKE_ONE_X
   x"018B",
   x"1A00",          -- LD      GR2, SNAKE_ONE_Y
   x"018C",
   x"1E00",          -- LD      GR3, LOW_BOUND
   x"0179",
   x"2600",          -- STORE   GR1, TEMP
   x"0174",
   x"3E00",          -- ADD     GR3, TEMP
   x"0174",
   x"2A00",          -- STORE   GR2, TEMP
   x"0174",
   x"1A00",          -- LD      GR2, ZERO
   x"017F",
   x"7A00",          -- CMP     GR2, TEMP
   x"0174",
   x"9100",          -- BEQ     draw_store
   x"0167",
   x"3E00",          -- ADD     GR3, TWENTY
   x"0185",
   x"3A00",          -- ADD     GR2, ONE
   x"0180",
   x"8100",          -- BRA     Y_draw_lp
   x"015D",
   x"2300",          -- STORE   GR0, [0]
   x"7200",          -- CMP     GR0, ZERO
   x"017F",
   x"9100",          -- BEQ     update_tail_position
   x"008F",
   x"7200",          -- CMP     GR0, FIVE
   x"0184",
   x"9100",          -- BEQ     update_old_head_return
   x"00DE",
   x"8100",          -- BRA     main_loop
   x"0030",
   x"7502",          -- POS                     :   .byte   29954
   x"0000",          -- KEY_PREV                :   .byte   0
   x"0000",          -- TEMP                    :   .byte   0
   x"001C",          -- KEY_A                   :   .byte   0x1C
   x"0023",          -- KEY_D                   :   .byte   0x23
   x"001D",          -- KEY_W                   :   .byte   0x1D
   x"001B",          -- KEY_S                   :   .byte   0x1B
   x"7500",          -- LOW_BOUND               :   .byte   29952
   x"762B",          -- HIGH_BOUND              :   .byte   30251
   x"FDE8",          -- DELAY_MAX               :   .byte   65000
   x"0005",          -- DELAY_DIV_MAX           :   .byte   5
   x"0013",          -- MAX_X                   :   .byte   19
   x"000E",          -- MAX_Y                   :   .byte   14
   x"0000",          -- ZERO                    :   .byte   0
   x"0001",          -- ONE                     :   .byte   1
   x"0002",          -- TWO                     :   .byte   2
   x"0003",          -- THREE                   :   .byte   3
   x"0004",          -- FOUR                    :   .byte   4
   x"0005",          -- FIVE                    :   .byte   5
   x"0014",          -- TWENTY                  :   .byte   20
   x"012C",          -- MAX_INDEX               :   .byte 300
   x"6D60",          -- SNAKE_ONE_START         :   .byte 28000
   x"6E8C",          -- SNAKE_ONE_END           :   .byte 28300
   x"0003",          -- SNAKE_ONE_HEAD_INDEX    :   .byte 3
   x"0001",          -- SNAKE_ONE_TAIL_INDEX    :   .byte 1
   x"0003",          -- SNAKE_ONE_X             :   .byte 3
   x"0000",          -- SNAKE_ONE_Y             :   .byte 0
   x"0000",          -- SNAKE_ONE_DIR           :   .byte 0
   x"0001",          -- SNAKE_ONE_TAIL_X        :   .byte 1
   x"0000",           -- SNAKE_ONE_TAIL_Y        :   .byte 0
    others => (others => '0')
  );
  
  -- Tile memory section (63488-65535)
  type tile_mem_t is array (0 to TILE_MEM_SIZE-1) of std_logic_vector(7 downto 0);
  --shared variable TILE_RAM : tile_mem_t := (others => x"00");
  shared variable TILE_RAM : tile_mem_t := (others => x"00");
  signal tile_addr : unsigned(10 downto 0);
  --attribute ram_style : string;
  --attribute ram_style of TILE_RAM : signal is "block";
begin
  tile_addr <= resize(pAddr - TILE_MEM_START, 11);
  -- CPU port process
  process(clk)
  begin
    if rising_edge(clk) then
      -- Read operations
      if pAddr >= TILE_MEM_START then
        -- Reading from tile memory (high addresses)
        --pData_out <= resize(unsigned(TILE_RAM(to_integer(tile_addr))), 16);

        if (pFb = "1011") then
          TILE_RAM(to_integer(tile_addr)) := std_logic_vector(pData_in(7 downto 0));

        end if;
      else
        -- Reading from program memory (low addresses)
        pData_out <= unsigned(PROG_RAM(to_integer(pAddr)));
        if (pFb = "1011") then
          PROG_RAM(to_integer(pAddr)) := std_logic_vector(pData_in);

        end if;
      end if;
      
      -- Write operations

    end if;
  end process;

  -- VGA port (read only from tile memory)
  process(clk)
  begin
    if rising_edge(clk) then
      vga_data_out <= TILE_RAM(to_integer(vga_addr));
    end if;
  end process;

end Behavioral;