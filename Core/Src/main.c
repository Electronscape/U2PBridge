#include "main.h"
#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_hid.h"
#include <stdint.h>

/* ============================================================================
 * Defines & Hardware Mapping
 * ============================================================================ */
#define PS2_CLK_PORT  GPIOB
#define PS2_CLK_PIN   GPIO_PIN_0
#define PS2_DATA_PORT GPIOB
#define PS2_DATA_PIN  GPIO_PIN_1

#define AMIGA_PORT       GPIOA
#define AMIGA_V_PIN      GPIO_PIN_0  // DE-9 pin 1 / joystick up
#define AMIGA_H_PIN      GPIO_PIN_1  // DE-9 pin 2 / joystick down
#define AMIGA_VQ_PIN     GPIO_PIN_2  // DE-9 pin 3 / joystick left
#define AMIGA_HQ_PIN     GPIO_PIN_3  // DE-9 pin 4 / joystick right
#define AMIGA_BTN1_PIN   GPIO_PIN_4  // DE-9 pin 6 / left button
#define AMIGA_BTN2_PIN   GPIO_PIN_5  // DE-9 pin 9 / right button
#define AMIGA_BTN3_PIN   GPIO_PIN_6  // Optional DE-9 pin 5 / middle button
#define AMIGA_OUTPUT_PINS \
    (AMIGA_V_PIN | AMIGA_H_PIN | AMIGA_VQ_PIN | AMIGA_HQ_PIN \
            | AMIGA_BTN1_PIN | AMIGA_BTN2_PIN | AMIGA_BTN3_PIN)

#define SYSTEM_CLOCK_HZ        84000000U
#define PS2_BIT_CLOCK_HZ       200000U
#define PS2_HALF_BIT_CYCLES    (SYSTEM_CLOCK_HZ / (PS2_BIT_CLOCK_HZ * 2U))
#define PS2_INTER_BYTE_GAP_US  20U
#define PS2_INTER_BYTE_CYCLES  ((SYSTEM_CLOCK_HZ / 1000000U) * PS2_INTER_BYTE_GAP_US)
#define AMIGA_X_DIRECTION (1)
#define AMIGA_Y_DIRECTION (1)
#define TIM2_CLOCK_HZ          SYSTEM_CLOCK_HZ
#define REPORT_TIMER_HZ        10000U
#define REPORT_TIMER_PSC       1U
#define REPORT_TIMER_PERIOD \
    ((TIM2_CLOCK_HZ / ((REPORT_TIMER_PSC + 1U) * REPORT_TIMER_HZ)) - 1U)
#define PS2_REPORT_HZ          1000U
#define PS2_REPORT_TICKS       (REPORT_TIMER_HZ / PS2_REPORT_HZ)
#define AMIGA_DEFAULT_REPORT_TICKS (REPORT_TIMER_HZ / 100U)
#define AMIGA_MAX_REPORT_TICKS     (REPORT_TIMER_HZ / 20U)
#define BUTTON_HOLD_REFRESH_TICKS 20U


#define HEARTBEAT_REFRESH_TICKS (PS2_REPORT_HZ/4)

/* ============================================================================
 * Peripheral Handles & External Declarations
 * ============================================================================ */
TIM_HandleTypeDef htim2;

extern USBH_HandleTypeDef hUsbHostFS;
extern TIM_HandleTypeDef htim2;

/* ============================================================================
 * Global & Static State Variables
 * ============================================================================ */

// Shared atomic buffers updated by USB interrupt
volatile int16_t pending_dx = 0;
volatile int16_t pending_dy = 0;
volatile uint8_t current_btns = 0;
volatile uint8_t ledon = 0;

static uint8_t prev_btn_left = 0;
static uint8_t prev_btn_rght = 0;
static uint8_t prev_btn_mid = 0;
static uint8_t last_btns = 0;
static uint8_t button_hold_refresh_ticks = 0;
static uint16_t heartbeat_refresh_ticks = 0;
static volatile int16_t amiga_pending_dx = 0;
static volatile int16_t amiga_pending_dy = 0;
static volatile uint8_t amiga_current_buttons = 0;
static volatile uint16_t amiga_ticks_since_report = AMIGA_DEFAULT_REPORT_TICKS;
static uint16_t amiga_report_ticks = AMIGA_DEFAULT_REPORT_TICKS;
static uint16_t amiga_x_error = 0;
static uint16_t amiga_y_error = 0;
static uint8_t amiga_x_phase = 0;
static uint8_t amiga_y_phase = 0;

