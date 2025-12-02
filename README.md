# 👾 Space Invaders on FPGA 👾
**PS/2 Keyboard + VGA running Space Invaders on a RISC-V SoC (Nexys A7)**

A simple project that integrates a PS/2 keyboard interface with a VGA video driver, running a parallel version of Space Invaders on a RISC-V system-on-chip.

---

## ⚙️ Requirements
- AMD Vivado (for bitstream generation)
- Nexys A7 FPGA board
- PS/2 keyboard
- VGA cable and monitor
- RISC-V cross-compiler (riscv64)
- Serial port access permission (`/dev/ttyUSB1` or similar)

---

## 🚀 How to Run

1. Generate the bistream using Vivado.
2. Program your board.
3. Make sure you have a RISC-V cross-compiler installed.  
4. Add the application `.c` file into `hf-risc/software/app/`.  
5. Add a new rule to the Makefile to compile your application (e.g., `space_invaders`).  

Then open **three terminals**:

---

### 🖥️ Terminal A — Serial Communication with the FPGA
```bash
make serial SERIAL_DEV=/dev/ttyUSB1
cat /dev/ttyUSB1
```

### ⚡ Terminal B — Compilating the application
```bash
make space_invaders F_CLK=50000000
```

### 📤 Terminal C — Upload para a Placa
```bash
make load SERIAL_DEV=/dev/ttyUSB1
```

---

## 🧩 Technical Description

This project integrates three main hardware components to run a simplified version of Space Invaders on the Nexys A7 FPGA:

### 1. RISC-V System-on-Chip (HF-RISC)
The game logic runs on a lightweight RISC-V SoC.  
The CPU executes a C application responsible for:
- reading keyboard input codes,
- updating the player and enemy states,
- handling collisions,
- sending pixel update commands to the VGA module.

The application is compiled using a RISC-V cross-compiler and uploaded through the serial bootloader provided by the HF-RISC environment.

---

### 2. PS/2 Keyboard Interface
A custom Verilog PS/2 driver receives scan codes from the keyboard and converts them into events readable by the RISC-V CPU.

The module performs:
- clock/data synchronization,
- bit shifting of the 11-bit PS/2 packet (start, 8 data bits, parity, stop),
- scan-code decoding,
- interrupt-like signaling to the CPU when a new key is detected.

The current implementation sends repeated key events continuously, which is listed as a known issue.

---

### 3. VGA Video Driver
The VGA controller generates the 640×480@60Hz video signal used to display the game.

Its responsibilities include:
- pixel clock generation,
- horizontal and vertical sync generation,
- active-area pixel addressing,
- rendering of game sprites (player ship, aliens, projectiles).

The RISC-V processor writes game state updates into a memory-mapped framebuffer-like interface, and the VGA module reads these signals in real time to update the display.

---

### 4. System Integration
All modules are connected through the HF-RISC SoC structure:
- The keyboard module interacts with the CPU through memory-mapped registers.
- The VGA controller accesses shared signals describing the game state.
- The CPU runs an infinite loop that updates the game environment based on inputs and timing.

This creates a complete embedded system capable of reading user input, processing a game loop, and rendering graphics—all entirely on the FPGA.



## 🐞 Errors to be fixed
The PS/2 keyboard driver is sending infinite repeat key codes, causing the pressed key to stay active indefinitely.
