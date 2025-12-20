# README for the VHDL Project

This project includes the following files and their purposes:

## Files in the project:
- **VHD**: Contains the VHDL files for the project.
  - `uprogCPU.vhd` - Top module for the project.
  - `Mem/uMem.vhd` - Memory module for the project.
  - `Mem/pMem.vhd` - Program memory module for the project.

- **XDC**: Contains the design constraints for the project.
  - `Basys3.xdc` - Xilinx Design Constraints (XDC) file specific to the Basys3 FPGA board.

- **TBF**: Contains the testbench file for simulating the design.
  - `uprogCPU_tb.vhd` - Testbench for the top module `uprogCPU.vhd`.

## Makefile utilities:
This project includes Makefile utilities to facilitate the build process. The utilities are included from the following path:

```makefile
include /courses/TSEA83/bin/util.mk
```
Make sure that the `util.mk` file is accessible at the specified path when running the make commands.

## Usage

- Compile the VHDL files.
- Run the testbench to simulate the design.
- Implement the design on the Basys3 FPGA using the provided constraints.

For more details on how to run the project and use the Makefile, refer to the instructions provided in your course materials.

This README outlines the files and the structure of the project, providing a clear overview of the VHDL files, constraints, testbench, and utilities involved.