/* ============================================================================
 * Function Prototypes
 * ============================================================================ */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
void MX_USB_HOST_Process(void);

static inline void PS2_CLK_Low(void);
static inline void PS2_CLK_High(void);
static inline void PS2_DATA_Low(void);
static inline void PS2_DATA_High(void);
static void PS2_Timing_Init(void);
void PS2_Write_Byte(uint8_t data);
void PS2_Send_Packet(int16_t dx, int16_t dy, uint8_t buttons);
static void AMIGA_Init_Output_State(void);
static void AMIGA_Queue_Report(int16_t dx, int16_t dy, uint8_t buttons);
static void AMIGA_Service_Output(void);

/* ============================================================================
 * Main Entry Point

 * ============================================================================ */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    PS2_Timing_Init();
    MX_GPIO_Init();
    AMIGA_Init_Output_State();
    MX_USB_HOST_Init();
    MX_TIM2_Init();

    HAL_TIM_Base_Start_IT(&htim2);

    while (1) {
        MX_USB_HOST_Process();
        static uint32_t last_blink = 0;
        if (HAL_GetTick() - last_blink >= 22) {
            //HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            last_blink = HAL_GetTick();
			if(ledon){
				ledon = 0;
				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, 0);
			} else 
				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, 1);
        }
		
    }
}

/* ============================================================================
 * Low-Level Bit-Bang Helpers & PS/2 Interface
 * ============================================================================ */
// Low-level bit-bang helper (Open-drain: LOW = pull down, HIGH = release to pull-up)
static inline void PS2_CLK_Low(void) {
    PS2_CLK_PORT->BSRR = (uint32_t)PS2_CLK_PIN << 16;
}

static inline void PS2_CLK_High(void) {
    PS2_CLK_PORT->BSRR = PS2_CLK_PIN;
}

static inline void PS2_DATA_Low(void) {
    PS2_DATA_PORT->BSRR = (uint32_t)PS2_DATA_PIN << 16;
}

static inline void PS2_DATA_High(void) {
    PS2_DATA_PORT->BSRR = PS2_DATA_PIN;
}

static void PS2_Timing_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline void PS2_Delay_Cycles(uint32_t cycles) {
    uint32_t start = DWT->CYCCNT;

    while ((uint32_t)(DWT->CYCCNT - start) < cycles) {
    }
}

static inline void PS2_Delay_Half_Bit(void) {
    PS2_Delay_Cycles(PS2_HALF_BIT_CYCLES);
}

static inline void PS2_Clock_Bit(void) {
    PS2_Delay_Half_Bit();
    PS2_CLK_Low();
    PS2_Delay_Half_Bit();
    PS2_CLK_High();
}

// Transmit a single byte using the configured PS/2 frame timing.
void PS2_Write_Byte(uint8_t data) {
    uint8_t parity = 1;

    // Guard against SysTick / IRQ jitter during frame transmission
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    // 1. Start Bit (Low)
    PS2_DATA_Low();
    PS2_Clock_Bit();

    // 2. 8 Data Bits (LSB First) - Branchless DATA drive
    for (int i = 0; i < 8; i++) {
        uint8_t bit = (data >> i) & 1;
        parity ^= bit;

        // Constant-time bit write using BSRR register math (No IF statements)
        PS2_DATA_PORT->BSRR = (uint32_t)PS2_DATA_PIN << ((!bit) * 16);
        PS2_Clock_Bit();
    }

    // 3. Parity Bit (Odd) - Branchless
    PS2_DATA_PORT->BSRR = (uint32_t)PS2_DATA_PIN << ((!parity) * 16);
    PS2_Clock_Bit();

    // 4. Stop Bit (High)
    PS2_DATA_High();
    PS2_Clock_Bit();

    // Restore interrupt state
    __set_PRIMASK(primask);
}

