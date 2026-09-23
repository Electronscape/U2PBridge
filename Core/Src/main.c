#include "main.h"
#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_hid.h"
#include <string.h>
#include <stdint.h>

/* ============================================================================
 * Defines & Hardware Mapping
 * ============================================================================ */
#define PS2_CLK_PORT  GPIOB
#define PS2_CLK_PIN   GPIO_PIN_0
#define PS2_DATA_PORT GPIOB
#define PS2_DATA_PIN  GPIO_PIN_1

/* ============================================================================
 * Peripheral Handles & External Declarations
 * ============================================================================ */
TIM_HandleTypeDef htim2;
UART_HandleTypeDef huart2;

extern USBH_HandleTypeDef hUsbHostFS;
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim2; // Assuming TIM2 configured for 8ms (125 Hz)

/* ============================================================================
 * Global & Static State Variables
 * ============================================================================ */
// Absolute screen position tracking (480x320)
static int16_t screen_x = 240; // Center screen default X
static int16_t screen_y = 160; // Center screen default Y

// Shared atomic buffers updated by USB interrupt
volatile int16_t pending_dx = 0;
volatile int16_t pending_dy = 0;
volatile uint8_t current_btns = 0;
volatile uint8_t ledon = 0;

static uint8_t prev_btn_left = 0;
static uint8_t prev_btn_rght = 0;
static uint8_t prev_btn_mid = 0;
static int16_t last_dx = 0;
static int16_t last_dy = 0;
static uint8_t last_btns = 0;

/* ============================================================================
 * Function Prototypes
 * ============================================================================ */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
void MX_USB_HOST_Process(void);

static inline void PS2_CLK_Low(void);
static inline void PS2_CLK_High(void);
static inline void PS2_DATA_Low(void);
static inline void PS2_DATA_High(void);
static void PS2_Delay_us(uint32_t us);
void PS2_Write_Byte(uint8_t data);
void PS2_Send_Packet(int16_t dx, int16_t dy, uint8_t buttons);

/* ============================================================================
 * Main Entry Point
 
 * ============================================================================ */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_USB_HOST_Init();
    MX_TIM2_Init();

    HAL_TIM_Base_Start_IT(&htim2);

    char msg[] = "USART2: I'm alive!\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*) msg, strlen(msg), 100);

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
    HAL_GPIO_WritePin(PS2_CLK_PORT, PS2_CLK_PIN, GPIO_PIN_RESET);
}
static inline void PS2_CLK_High(void) {
    HAL_GPIO_WritePin(PS2_CLK_PORT, PS2_CLK_PIN, GPIO_PIN_SET);
}
static inline void PS2_DATA_Low(void) {
    HAL_GPIO_WritePin(PS2_DATA_PORT, PS2_DATA_PIN, GPIO_PIN_RESET);
}
static inline void PS2_DATA_High(void) {
    HAL_GPIO_WritePin(PS2_DATA_PORT, PS2_DATA_PIN, GPIO_PIN_SET);
}

static void PS2_Delay_us(uint32_t us) {
    uint32_t count = us * 10; // Simple microsecond delay loop
    while (count--) {
        __NOP();
    }
}

// Transmit a single byte using standard PS/2 frame timing (~10kHz - 16kHz clock)
void PS2_Write_Byte(uint8_t data) {
    uint8_t parity = 1; // Odd parity calculation

    // 1. Start Bit (Low)
    PS2_DATA_Low();
    PS2_Delay_us(20);
    PS2_CLK_Low();
    PS2_Delay_us(40);
    PS2_CLK_High();
    PS2_Delay_us(20);

    // 2. 8 Data Bits (LSB First)
    for (int i = 0; i < 8; i++) {
        uint8_t bit = (data >> i) & 1;
        parity ^= bit;

        if (bit)
            PS2_DATA_High();
        else
            PS2_DATA_Low();

        PS2_Delay_us(20);
        PS2_CLK_Low();
        PS2_Delay_us(40);
        PS2_CLK_High();
        PS2_Delay_us(20);
    }

    // 3. Parity Bit (Odd)
    if (parity)
        PS2_DATA_High();
    else
        PS2_DATA_Low();

    PS2_Delay_us(20);
    PS2_CLK_Low();
    PS2_Delay_us(40);
    PS2_CLK_High();
    PS2_Delay_us(20);

    // 4. Stop Bit (High)
    PS2_DATA_High();
    PS2_Delay_us(20);
    PS2_CLK_Low();
    PS2_Delay_us(40);
    PS2_CLK_High();
    PS2_Delay_us(40);
}

