# LC-3 Virtual Machine (Windows)

This is a **custom LC-3 Virtual Machine** written in C, designed to run in the Windows terminal/command prompt.  
This project features a **text-based interactive UI** for running LC-3 programs with built-in commands, memory visualization, and register inspection.

---

## ✨ Features

- Works on **Windows** (tested in Command Prompt & PowerShell)
- Ability to load and run LC-3 programs (`.obj` files)
- Includes the classic **2048** game (`2048.obj`) preloaded
- Text-based command interface:
  - `s` — Step through execution
  - `run` — Run continuously until the program halts
  - `print registers` — Display CPU register states
  - `memory visualization` — View LC-3 memory contents
  - `q` — Quit the current program
- Modular — other LC-3 programs can be added by editing the code
- Cleanly handles program exit and multiple runs in one session

---

## 🛠 Building

### **Windows (Command Prompt or PowerShell)**
You need **GCC** installed (e.g., via [MinGW-w64](http://mingw-w64.org/) or [MSYS2](https://www.msys2.org/)).

```bash
# Compile:
gcc lc3vm.c main.c -o vm

# Run:
./vm
```

---

This project was inspired by various LC-3 VM projects, including Build Your Own X’s LC-3 guide, but implemented with my own custom command-line UI and features.
The included 2048.obj is a classic LC-3 version of the game for demonstration.
