class Instruction:
    def __init__(self, opcode, mnemonic, operand_format, flags, description):
        self.opcode = opcode
        self.mnemonic = mnemonic
        self.operand_format = operand_format
        self.flags = flags
        self.description = description

# Addressing modes
class AddressingMode:
    IMMEDIATE = 1  # Value is directly specified
    DIRECT = 0     # Value is at the memory address
    ABSOLUTE = 2   # Full 16-bit address in next word
    INDEXED = 3    # Value is at the memory address + offset

# Define instruction set
instruction_set = {
    # Opcode: Instruction(opcode, mnemonic, operand_format, flags, description)
    0x00: Instruction(0x00, "LDI", ["GRx", "VALUE"], None, "Load immediate value into register"),
    0x01: Instruction(0x01, "LD", ["GRx", "M", "ADR"], None, "Load from memory into register"),
    0x02: Instruction(0x02, "STORE", ["GRx", "M", "ADR"], None, "Store register to memory"),
    0x03: Instruction(0x03, "ADD", ["GRx", "M", "ADR"], ["Z", "N", "C", "V"], "Add memory to register"),
    0x04: Instruction(0x04, "SUB", ["GRx", "M", "ADR"], ["Z", "N", "C", "V"], "Subtract memory from register"),
    0x05: Instruction(0x05, "OR", ["GRx", "M", "ADR"], ["Z", "N"], "Bitwise OR with memory"),
    0x06: Instruction(0x06, "LSR", ["GRx"], ["Z", "N", "C"], "Logical shift right register"),
    0x07: Instruction(0x07, "CMP", ["GRx", "M", "ADR"], ["Z", "N", "C", "V"], "Compare register with memory"),
    0x08: Instruction(0x08, "BRA", ["M", "ADR"], None, "Branch always to address"),
    0x09: Instruction(0x09, "BEQ", ["M", "ADR"], None, "Branch if Z=1"),
    0x0A: Instruction(0x0A, "AND", ["GRx", "M", "ADR"], ["Z", "N"], "Bitwise AND memory with register"),
    0x0B: Instruction(0x0B, "BPL", ["M", "ADR"], None, "Branch if N=0"),
    0x0C: Instruction(0x0C, "KEY0", ["GRx"], None, "Read key input from player 0"),
    0x0D: Instruction(0x0D, "PLAY", ["M", "ADR"], None, "Play piezo sound"),
    0x0E: Instruction(0x0E, "UART", ["GRx", "M", "ADR"], None, "Read/write UART data"),
    0x0F: Instruction(0x0F, "KEY1", ["GRx"], None, "Read key input from player 1")
}