// Convert USB deltas to standard 3-Byte PS/2 Mouse Packet
void PS2_Send_Packet(int16_t dx, int16_t dy, uint8_t buttons) {
    // PS/2 Y-axis is inverted relative to USB HID
    dy = -dy;

    // Clamp values to standard int8_t limits (-127 to +127)
    if (dx > 127)
        dx = 127;
    if (dx < -127)
        dx = -127;
    if (dy > 127)
        dy = 127;
    if (dy < -127)
        dy = -127;

    // Build Byte 1 Flags
    uint8_t b1 = 0x08; // Bit 3 is always 1
    if (buttons & 1)
        b1 |= 0x01; // Left Button
    if (buttons & 2)
        b1 |= 0x02; // Right Button
    if (buttons & 4)
        b1 |= 0x04; // Middle Button
    if (dx < 0)
        b1 |= 0x10; // X Sign Bit
    if (dy < 0)
        b1 |= 0x20; // Y Sign Bit

    // Byte 2 & 3: Raw low 8 bits of deltas
    uint8_t b2 = (uint8_t) (dx & 0xFF);
    uint8_t b3 = (uint8_t) (dy & 0xFF);

    // Send the 3-byte packet sequence
    PS2_Write_Byte(b1);
    PS2_Write_Byte(b2);
    PS2_Write_Byte(b3);
}

/* ============================================================================
 * Interrupt & Host Event Callbacks
 * ============================================================================ */
void USBH_HID_EventCallback(USBH_HandleTypeDef *phost) {
    if (USBH_HID_GetDeviceType(phost) == HID_MOUSE) {
        HID_MOUSE_Info_TypeDef *mouse_info = USBH_HID_GetMouseInfo(phost);

        if (mouse_info != NULL) {
            // Accumulate relative deltas from raw HID packets
            pending_dx += (int8_t) mouse_info->x;
            pending_dy += (int8_t) mouse_info->y;

            // Pack buttons (Bit 0 = Left, Bit 1 = Right, Bit 2 = Middle)
            current_btns = (mouse_info->buttons[0] ? 1 : 0)
                    | (mouse_info->buttons[1] ? 2 : 0)
                    | (mouse_info->buttons[2] ? 4 : 0);

            // Clear the internal HAL buffer
            mouse_info->x = 0;
            mouse_info->y = 0;
        }
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        int16_t dx = pending_dx;
        int16_t dy = pending_dy;
        uint8_t btns = current_btns;

        pending_dx = 0;
        pending_dy = 0;

        // Send PS/2 packet whenever there is motion, when motion stops, or buttons change
        if (dx != 0 || dy != 0 || last_dx != 0 || last_dy != 0
                || btns != last_btns) {
            // 1. Send native PS/2 packet out PB0/PB1
            PS2_Send_Packet(dx, dy, btns);
			ledon = 100;

            // 2. Accumulate deltas into bounded screen-space coordinates
            screen_x += dx;
            screen_y += dy;

            // Clamp X to [0 .. 479]
            if (screen_x < 0)
                screen_x = 0;
            if (screen_x > 479)
                screen_x = 479;

            // Clamp Y to [0 .. 319]
            if (screen_y < 0)
                screen_y = 0;
            if (screen_y > 319)
                screen_y = 319;

#if(0)
            // 3. Heartbeat report over UART2
            uint8_t btn_left = (btns & 1) ? 1 : 0;
            uint8_t btn_rght = (btns & 2) ? 1 : 0;
            uint8_t btn_mid = (btns & 4) ? 1 : 0;

            char msg[64];
            int len = snprintf(msg, sizeof(msg),
                    "Pos:[%3d, %3d] | dX:%3d dY:%3d | L:%d R:%d M:%d\r\n",
                    screen_x, screen_y, dx, dy, btn_left, btn_rght, btn_mid);

            HAL_UART_Transmit(&huart2, (uint8_t*) msg, len, 2);
#endif
            last_dx = dx;
            last_dy = dy;
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
    htim2.Init.Prescaler = 83;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 999;
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

static void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
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

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; // Open-Drain is required for PS/2!
    GPIO_InitStruct.Pull = GPIO_PULLUP;          // Internal pull-up enable
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
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