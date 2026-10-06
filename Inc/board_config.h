#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "stm32f411xe.h"

/* STEO Training Shield 1 Rev 02.00 (red PCB) */

#define BOARD_LED_HEADLIGHT_PORT         GPIOA
#define BOARD_LED_HEADLIGHT_PIN          5U
/* Red LED PA6 mirrors buzzer state (visual check of LVDA logic) */
#define BOARD_LED_ALERT_PORT             GPIOA
#define BOARD_LED_ALERT_PIN              6U
#define BOARD_LED_REVERSE_WARN_PORT      GPIOA
#define BOARD_LED_REVERSE_WARN_PIN       7U
#define BOARD_LED_DRIVE_WARN_PORT        GPIOB
#define BOARD_LED_DRIVE_WARN_PIN         6U

/* Buttons are active LOW and raise EXTI on the falling edge.
 * EXTI3 = PB3, EXTI4 = PB4, EXTI9_5 = PB5, EXTI15_10 = PA10:
 * if a pin changes, check the IRQ handlers in driver_exti.c   */
#define BOARD_BTN_PARK_PORT              GPIOA
#define BOARD_BTN_PARK_PIN               10U
#define BOARD_BTN_RAIN_PORT              GPIOB
#define BOARD_BTN_RAIN_PIN               3U
#define BOARD_BTN_REDLIGHT_PORT          GPIOB
#define BOARD_BTN_REDLIGHT_PIN           5U
#define BOARD_BTN_BRAKE_PORT             GPIOB
#define BOARD_BTN_BRAKE_PIN              4U

#define BOARD_POT_PORT                   GPIOA
#define BOARD_POT_PIN                    4U
#define BOARD_POT_ADC_CHANNEL            4U
#define BOARD_POT_DIRECTION_INVERTED     1U

#define BOARD_LDR_PORT                   GPIOA
#define BOARD_LDR_PIN                    1U
#define BOARD_LDR_ADC_CHANNEL            1U
#define BOARD_LDR_DARK_IS_LOW            0U
#define BOARD_LDR_DARK_THRESHOLD         1500U

#define BOARD_JOY_VRX_PORT               GPIOB
#define BOARD_JOY_VRX_PIN                0U
#define BOARD_JOY_VRX_ADC_CHANNEL        8U
#define BOARD_JOY_VRY_PORT               GPIOB
#define BOARD_JOY_VRY_PIN                1U
#define BOARD_JOY_VRY_ADC_CHANNEL        9U
#define BOARD_JOY_FORWARD_IS_VRY         1U
#define BOARD_JOY_SW_PORT                GPIOC
#define BOARD_JOY_SW_PIN                 0U

/* HW-512 (KY-012) active buzzer: HIGH = sound */
#define BOARD_BUZZER_PORT                GPIOC
#define BOARD_BUZZER_PIN                 2U
#define BOARD_BUZZER_ACTIVE_LOW          0U

#define BOARD_OLED_SCL_PORT              GPIOB
#define BOARD_OLED_SCL_PIN               8U
#define BOARD_OLED_SDA_PORT              GPIOB
#define BOARD_OLED_SDA_PIN               9U
#define BOARD_OLED_I2C_AF_NUMBER         4U
#define BOARD_OLED_I2C_ADDRESS           0x3CU

#define BOARD_UART_TX_PORT               GPIOA
#define BOARD_UART_TX_PIN                2U
#define BOARD_UART_RX_PORT               GPIOA
#define BOARD_UART_RX_PIN                3U
#define BOARD_UART_AF_NUMBER             7U
#define BOARD_UART_BRR_VALUE             139U

#define BOARD_TIM2_PRESCALER             15999U
#define BOARD_TIM2_PERIOD                9U

#define ADC_BUF_IDX_POT                  0U
#define ADC_BUF_IDX_LDR                  1U
#define ADC_BUF_IDX_JOY_VRX             2U
#define ADC_BUF_IDX_JOY_VRY             3U
#define ADC_SCAN_CHANNEL_COUNT           4U

#define SPEED_MAX_NORMAL                 200U
#define SPEED_MAX_RAIN                   120U
#define SPEED_MAX_REVERSE                30U
#define ZONE_CAUTION_NORMAL              40U
#define ZONE_CRITICAL_NORMAL             70U
#define ZONE_CAUTION_RAIN                25U
#define ZONE_CRITICAL_RAIN               55U
#define REDLIGHT_BUZZER_DELTA            5U
#define REDLIGHT_ARM_GAP_MIN             90U   /* LVDA armed only if stopped this close (90-100%) */
#define JOY_CENTER_VALUE                 2048U
#define JOY_DEADZONE                     300U

#endif