// Convert USB deltas to standard 3-Byte PS/2 Mouse Packet
void PS2_Send_Packet(int16_t dx, int16_t dy, uint8_t buttons) {
    // PS/2 Y-axis is inverted relative to USB HID
    dy = -dy;

    // Clamp values to standard int8_t limits (-127 to +127)
    if (dx > 127)  dx = 127;
    if (dx < -127) dx = -127;
    if (dy > 127)  dy = 127;
    if (dy < -127) dy = -127;

    // Build Byte 1 Flags
    uint8_t b1 = 0x08; // Bit 3 is always 1
    if (buttons & 1) b1 |= 0x01; // Left Button
    if (buttons & 2) b1 |= 0x02; // Right Button
    if (buttons & 4) b1 |= 0x04; // Middle Button
    if (dx < 0) b1 |= 0x10; // X Sign Bit
    if (dy < 0) b1 |= 0x20; // Y Sign Bit

    // Byte 2 & 3: Raw low 8 bits of deltas
    uint8_t b2 = (uint8_t) (dx & 0xFF);
    uint8_t b3 = (uint8_t) (dy & 0xFF);

    // Send the 3-byte packet sequence
    PS2_Write_Byte(b1);
    PS2_Delay_Cycles(PS2_INTER_BYTE_CYCLES);
    PS2_Write_Byte(b2);
    PS2_Delay_Cycles(PS2_INTER_BYTE_CYCLES);
    PS2_Write_Byte(b3);
}

/* ============================================================================
 * Amiga Mouse Quadrature Output
 * ============================================================================ */
static void AMIGA_Write_Pin(uint16_t pin, uint8_t high) {
    AMIGA_PORT->BSRR = high ? pin : ((uint32_t) pin << 16);
}

static void AMIGA_Write_Quadrature(uint16_t phase_a_pin, uint16_t phase_b_pin,
        uint8_t phase) {
    // Idle high keeps the DE-9 direction lines released when there is no motion.
    static const uint8_t state[4][2] = {
            { 1, 1 },
            { 0, 1 },
            { 0, 0 },
            { 1, 0 },
    };
    uint32_t set_pins = 0;
    uint32_t reset_pins = 0;

    if (state[phase][0]) {
        set_pins |= phase_a_pin;
    } else {
        reset_pins |= phase_a_pin;
    }

    if (state[phase][1]) {
        set_pins |= phase_b_pin;
    } else {
        reset_pins |= phase_b_pin;
    }

    AMIGA_PORT->BSRR = set_pins | (reset_pins << 16);
}

static void AMIGA_Step_Axis(uint8_t *phase, int8_t direction,
        uint16_t phase_a_pin, uint16_t phase_b_pin) {
    if (direction > 0) {
        *phase = (*phase + 1U) & 0x03U;
    } else {
        *phase = (*phase + 3U) & 0x03U;
    }

    AMIGA_Write_Quadrature(phase_a_pin, phase_b_pin, *phase);
}

static void AMIGA_Set_Buttons(uint8_t buttons) {
    // Amiga mouse buttons are active-low.
    AMIGA_Write_Pin(AMIGA_BTN1_PIN, (buttons & 0x01U) == 0U);
    AMIGA_Write_Pin(AMIGA_BTN2_PIN, (buttons & 0x02U) == 0U);
    AMIGA_Write_Pin(AMIGA_BTN3_PIN, (buttons & 0x04U) == 0U);
}

static void AMIGA_Init_Output_State(void) {
    AMIGA_Write_Quadrature(AMIGA_H_PIN, AMIGA_HQ_PIN, amiga_x_phase);
    AMIGA_Write_Quadrature(AMIGA_V_PIN, AMIGA_VQ_PIN, amiga_y_phase);
    AMIGA_Set_Buttons(0U);
}

static int16_t AMIGA_Clamp_Pending(int32_t value) {
    if (value > 127) {
        return 127;
    }
    if (value < -127) {
        return -127;
    }
    return (int16_t) value;
}

static uint16_t AMIGA_Abs16(int16_t value) {
    return (uint16_t) ((value < 0) ? -value : value);
}

static void AMIGA_Queue_Report(int16_t dx, int16_t dy, uint8_t buttons) {
    uint16_t report_ticks = amiga_ticks_since_report;

    if (report_ticks == 0U || report_ticks > AMIGA_MAX_REPORT_TICKS) {
        report_ticks = AMIGA_DEFAULT_REPORT_TICKS;
    }

    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    amiga_pending_dx = AMIGA_Clamp_Pending((int32_t) amiga_pending_dx + dx);
    amiga_pending_dy = AMIGA_Clamp_Pending((int32_t) amiga_pending_dy + dy);
    amiga_current_buttons = buttons;
    amiga_report_ticks = report_ticks;
    amiga_ticks_since_report = 0U;

    __set_PRIMASK(primask);

    AMIGA_Set_Buttons(buttons);
}

