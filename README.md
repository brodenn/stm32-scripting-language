# STM32 Scripting Language Project

This project is a **learning experiment** to build a **simple compiler and virtual machine** for STM32 microcontrollers.

---

## 🎯 Goal

- Learn about **compilers**, **interpreters**, and **virtual machines**  
- Build a **basic scripting language** for embedded systems  
- Expand the language over time with more **commands** and **features**  

> This project is for **education and exploration**—it has **no production use** (yet).

---

## 🚀 How It Works

1. Write your **script** in `compiler/input.txt`.  
2. Run the **compiler** to translate it into **bytecode** (stored in the `program[]` array in `src/program.h`).  
3. The STM32 firmware **interprets** and **executes** the bytecode at runtime, driving GPIOs, UART, and other peripherals.

---

## 📝 Script Commands & Features

### Currently Supported

| Command                          | Description                                 |
|:---------------------------------|:--------------------------------------------|
| `set PAx 0/1`                    | Drive GPIO PAx low or high                  |
| `toggle PAx`                     | Toggle GPIO PAx                             |
| `wait <ms>`                      | Delay for specified milliseconds           |
| `loop N ... endloop`             | Repeat enclosed block N times               |
| `if PC13 == 0 then ... end`      | Conditional on button (B1) state            |
| `print "text\n"`                 | Send string over UART                       |
| `printNum <number>`              | Print a numeric literal                     |
| `printCount`                     | Print and increment an internal counter     |
| `on PC13 falling then ... end`   | Interrupt-driven block on B1 press (debounced) |

### Planned Enhancements

- ✅ Support for **other GPIO ports** (PBx, PCx…)  
- ✅ **Nested** loops and **else** branches  
- ⚙️ **Variables** & **arithmetic** in scripts  
- 🎛️ **PWM**, **ADC/DAC**, **I²C**, **SPI**, **UART** commands for sensors & actuators  
- 🔄 **Function**/subroutine calls in scripts  
- 💾 **Persistent storage** (flash emulation)  
- 📡 **Dynamic script upload** via UART at runtime  

---

## 🛠️ Build & Flash

### 1. Compiler (PC-side)

Compile the script-to-bytecode translator:

```bash
gcc compiler/main.c -o compiler/compiler
```

Run it to convert your `compiler/input.txt` into `src/program.h`:

```bash
./compiler/compiler
```

### 2. Firmware (STM32-side)

Using PlatformIO:

Build the firmware:

```bash
pio run
```

Flash to your board:

```bash
pio run --target upload
```

Monitor the UART output (115200 baud):

```bash
pio device monitor --baud 115200
```