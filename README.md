# ADAS Simulator v3.1 (bare-metal STM32F4)

Nucleo-F4 + STEO Training Shield Rev 02.00

| Input / Output | Pin | Function |
|---|---|---|
| Joystick VRy / VRx | PB1 / PB0 | Gear D/R + throttle |
| Potentiometer | PA4 | Distance to lead vehicle (GAP %) |
| LDR | PA1 | Auto headlight (PA5), calibrated at boot |
| Button 1 | PA10 | Park lock (only when stopped) |
| Button 2 | PB3 | Rain mode (max 120 km/h, earlier braking) |
| Button 3 | PB5 | Red-light mode (lead vehicle departure alert) |
| Button 4 | PB4 | Emergency brake |
| Buzzer HW-512 | PC2 | Alert (drive via NPN transistor, 5V) |
| LED PB6 / PA7 | | Drive / Reverse proximity warning |
| LED PA6 | | Mirrors buzzer |
| OLED SH1106 | PB8 / PB9 (I2C1 400 kHz) | Dashboard |
| UART2 | PA2 / PA3 (115200) | Debug dashboard |

Main loop: TIM2 10 ms tick. Speed model: accel/decel ramps (D 0-200, R 0-30 km/h).

## Peripherals

| Peripheral | Mode | Where |
|---|---|---|
| GPIO | Output: 4 LEDs, buzzer. Input pull-up: 4 buttons, joystick SW | `driver_gpio.c`, `driver_led.c` |
| UART2 TX | DMA1 Stream6 Ch4 + transfer-complete IRQ, 1 KB ring buffer (no polling) | `driver_uart.c` |
| ADC1 | 4-channel scan, DMA2 Stream0 circular (no polling) | `driver_adc.c` |
| EXTI | Lines 3, 4, 5, 10 falling edge (buttons), debounce confirmed on 10 ms tick | `driver_exti.c` |
| TIM2 | Update IRQ every 10 ms (control tick) | `driver_tim.c` |
| I2C1 | 400 kHz, SH1106 OLED | `driver_i2c.c`, `driver_oled.c` |

## Software structure

Application calls driver APIs only; drivers never call application code.

| Layer | Files | Role |
|---|---|---|
| Application | `main.c`, `gear_fsm.c`, `headlight.c`, `safety_brake.c`, `sensor_convert.c`, `util_format.c` | Boot sequence, gear/speed FSM, ADAS logic (AEB, LVDA, rain mode), headlight, dashboard text |
| Driver | `driver_gpio.c`, `driver_led.c`, `driver_buzzer.c`, `driver_uart.c`, `driver_adc.c`, `driver_exti.c`, `driver_tim.c`, `driver_i2c.c`, `driver_oled.c` | Register-level access, ISRs |
| Board config | `board_config.h` | Pin map and tuning constants |

Button flow: EXTI ISR sets an edge flag -> `exti_buttons_scan_tick()` (driver) debounces and returns event bits -> `gear_fsm_tick()` (application) decides what each button does.