class CPUCompiler:
    def __init__(self):
        self.instruction_set = instruction_set
        self.labels = {}
        self.binary_output = []
        
    def parse_line(self, line):
        """Parse a single line of assembly code"""
        if ';' in line:
            line = line[:line.index(';')]
        line = line.strip()
        
        result = None
        if ':' in line:
            label, rest = line.split(':', 1)
            line = rest.strip()
        
        if not line:
            return None
            
        if line.startswith('.'):
            parts = line.split(maxsplit=1)
            directive = parts[0].lower()
            value = parts[1] if len(parts) > 1 else ""
            
            return {
                'type': 'directive',
                'directive': directive,
                'value': value
            }
        
        parts = line.split()
        mnemonic = parts[0].upper()
        operands = []
        
        if len(parts) > 1:
            operands = [op.strip() for op in ' '.join(parts[1:]).split(',')]
        
        return {
            'type': 'instruction',
            'mnemonic': mnemonic,
            'operands': operands
        }

    def process_directive(self, directive, value):
        """Process an assembly directive"""
        if directive == '.byte':
            try:
                byte_value = self._parse_value(value)
                return byte_value & 0xFFFF  
            except ValueError:
                raise ValueError(f"Invalid value for .byte directive: {value}")
        else:
            raise ValueError(f"Unknown directive: {directive}")
    
    def determine_addressing_mode(self, operand):
        """Determine the addressing mode for an operand"""
        if operand.startswith('#'):  
            return AddressingMode.IMMEDIATE
        elif '[' in operand and ']' in operand:  
            return AddressingMode.INDEXED
        else:  
            return AddressingMode.DIRECT
            
    def _parse_register(self, reg_str):
        """Parse a register reference (R0, R1, R2, R3)"""
        reg_str = reg_str.upper().strip()
        if reg_str in ["R0", "GR0"]:
            return 0
        elif reg_str in ["R1", "GR1"]:
            return 1
        elif reg_str in ["R2", "GR2"]:
            return 2
        elif reg_str in ["R3", "GR3"]:
            return 3
        else:
            raise ValueError(f"Invalid register: {reg_str}")

    def _parse_value(self, value_str):
        """Parse an immediate value (#123, $AB, etc.)"""
        value_str = value_str.strip()
        
        if value_str.startswith('#'):
            value_str = value_str[1:]
        
        if value_str.startswith('0x') or value_str.startswith('$'):
            if value_str.startswith('$'):
                value_str = '0x' + value_str[1:]
            return int(value_str, 16)
        else:
            return int(value_str)

    def _parse_address(self, addr_operands):
        """Parse memory address operands"""
        if len(addr_operands) == 1:
            operand = addr_operands[0].strip()
        else:
            operand = ' '.join(addr_operands).strip()
        
        addr_mode = self.determine_addressing_mode(operand)
        
        if addr_mode == AddressingMode.IMMEDIATE: 
            value = self._parse_value(operand)
        elif addr_mode == AddressingMode.INDEXED:  
            content = operand[operand.index('[')+1:operand.index(']')]
            parts = content.split('+')
            base_reg = self._parse_register(parts[0])
            offset = int(parts[1]) if len(parts) > 1 else 0
            value = offset  
        else:  # DIRECT
            if operand in self.labels:
                value = self.labels[operand]
                # Check if address is too large for direct addressing
                if value > 255:
                    addr_mode = AddressingMode.ABSOLUTE
            else:
                try:
                    value = self._parse_value(operand)
                    # Check if address is too large for direct addressing
                    if value > 255:
                        addr_mode = AddressingMode.ABSOLUTE
                except ValueError:
                    value = 0xFF  
        
        return addr_mode, value
    
    def encode_instruction(self, parsed):
        if parsed.get('type') == 'directive':
            return self.process_directive(parsed['directive'], parsed['value'])
        
        mnemonic = parsed['mnemonic']
        operands = parsed['operands']
        
        matching_inst = None
        for opcode, inst in self.instruction_set.items():
            if inst.mnemonic == mnemonic:
                matching_inst = inst
                break
        
        if not matching_inst:
            raise ValueError(f"Unknown instruction: {mnemonic}")
        
        instruction_word = matching_inst.opcode << 12
        
        if mnemonic == "LDI":  # 0x00: Load immediate
            reg_num = self._parse_register(operands[0])
            value = self._parse_value(operands[1])
            
            instruction_word |= (reg_num << 10)  
            instruction_word |= (value & 0xFF)   
        
        elif mnemonic == "LD":  # 0x01: Load from memory
            reg_num = self._parse_register(operands[0])
            addr_mode, addr_value = self._parse_address(operands[1:])
            
            # For absolute mode, we'll handle this separately in compile_file
            if addr_mode == AddressingMode.ABSOLUTE:
                raise ValueError("Absolute addressing mode should be handled in compile_file")
            
            instruction_word |= (reg_num << 10)     
            instruction_word |= (addr_mode << 8)    
            instruction_word |= (addr_value & 0xFF) 
        
        elif mnemonic == "STORE":  # 0x02: Store to memory
            reg_num = self._parse_register(operands[0])
            addr_mode, addr_value = self._parse_address(operands[1:])
            
            # For absolute mode, we'll handle this separately in compile_file
            if addr_mode == AddressingMode.ABSOLUTE:
                raise ValueError("Absolute addressing mode should be handled in compile_file")
            
            instruction_word |= (reg_num << 10)     
            instruction_word |= (addr_mode << 8)    
            instruction_word |= (addr_value & 0xFF) 
        
        elif mnemonic == "ADD":  # 0x03: Add memory to register
            reg_num = self._parse_register(operands[0])
            addr_mode, addr_value = self._parse_address(operands[1:])
            
            # For absolute mode, we'll handle this separately in compile_file
            if addr_mode == AddressingMode.ABSOLUTE:
                raise ValueError("Absolute addressing mode should be handled in compile_file")
            
            instruction_word |= (reg_num << 10)     
            instruction_word |= (addr_mode << 8)    
            instruction_word |= (addr_value & 0xFF) 
        
        elif mnemonic == "SUB":  # 0x04: Subtract memory from register
            reg_num = self._parse_register(operands[0])
            addr_mode, addr_value = self._parse_address(operands[1:])
            
            # For absolute mode, we'll handle this separately in compile_file
            if addr_mode == AddressingMode.ABSOLUTE:
                raise ValueError("Absolute addressing mode should be handled in compile_file")
            
            instruction_word |= (reg_num << 10)     
            instruction_word |= (addr_mode << 8)    
            instruction_word |= (addr_value & 0xFF) 
        
        elif mnemonic == "OR":  # 0x05: Bitwise OR
            reg_num = self._parse_register(operands[0])
            addr_mode, addr_value = self._parse_address(operands[1:])
            
            # For absolute mode, we'll handle this separately in compile_file
            if addr_mode == AddressingMode.ABSOLUTE:
                raise ValueError("Absolute addressing mode should be handled in compile_file")
            
            instruction_word |= (reg_num << 10)     
            instruction_word |= (addr_mode << 8)    
            instruction_word |= (addr_value & 0xFF) 
        
        elif mnemonic == "LSR":  # 0x06: Logical shift right
            reg_num = self._parse_register(operands[0])
            instruction_word |= (reg_num << 10)     # Set register number
        
        elif mnemonic == "CMP":  # 0x07: Compare
            reg_num = self._parse_register(operands[0])
            addr_mode, addr_value = self._parse_address(operands[1:])
            
            # For absolute mode, we'll handle this separately in compile_file
            if addr_mode == AddressingMode.ABSOLUTE:
                raise ValueError("Absolute addressing mode should be handled in compile_file")
            
            instruction_word |= (reg_num << 10)     
            instruction_word |= (addr_mode << 8)    
            instruction_word |= (addr_value & 0xFF) 
        
        elif mnemonic in ["BRA", "BEQ", "BPL"]:  # Branch instructions
            raise ValueError("Branch instructions should be handled separately.")
        
        elif mnemonic == "AND":  # 0x0A: AND memory with register
            reg_num = self._parse_register(operands[0])
            addr_mode, addr_value = self._parse_address(operands[1:])
            
            # For absolute mode, we'll handle this separately in compile_file
            if addr_mode == AddressingMode.ABSOLUTE:
                raise ValueError("Absolute addressing mode should be handled in compile_file")
            
            instruction_word |= (reg_num << 10)     # Set register number
            instruction_word |= (addr_mode << 8)    # Set addressing mode
            instruction_word |= (addr_value & 0xFF) # Set address/value
        
        elif mnemonic == "KEY0":  # 0x0C: Read key input from player 0
            reg_num = self._parse_register(operands[0])
            instruction_word |= (reg_num << 10)
        
        elif mnemonic == "KEY1":  # 0x0F: Read key input from player 1
            reg_num = self._parse_register(operands[0])
            instruction_word |= (reg_num << 10)
        
        elif mnemonic == "PLAY":  # 0x0D: Play sound
            addr_mode, addr_value = self._parse_address(operands)
            
            # For absolute mode, we'll handle this separately in compile_file
            if addr_mode == AddressingMode.ABSOLUTE:
                raise ValueError("Absolute addressing mode should be handled in compile_file")
            
            instruction_word |= (addr_mode << 8)    
            instruction_word |= (addr_value & 0xFF) 
        
        elif mnemonic == "UART":  # 0x0E: UART I/O
            reg_num = self._parse_register(operands[0])
            addr_mode, addr_value = self._parse_address(operands[1:])
            
            # For absolute mode, we'll handle this separately in compile_file
            if addr_mode == AddressingMode.ABSOLUTE:
                raise ValueError("Absolute addressing mode should be handled in compile_file")
            
            instruction_word |= (reg_num << 10)     
            instruction_word |= (addr_mode << 8)    
            instruction_word |= (addr_value & 0xFF) 
        
        return instruction_word
    
    def compile_file(self, filename):
        """Compile an assembly file to machine code"""
        with open(filename, 'r') as f:
            lines = f.readlines()
        
        # First pass: collect labels with approximate addresses
        # We'll assume all memory references use single-word format initially
        self.binary_output = []
        self.labels = {}
        current_pos = 0
        
        # Instructions that could need two words when using absolute addressing
        memory_instructions = ["LD", "STORE", "ADD", "SUB", "OR", "LSR", "CMP", "PLAY", "UART"]
        # Branch instructions always use two words
        branch_instructions = ["BRA", "BEQ", "BPL"]
        
        for line in lines:
            line = line.strip()
            if not line or line.startswith(';'):
                continue
                    
            # Handle labels first
            if ':' in line:
                label_name = line[:line.index(':')].strip()
                self.labels[label_name] = current_pos
                line = line[line.index(':')+1:].strip()
            
            # Only increment position if there's an actual instruction or directive
            if line and not line.startswith(';'):
                parsed = self.parse_line(line)
                if not parsed:
                    continue
                    
                if parsed.get('type') == 'instruction':
                    mnemonic = parsed['mnemonic']
                    
                    # Branch instructions always take 2 words
                    if mnemonic in branch_instructions:
                        current_pos += 2
                    else:
                        current_pos += 1  # Initially assume 1 word for other instructions
                else:
                    current_pos += 1  # Directive
        
        # Second pass: Fix label addresses based on instruction sizes
        # This is crucial - we need to iterate multiple times until addresses stabilize
        labels_changed = True
        max_iterations = 5  # Prevent infinite loops
        iteration = 0
        
        while labels_changed and iteration < max_iterations:
            labels_changed = False
            iteration += 1
            current_pos = 0
            old_labels = self.labels.copy()
            
            for line in lines:
                line = line.strip()
                if not line or line.startswith(';'):
                    continue
                        
                # Update label position
                if ':' in line:
                    label_name = line[:line.index(':')].strip()
                    if self.labels[label_name] != current_pos:
                        self.labels[label_name] = current_pos
                        labels_changed = True
                    line = line[line.index(':')+1:].strip()
                
                if not line or line.startswith(';'):
                    continue
                    
                parsed = self.parse_line(line)
                if not parsed:
                    continue
                    
                if parsed.get('type') == 'instruction':
                    mnemonic = parsed['mnemonic']
                    
                    # Branch instructions always take 2 words
                    if mnemonic in branch_instructions:
                        current_pos += 2
                    # For memory instructions, check for absolute addressing
                    elif mnemonic in memory_instructions:
                        needs_absolute = False
                        
                        # Get the address operand
                        if len(parsed['operands']) > 1:
                            addr_operand = parsed['operands'][1]
                        else:
                            addr_operand = parsed['operands'][0]  # For PLAY
                            
                        # Check if operand is a label requiring absolute addressing
                        if addr_operand in self.labels:
                            target_addr = self.labels[addr_operand]
                            if target_addr > 255:
                                needs_absolute = True
                                current_pos += 2  # Two words for absolute
                            else:
                                current_pos += 1  # One word for direct
                        # Check if it's a numeric address > 255
                        elif not addr_operand.startswith('#') and not ('[' in addr_operand and ']' in addr_operand):
                            try:
                                value = self._parse_value(addr_operand)
                                if value > 255:
                                    needs_absolute = True
                                    current_pos += 2
                                else:
                                    current_pos += 1
                            except ValueError:
                                current_pos += 1  # Assume direct addressing if we can't parse
                        else:
                            current_pos += 1  # One word for other addressing modes
                    else:
                        current_pos += 1  # Single word instructions
                else:
                    current_pos += 1  # Directive
        
        # Now we have accurate label addresses - third pass: generate actual code
        self.binary_output = []
        current_pos = 0
        
        for line in lines:
            parsed = self.parse_line(line)
            if not parsed:
                continue

            if parsed.get('type') == 'instruction':
                mnemonic = parsed['mnemonic']
                
                # Get opcode for this instruction
                opcode = None
                for op, inst in self.instruction_set.items():
                    if inst.mnemonic == mnemonic:
                        opcode = op
                        break
                
                if opcode is None:
                    raise ValueError(f"Unknown instruction: {mnemonic}")
                
                # Handle branch instructions
                if mnemonic in branch_instructions:
                    # First word: Branch instruction with immediate mode
                    instruction_word = (opcode << 12) | (AddressingMode.IMMEDIATE << 8)
                    self.binary_output.append(instruction_word)
                    
                    # Second word: Target address
                    target_label = parsed['operands'][0]
                    if target_label in self.labels:
                        target_addr = self.labels[target_label]
                    else:
                        try:
                            target_addr = self._parse_value(target_label)
                        except ValueError:
                            raise ValueError(f"Unknown label or value: {target_label}")
                    
                    self.binary_output.append(target_addr & 0xFFFF)
                    current_pos += 2
                
                # Handle memory-reference instructions
                elif mnemonic in memory_instructions:
                    # Skip LSR since it's register-only now
                    if mnemonic == "LSR":
                        reg_num = self._parse_register(parsed['operands'][0])
                        instruction_word = (opcode << 12) | (reg_num << 10)
                        self.binary_output.append(instruction_word)
                        current_pos += 1
                        continue

                    # Handle other memory instructions
                    if mnemonic != "PLAY":
                        reg_num = self._parse_register(parsed['operands'][0])
                        addr_operand = parsed['operands'][1]
                    else:
                        reg_num = 0  # Default for PLAY
                        addr_operand = parsed['operands'][0]
                    
                    target_addr = 0
                    needs_absolute = False
                    
                    # Check if operand is a label
                    if addr_operand in self.labels:
                        target_addr = self.labels[addr_operand]
                        if target_addr > 255:
                            needs_absolute = True
                    # Otherwise try to parse as direct value
                    elif not addr_operand.startswith('#') and not ('[' in addr_operand and ']' in addr_operand):
                        try:
                            target_addr = self._parse_value(addr_operand)
                            if target_addr > 255:
                                needs_absolute = True
                        except ValueError:
                            pass
                    
                    if needs_absolute:
                        # First word: instruction with register and absolute addressing mode
                        instruction_word = (opcode << 12) | (reg_num << 10) | (AddressingMode.ABSOLUTE << 8)
                        self.binary_output.append(instruction_word)
                        
                        # Second word: full 16-bit address
                        self.binary_output.append(target_addr & 0xFFFF)
                        current_pos += 2
                    else:
                        # Use standard encoding for direct/immediate/indexed addressing
                        try:
                            addr_mode = self.determine_addressing_mode(addr_operand)
                            if addr_operand in self.labels:
                                addr_value = self.labels[addr_operand]
                            else:
                                try:
                                    addr_value = self._parse_value(addr_operand)
                                except ValueError:
                                    addr_value = 0
                                    
                            instruction_word = (opcode << 12) | (reg_num << 10) | (addr_mode << 8) | (addr_value & 0xFF)
                            self.binary_output.append(instruction_word)
                            current_pos += 1
                        except ValueError as e:
                            raise ValueError(f"Error encoding instruction {mnemonic} {', '.join(parsed['operands'])}: {e}")
                
                # Handle other instructions
                else:
                    binary_instruction = self.encode_instruction(parsed)
                    self.binary_output.append(binary_instruction)
                    current_pos += 1
            else:
                # Handle directives
                binary_instruction = self.encode_instruction(parsed)
                self.binary_output.append(binary_instruction)
                current_pos += 1
        
        return True
        
    def encode_branch_instruction(self, parsed):
        """Special handling for branch instructions"""
        mnemonic = parsed['mnemonic']
        
        # Get the opcode from instruction set
        for opcode, inst in self.instruction_set.items():
            if inst.mnemonic == mnemonic:
                instruction_word = opcode << 12
                break
        
        # Always use immediate addressing mode for branches
        instruction_word |= (AddressingMode.IMMEDIATE << 8)
        return instruction_word

    def write_output(self, filename):
        """Write compiled binary to files"""
        bin_filename = filename + ".bin"
        with open(bin_filename, 'w') as f:
            for instruction in self.binary_output:
                binary_str = format(instruction, '016b')  
                f.write(binary_str + '\n')
        
        # We need to re-read the input file to get the original assembly lines
        input_filename = filename + ".asm"
        assembly_lines = []
        try:
            with open(input_filename, 'r') as f:
                assembly_lines = [line.strip() for line in f.readlines()]
        except FileNotFoundError:
            # If we can't find the original file, just continue without the comments
            print(f"Warning: Couldn't find {input_filename} to add assembly comments")
        
        vhdl_filename = filename + ".vhd"
        with open(vhdl_filename, 'w') as f:
            # Add instruction set documentation as VHDL comment
            f.write("-- Instruction Set Reference:\n")
            f.write("-- Opcode | Mnemonic | Description\n")
            f.write("-- ----------------------------\n")
            for opcode, inst in sorted(self.instruction_set.items()):
                f.write(f"-- 0x{opcode:02X}   | {inst.mnemonic:<6} | {inst.description}\n")
            f.write("\n")
            
            # Create a mapping between assembly lines and compiled instructions
            current_asm_line = 0
            instruction_to_asm = {}
            instruction_index = 0
            
            while current_asm_line < len(assembly_lines) and instruction_index < len(self.binary_output):
                line = assembly_lines[current_asm_line].strip()
                current_asm_line += 1
                
                # Skip empty lines and comments
                if not line or line.startswith(';'):
                    continue
                    
                parsed = self.parse_line(line)
                if not parsed:
                    continue
                    
                # Get the original assembly instruction with any comments
                clean_line = line
                if ';' in clean_line:
                    clean_line = clean_line[:clean_line.index(';')].strip()
                    
                # Map the instruction index to the assembly line
                if parsed.get('type') == 'instruction':
                    mnemonic = parsed['mnemonic']
                    # Branch instructions and absolute addressing take two words
                    if mnemonic in ["BRA", "BEQ", "BPL"]:
                        instruction_to_asm[instruction_index] = clean_line
                        instruction_index += 2
                    elif mnemonic in ["LD", "STORE", "ADD", "SUB", "OR", "CMP", "PLAY", "UART"]:
                        # Check if we used absolute addressing (this is simplified)
                        # You might need more complex logic based on your compiler
                        if instruction_index + 1 < len(self.binary_output):
                            # Check if next value isn't an instruction (crude heuristic)
                            next_val = self.binary_output[instruction_index + 1]
                            if (next_val & 0xF000) == 0:  # Not an instruction
                                instruction_to_asm[instruction_index] = clean_line
                                instruction_index += 2
                                continue
                        
                        instruction_to_asm[instruction_index] = clean_line
                        instruction_index += 1
                    else:
                        instruction_to_asm[instruction_index] = clean_line
                        instruction_index += 1
                else:
                    # Directive takes one word
                    instruction_to_asm[instruction_index] = clean_line
                    instruction_index += 1
            
            # Write the actual ROM data with assembly comments
            f.write("(\n")
            for i, instruction in enumerate(self.binary_output):
                hex_str = format(instruction, '04X')
                line = f"   x\"{hex_str}\""
                if i < len(self.binary_output) - 1:
                    line += ","
                    
                # Add the original assembly as a comment if available
                if i in instruction_to_asm:
                    line = f"{line:<20} -- {instruction_to_asm[i]}"
                
                f.write(line + '\n')
            f.write(");")
        
        print(f"Compiled {len(self.binary_output)} instructions.")
        print(f"Binary output written to: {bin_filename}")
        print(f"VHDL output written to: {vhdl_filename}")

if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python compiler.py input.asm [output]")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else input_file.rsplit('.', 1)[0]
    
    compiler = CPUCompiler()
    if compiler.compile_file(input_file):
        compiler.write_output(output_file)
        print(f"Compilation successful: {input_file} -> {output_file}.bin and {output_file}.vhd")
    else:
        print("Compilation failed.")
        sys.exit(1)