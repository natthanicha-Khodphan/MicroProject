# ADAS Simulator v3 (bare-metal STM32F4)

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
