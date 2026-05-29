# Dual-Factor Hardware Security System

An advanced, bare-metal C security system built for the ATmega328PB microcontroller. Designed to showcase low-level hardware control, this project implements a dual-factor authentication mechanism requiring both a local physical keypad entry and a remote serial password to actuate a servo-driven locking mechanism.

Developed using **Microchip Studio** for ECE 412 — Microcontrollers at the University of Louisville (Spring 2025).

## Key Features

- **Dual-Factor Authentication:** Requires a 4-digit PIN entered via a matrix keypad followed by a string password over a UART serial connection.
- **Bare-Metal Hardware Control:** Direct manipulation of AVR registers without reliance on high-level hardware abstraction libraries (like Arduino).
- **Matrix Keypad Scanning & Software Debounce:** Implements a custom scanning algorithm for a 4x4 keypad with a 30ms software debounce mechanism to ensure reliable input.
- **Hardware PWM Servo Actuation:** Utilizes Timer1 Fast PWM to generate precise control signals for a servo motor, transitioning between "locked" and "unlocked" states.
- **Interrupt-Driven State Management:** Leverages Timer2 and Interrupt Service Routines (ISRs) for asynchronous system tasks, such as a heartbeat status LED.
- **UART Serial Interface:** Full serial communication handling (9600 baud) for the second authentication factor and real-time user prompts/feedback.

## Technical Details

This project was built to demonstrate proficiency in embedded systems programming and hardware-software interfacing.

### Software Stack
- **Environment:** Microchip Studio (formerly Atmel Studio)
- **Language:** Bare-Metal C (AVR-GCC)
- **Clock Speed:** 16 MHz

### Hardware Architecture & Pinout
The system is built around the **ATmega328PB** microcontroller. The following pin mappings are utilized:

| Component | Microcontroller Pin | Notes |
| :--- | :--- | :--- |
| **Keypad Rows** | `PD2`, `PD3`, `PD4`, `PD5` | Configured as outputs, pulled low sequentially during scanning. |
| **Keypad Columns** | `PD6`, `PD7`, `PB0`, `PB1` | Configured as inputs with internal pull-ups to detect row intersections. |
| **Servo PWM Signal** | `PB2` (OC1B) | Driven by **Timer1 Fast PWM**. Period is set via `ICR1` (20ms/50Hz), and duty cycle is controlled via `OCR1B` (1ms to 2ms pulses). |
| **Status LED** | `PB5` | Toggled every 500ms by the **Timer2 Compare Match ISR** to indicate system activity. |
| **UART Interface** | `PD0` (RX), `PD1` (TX) | Serial communication operating at 9600 baud. |

### Core Implementations

1. **Timer1 Fast PWM (Servo Control):** Timer1 is configured in Mode 14 (Fast PWM, TOP=ICR1) with a prescaler of 8 to generate a 50Hz (20ms) signal. The `OCR1B` register dynamically adjusts the pulse width to actuate the servo: 1000 µs for the "locked" position and 2000 µs for the "unlocked" position.
2. **Timer2 Interrupts (Heartbeat):** Timer2 is set up in CTC mode with a prescaler of 64. The Output Compare A Match Interrupt (`TIMER2_COMPA_vect`) fires every 1ms, ticking a millisecond counter used for an auto-lock delay (5 seconds) and toggling the status LED every 500ms.
3. **UART Communication:** The baud rate is configured using the `UBRR0` register for 9600 bps. Functions like `uart_putchar` and `uart_getchar` manage character transmission and reception by polling the `UCSR0A` status register.

## Getting Started

### Prerequisites
- **Microchip Studio** installed on a Windows machine.
- An **ATmega328PB** development board (or custom PCB).
- A flashing tool such as an **Atmel-ICE**, **AVRISP mkII**, or a compatible bootloader tool.
- A terminal emulator program (e.g., PuTTY, Tera Term) to interface with the serial port.

### Installation & Flashing
1. **Clone the repository:**
   ```bash
   git clone https://github.com/your-username/your-repo-name.git
   ```
2. **Open the Project:**
   Navigate to the cloned directory and open the `GccApplication1.atsln` solution file in Microchip Studio.
3. **Build the Solution:**
   Press `F7` or navigate to `Build -> Build Solution` to compile the bare-metal C code into a `.hex` file.
4. **Flash the Microcontroller:**
   Connect your programmer to the board, open the `Device Programming` dialog in Microchip Studio (`Ctrl+Shift+P`), select your tool/device/interface, and program the `.hex` file to the ATmega328PB's flash memory.

## Usage Examples

Once the board is flashed and powered, connect your terminal emulator to the board's COM port at **9600 baud**.

By default, the credentials are:
- **Keypad Code:** `1111`
- **Serial Password:** `Password`

**Standard Unlock Flow:**
1. The terminal displays:
   `Enter 4-digit code:`
2. Press `1`, `1`, `1`, `1` on the physical 4x4 matrix keypad. As you press, the characters will echo to the serial terminal.
3. The terminal displays:
   `Code OK. Enter password:`
4. Type `Password` into the terminal emulator and press `Enter`.
5. The terminal displays:
   `Access granted. Unlocking...`
6. The servo actuates to the unlocked position (2000 µs pulse width).
7. After a 5-second delay managed by the Timer2 ISR, the system auto-locks, moving the servo back to the locked position (1000 µs pulse width), and prints:
   `Locked.`
   `Enter 4-digit code:`

## Contributing

While this is primarily a personal portfolio project demonstrating bare-metal embedded systems engineering, suggestions, feedback, and contributions are always welcome. Feel free to open an issue or submit a pull request if you see areas for optimization or enhancement.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
