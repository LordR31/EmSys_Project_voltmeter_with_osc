/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_VOLTS 5
#define ADC_MAX_VALUE 4095
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
volatile uint16_t adc_value;
volatile float voltage_value;
const char *voltage_value_string[10];
bool is_data_available = false;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void FPU_Init(void){
	// for fpu
	SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));
	__DSB();
	__ISB();
}

void Clocks_Init(void){
	// enable clocks
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;   // GPIOA clock
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;    // ADC1 clock
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;   // GPIOB clock
	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;    // I2C1 clock
}

void ADC_Init(void){
	GPIOA->MODER &= ~(0x3 << 0); // clear MODER0
	GPIOA->MODER |= (0x3 << 0);  // set PA0 as ADC (Analog mode)

	ADC->CCR &= ~(0x3 << 16);;    // clear prescaler bits
	ADC->CCR |= (0x1 << 16);     // set prescaler to 01 (divide by 4 so the ADC clock is <= 36MH)
	ADC1->CR1 &= ~(0x3 << 24);    // set 12 bit resolution
	ADC1->CR1 |= ADC_CR1_EOCIE;  // enable adc end of conversion interrupt
	ADC1->SMPR2 &= ~(0x7 << 0);  // clear SMP0 (sample time reg)
	ADC1->SMPR2 |=  (0x1 << 0);  // set SMP0 to '001' (15 cycles needed by resolution)
	ADC1->SQR3 = 0;              // select channel 0
	ADC1->CR2 |= ADC_CR2_ADON |ADC_CR2_CONT;   // enable adc and set continuous mode
}

void ADC_Read(void){
	if (ADC1->SR & ADC_SR_EOC)
		adc_value = ADC1->DR; // Read result and clear EOC
}

void I2C1_Init(void){
	GPIOB->MODER &= ~((0x3 << 16) | (0x3 << 18)); // clear PB8 & PB9;
	GPIOB->MODER |= (0x2 << 16) | (0x2 << 18);   // set PB8 & PB9 to alternate function (I2C);

	GPIOB->AFR[1] &= ~((0xF << 0) | (0xF << 4)); // clear Alternate Function 4 regs
	GPIOB->AFR[1] |= (0x4 << 0) | (0x4 << 4);   // set Alternate Function 4 (I2C)

	GPIOB->OTYPER |= (0x1 << 16) | (0x1 << 18);  // set output type to open-drain for i2c protocol
	GPIOB->PUPDR &= ~((0x3 << 16) | (0x3 << 18)); // clear pull up pull down regs
	GPIOB->PUPDR |= (0x1 << 16) | (0x1 << 18);   // set pull up

	I2C1->CR2 &= ~(0x3F << 0);   // clear FREQ
	I2C1->CR2 |=  (0x10 << 0);   // set 16 MHz

	I2C1->CCR &= ~(0xFFF << 0);  // clear CCR
	I2C1->CCR |=  (0x50 << 0);   // set 100 KHz

	I2C1->TRISE &= ~(0x3F << 0); // clear TRISE[5:0]
	I2C1->TRISE |=  (0x11 << 0); // set TRISE 11

	I2C1->CR1 |= I2C_CR1_PE; // enable I2C
}

void I2C1_write(uint8_t addr, uint8_t data){
	 // Start condition
	I2C1->CR1 |= I2C_CR1_START;
	while (!(I2C1->SR1 & I2C_SR1_SB)); // Wait for start

	// Send address
	(void)I2C1->SR1;      // Clear SB
	I2C1->DR = addr << 1; // Write mode
	while (!(I2C1->SR1 & I2C_SR1_ADDR));
	(void)I2C1->SR2; // Clear ADDR

	// Send data
	while (!(I2C1->SR1 & I2C_SR1_TXE));
	I2C1->DR = data;
	while (!(I2C1->SR1 & I2C_SR1_BTF)); // Wait for byte transfer

	// Stop condition
	I2C1->CR1 |= I2C_CR1_STOP;
}

void LCD_send_command(uint8_t cmd){
    uint8_t high = (cmd & 0xF0);
    uint8_t low  = (cmd << 4) & 0xF0;

    // Send high nibble
    I2C1_write(0x27, high | 0x08); // RS=0, EN=0
    I2C1_write(0x27, high | 0x0C); // RS=0, EN=1
    for (volatile int i = 0; i < 1000; i++);
    I2C1_write(0x27, high | 0x08); // RS=0, EN=0

    // Send low nibble
    I2C1_write(0x27, low | 0x08);
    I2C1_write(0x27, low | 0x0C);
    for (volatile int i = 0; i < 1000; i++);
    I2C1_write(0x27, low | 0x08);
}

void LCD_Init(void){
	LCD_send_command(0x33); // Init
	LCD_send_command(0x32); // 4-bit mode
	LCD_send_command(0x28); // 2-line, 5x8 font
	LCD_send_command(0x0C); // Display ON, cursor OFF
	LCD_send_command(0x06); // Entry mode
	LCD_send_command(0x01); // Clear display
}

void LCD_send_data(uint8_t data){
    uint8_t high = (data & 0xF0);
    uint8_t low  = (data << 4) & 0xF0;

    // Send high nibble
    I2C1_write(0x27, high | 0x09); // RS=1, EN=0
    I2C1_write(0x27, high | 0x0D); // RS=1, EN=1
    for (volatile int i = 0; i < 1000; i++);
    I2C1_write(0x27, high | 0x09); // RS=1, EN=0

    // Send low nibble
    I2C1_write(0x27, low | 0x09);
    I2C1_write(0x27, low | 0x0D);
    for (volatile int i = 0; i < 1000; i++);
    I2C1_write(0x27, low | 0x09);
}


void LCD_send_string(char *str){
    while (*str){
        LCD_send_data(*str++);
    }
}

void LCD_set_cursor(uint8_t row, uint8_t col){
    uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54}; // DDRAM addresses for each row
    LCD_send_command(0x80 | (row_offsets[row] + col));
}

void LCD_DebugHelloWorld(void){
    LCD_set_cursor(0, 0);
    LCD_send_string("Hello Amazing World");

    LCD_set_cursor(1, 0);
    LCD_send_string("Hello Amazing World");

    LCD_set_cursor(2, 0);
    LCD_send_string("Hello Amazing World");

    LCD_set_cursor(3, 0);
    LCD_send_string("Hello Amazing World");
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  	FPU_Init();
  	Clocks_Init();
  	ADC_Init();
  	I2C1_Init();
  	LCD_Init();

  	ADC1->CR2 |= ADC_CR2_SWSTART; // start conversion

  	LCD_send_command(0x01); // Clear display
  	for (volatile int i = 0; i < 10000; i++);
  	LCD_set_cursor(0, 2);
  	LCD_send_string("VOLTMETER SCREEN");
//  	LCD_DebugHelloWorld();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1){
	ADC_Read();
	voltage_value = ((float)(adc_value) * MAX_VOLTS) / ADC_MAX_VALUE;  // convert value
	const char *voltage_value_string[10];
	uint16_t voltage_mV = (uint16_t)(voltage_value * 1000);  // Convert to millivolts
	sprintf(voltage_value_string, "%u.%02uV", voltage_mV / 1000, (voltage_mV % 1000) / 10);
	LCD_set_cursor(2, 3);
	LCD_send_string("Voltage: ");
	LCD_send_string(voltage_value_string);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
