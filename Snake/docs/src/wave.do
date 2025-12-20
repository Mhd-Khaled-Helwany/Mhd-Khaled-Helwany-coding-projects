onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate -group KB /uprogcpu_tb/PS2KeyboardCLK
add wave -noupdate -group KB /uprogcpu_tb/PS2KeyboardData
add wave -noupdate -group KB /uprogcpu_tb/uut/PS2Clk
add wave -noupdate -group KB /uprogcpu_tb/uut/PS2Data
add wave -noupdate -group VGA /uprogcpu_tb/uut/Hsync
add wave -noupdate -group VGA /uprogcpu_tb/uut/Vsync
add wave -noupdate -group VGA /uprogcpu_tb/uut/vgaRed
add wave -noupdate -group VGA /uprogcpu_tb/uut/vgaGreen
add wave -noupdate -group VGA /uprogcpu_tb/uut/vgaBlue
add wave -noupdate /uprogcpu_tb/uut/JB
add wave -noupdate /uprogcpu_tb/uut/clk
add wave -noupdate /uprogcpu_tb/uut/rst
add wave -noupdate -group uProg /uprogcpu_tb/uut/uPC
add wave -noupdate -group uProg -color pink /uprogcpu_tb/uut/IR
add wave -noupdate -group uProg /uprogcpu_tb/uut/FB
add wave -noupdate -group uProg /uprogcpu_tb/uut/TB
add wave -noupdate -group uProg /uprogcpu_tb/uut/ALU
add wave -noupdate -group uProg /uprogcpu_tb/uut/U_K1/K1Addr
add wave -noupdate -group uProg /uprogcpu_tb/uut/U_K1/K1Data
add wave -noupdate -group uProg /uprogcpu_tb/uut/U_K2/k2Addr
add wave -noupdate -group uProg /uprogcpu_tb/uut/U_K2/k2Data
add wave -noupdate -group uProg /uprogcpu_tb/uut/uM
add wave -noupdate -group GRx /uprogcpu_tb/uut/U_GRX/GR0
add wave -noupdate -group GRx /uprogcpu_tb/uut/U_GRX/GR1
add wave -noupdate -group GRx /uprogcpu_tb/uut/U_GRX/GR2
add wave -noupdate -group GRx /uprogcpu_tb/uut/U_GRX/GR3
add wave -noupdate -color pink /uprogcpu_tb/uut/PC
add wave -noupdate /uprogcpu_tb/uut/ASR
add wave -noupdate /uprogcpu_tb/uut/PM
add wave -noupdate /uprogcpu_tb/uut/AR
add wave -noupdate /uprogcpu_tb/uut/SR
add wave -noupdate /uprogcpu_tb/uut/Z
add wave -noupdate /uprogcpu_tb/uut/SEQ
add wave -noupdate /uprogcpu_tb/uut/N
add wave -noupdate /uprogcpu_tb/uut/check_flag
add wave -noupdate /uprogcpu_tb/uut/DATA_BUS
add wave -noupdate /uprogcpu_tb/uut/ScanCode
add wave -noupdate /uprogcpu_tb/uut/make_op
add wave -noupdate /uprogcpu_tb/uut/we_s
add wave -noupdate /uprogcpu_tb/uut/pMem_data_out
add wave -noupdate /uprogcpu_tb/uut/vga_data
add wave -noupdate /uprogcpu_tb/uut/vga_addr
add wave -noupdate /uprogcpu_tb/uut/TM_select
add wave -noupdate /uprogcpu_tb/uut/DM_select
add wave -noupdate /uprogcpu_tb/uut/player0
add wave -noupdate /uprogcpu_tb/uut/player1
add wave -noupdate /uprogcpu_tb/uut/piezo_data
add wave -noupdate /uprogcpu_tb/uut/piezo_out
add wave -noupdate /uprogcpu_tb/uut/opcode
add wave -noupdate /uprogcpu_tb/uut/addr_mode
add wave -noupdate /uprogcpu_tb/uut/k1_out
add wave -noupdate /uprogcpu_tb/uut/k2_out
add wave -noupdate /uprogcpu_tb/uut/grx_inData
add wave -noupdate /uprogcpu_tb/uut/grx_utData
add wave -noupdate /uprogcpu_tb/uut/addrC
add wave -noupdate /uprogcpu_tb/uut/mKod
add wave -noupdate /uprogcpu_tb/uut/grxkod
add wave -noupdate /uprogcpu_tb/uut/opkod
add wave -noupdate /uprogcpu_tb/uut/uAddr
add wave -noupdate /uprogcpu_tb/uut/PCsig
add wave -noupdate /uprogcpu_tb/uut/U_K1/K1_mem
add wave -noupdate /uprogcpu_tb/uut/U_K2/K2_mem
add wave -noupdate /uprogcpu_tb/uut/U_GRX/clk
add wave -noupdate /uprogcpu_tb/uut/U_GRX/rst_grx
add wave -noupdate /uprogcpu_tb/uut/U_GRX/mux
add wave -noupdate /uprogcpu_tb/uut/U_GRX/GrALU
add wave -noupdate /uprogcpu_tb/uut/U_GRX/FB
add wave -noupdate /uprogcpu_tb/uut/U_GRX/TB_Grx
add wave -noupdate /uprogcpu_tb/uut/U_GRX/inData
add wave -noupdate /uprogcpu_tb/uut/U_GRX/utData
add wave -noupdate /uprogcpu_tb/uut/U0/uAddr
add wave -noupdate /uprogcpu_tb/uut/U0/uData
add wave -noupdate /uprogcpu_tb/uut/U0/u_mem
add wave -noupdate /uprogcpu_tb/uut/U1/clk
add wave -noupdate /uprogcpu_tb/uut/U1/pAddr
add wave -noupdate /uprogcpu_tb/uut/U1/pData_in
add wave -noupdate /uprogcpu_tb/uut/U1/pData_out
add wave -noupdate /uprogcpu_tb/uut/U1/pFb
add wave -noupdate /uprogcpu_tb/uut/U1/vga_addr
add wave -noupdate /uprogcpu_tb/uut/U1/vga_data_out
add wave -noupdate /uprogcpu_tb/uut/U1/TM
add wave -noupdate /uprogcpu_tb/uut/U1/DM
add wave -noupdate /uprogcpu_tb/uut/U2/clk
add wave -noupdate /uprogcpu_tb/uut/U2/rst_kbd_enc
add wave -noupdate /uprogcpu_tb/uut/U2/PS2KeyboardCLK
add wave -noupdate /uprogcpu_tb/uut/U2/PS2KeyboardData
add wave -noupdate /uprogcpu_tb/uut/U2/ScanCode
add wave -noupdate /uprogcpu_tb/uut/U2/MAKE_op
add wave -noupdate /uprogcpu_tb/uut/U2/PS2Clk
add wave -noupdate /uprogcpu_tb/uut/U2/PS2Data
add wave -noupdate /uprogcpu_tb/uut/U2/PS2Clk_Q1
add wave -noupdate /uprogcpu_tb/uut/U2/PS2Clk_Q2
add wave -noupdate /uprogcpu_tb/uut/U2/PS2Clk_op
add wave -noupdate /uprogcpu_tb/uut/U2/PS2Data_sr
add wave -noupdate /uprogcpu_tb/uut/U2/ScanCode_int
add wave -noupdate /uprogcpu_tb/uut/U2/PS2BitCounter
add wave -noupdate /uprogcpu_tb/uut/U2/BC11
add wave -noupdate /uprogcpu_tb/uut/U2/PS2state
add wave -noupdate /uprogcpu_tb/uut/U3/clk
add wave -noupdate /uprogcpu_tb/uut/U3/rst_kbd_mngr
add wave -noupdate /uprogcpu_tb/uut/U3/ScanCode
add wave -noupdate /uprogcpu_tb/uut/U3/make_op
add wave -noupdate /uprogcpu_tb/uut/U3/player0
add wave -noupdate /uprogcpu_tb/uut/U3/player1
add wave -noupdate /uprogcpu_tb/uut/U3/we
add wave -noupdate /uprogcpu_tb/uut/U3/Wrst_kbd_mngrate
add wave -noupdate /uprogcpu_tb/uut/U3/player0_int
add wave -noupdate /uprogcpu_tb/uut/U3/player1_int
add wave -noupdate /uprogcpu_tb/uut/U5/clk
add wave -noupdate /uprogcpu_tb/uut/U5/VR_data
add wave -noupdate /uprogcpu_tb/uut/U5/VR_addr
add wave -noupdate /uprogcpu_tb/uut/U5/clear
add wave -noupdate /uprogcpu_tb/uut/U5/vgaRed
add wave -noupdate /uprogcpu_tb/uut/U5/vgaGreen
add wave -noupdate /uprogcpu_tb/uut/U5/vgaBlue
add wave -noupdate /uprogcpu_tb/uut/U5/Hsync
add wave -noupdate /uprogcpu_tb/uut/U5/Vsync
add wave -noupdate /uprogcpu_tb/uut/U5/Xpixel1
add wave -noupdate /uprogcpu_tb/uut/U5/Xpixel2
add wave -noupdate /uprogcpu_tb/uut/U5/Ypixel1
add wave -noupdate /uprogcpu_tb/uut/U5/Ypixel2
add wave -noupdate /uprogcpu_tb/uut/U5/ClkDiv
add wave -noupdate /uprogcpu_tb/uut/U5/Clk25
add wave -noupdate /uprogcpu_tb/uut/U5/TR_addr
add wave -noupdate /uprogcpu_tb/uut/U5/TR_data
add wave -noupdate /uprogcpu_tb/uut/U5/blank1
add wave -noupdate /uprogcpu_tb/uut/U5/blank2
add wave -noupdate /uprogcpu_tb/uut/U5/blank
add wave -noupdate /uprogcpu_tb/uut/U5/Hsync1
add wave -noupdate /uprogcpu_tb/uut/U5/Hsync2
add wave -noupdate /uprogcpu_tb/uut/U5/Vsync1
add wave -noupdate /uprogcpu_tb/uut/U5/Vsync2
add wave -noupdate /uprogcpu_tb/uut/U5/U0b/clk
add wave -noupdate /uprogcpu_tb/uut/U5/U0b/addr
add wave -noupdate /uprogcpu_tb/uut/U5/U0b/data
add wave -noupdate /uprogcpu_tb/uut/u6/clk
add wave -noupdate /uprogcpu_tb/uut/u6/rst_piezo
add wave -noupdate /uprogcpu_tb/uut/u6/piezoData
add wave -noupdate /uprogcpu_tb/uut/u6/piezoOut
add wave -noupdate /uprogcpu_tb/uut/u6/counter
add wave -noupdate /uprogcpu_tb/uut/u6/toggle
add wave -noupdate /uprogcpu_tb/uut/u6/durationCounter
add wave -noupdate /uprogcpu_tb/uut/u6/is_playing
add wave -noupdate /uprogcpu_tb/uut/u6/stored_freq
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {225 ns} 0}
quietly wave cursor active 1
configure wave -namecolwidth 150
configure wave -valuecolwidth 100
configure wave -justifyvalue left
configure wave -signalnamewidth 1
configure wave -snapdistance 10
configure wave -datasetprefix 0
configure wave -rowmargin 4
configure wave -childrowmargin 2
configure wave -gridoffset 0
configure wave -gridperiod 1
configure wave -griddelta 40
configure wave -timeline 0
configure wave -timelineunits ns
update
WaveRestoreZoom {0 ns} {110 ns}