static void AMIGA_Service_Axis(volatile int16_t *pending, uint16_t *error,
        uint8_t *phase, int8_t positive_direction, uint16_t phase_a_pin,
        uint16_t phase_b_pin) {
    int16_t delta = *pending;

    if (delta == 0) {
        *error = 0U;
        return;
    }

    *error += AMIGA_Abs16(delta);

    if (*error >= amiga_report_ticks) {
        *error -= amiga_report_ticks;

        if (delta > 0) {
            AMIGA_Step_Axis(phase, positive_direction, phase_a_pin,
                    phase_b_pin);
            (*pending)--;
        } else {
            AMIGA_Step_Axis(phase, -positive_direction, phase_a_pin,
                    phase_b_pin);
            (*pending)++;
        }
    }
}

static void AMIGA_Service_Output(void) {
    if (amiga_ticks_since_report < AMIGA_MAX_REPORT_TICKS) {
        amiga_ticks_since_report++;
    }

    AMIGA_Set_Buttons(amiga_current_buttons);
    AMIGA_Service_Axis(&amiga_pending_dx, &amiga_x_error, &amiga_x_phase,
            AMIGA_X_DIRECTION, AMIGA_H_PIN, AMIGA_HQ_PIN);
    AMIGA_Service_Axis(&amiga_pending_dy, &amiga_y_error, &amiga_y_phase,
            AMIGA_Y_DIRECTION, AMIGA_V_PIN, AMIGA_VQ_PIN);
}

/* ============================================================================
 * Interrupt & Host Event Callbacks
 * ============================================================================ */
void USBH_HID_EventCallback(USBH_HandleTypeDef *phost) {
    if (USBH_HID_GetDeviceType(phost) == HID_MOUSE) {
        HID_MOUSE_Info_TypeDef *mouse_info = USBH_HID_GetMouseInfo(phost);

        if (mouse_info != NULL) {
            int8_t dx = (int8_t) mouse_info->x;
            int8_t dy = (int8_t) mouse_info->y;
            uint8_t buttons = (mouse_info->buttons[0] ? 1 : 0)
                    | (mouse_info->buttons[1] ? 2 : 0)
                    | (mouse_info->buttons[2] ? 4 : 0);

            AMIGA_Queue_Report(dx, dy, buttons);

            // Accumulate relative deltas from raw HID packets
            pending_dx += dx;
            pending_dy += dy;

            // Pack buttons (Bit 0 = Left, Bit 1 = Right, Bit 2 = Middle)
            current_btns = buttons;

            // Clear the internal HAL buffer
            mouse_info->x = 0;
            mouse_info->y = 0;
        }
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        static uint8_t ps2_report_tick = 0;

        AMIGA_Service_Output();

        ps2_report_tick++;
        if (ps2_report_tick < PS2_REPORT_TICKS) {
            return;
        }
        ps2_report_tick = 0;

        int16_t dx = pending_dx;
        int16_t dy = pending_dy;
        uint8_t btns = current_btns;
        uint8_t send_packet = 0;

        pending_dx = 0;
        pending_dy = 0;

        if (dx != 0 || dy != 0 || btns != last_btns) {
            send_packet = 1;
            heartbeat_refresh_ticks = (HEARTBEAT_REFRESH_TICKS/2);
        } else {
            heartbeat_refresh_ticks++;
            if (heartbeat_refresh_ticks >= HEARTBEAT_REFRESH_TICKS) {
                send_packet = 1;
                heartbeat_refresh_ticks = 0;
            }
        }


        // Send packets from one place so heartbeat bytes cannot interleave movement bytes.
        if (send_packet) {
            PS2_Send_Packet(dx, dy, btns);
			ledon = 100;

            last_btns = btns;
        }
    }
}

/* ============================================================================
 * STM32 Hardware Initialization Functions
 * ============================================================================ */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
            | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_TIM2_Init(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
    TIM_MasterConfigTypeDef sMasterConfig = { 0 };

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = REPORT_TIMER_PSC;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = REPORT_TIMER_PERIOD;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig)
            != HAL_OK) {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    HAL_GPIO_WritePin(AMIGA_PORT, AMIGA_OUTPUT_PINS, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = AMIGA_OUTPUT_PINS;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(AMIGA_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // Open-Drain is required for PS/2!
    GPIO_InitStruct.Pull = GPIO_PULLUP;          // Internal pull-up enable
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* ============================================================================
 * Fault & Assert Handling
 * ============================================================================ */
void Error_Handler(void) {
    __disable_irq();
    while (1) {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
