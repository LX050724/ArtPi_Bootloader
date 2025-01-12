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
#include "cmsis_gcc.h"
#include "memorymap.h"
#include "quadspi.h"
#include "usart.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <cmsis_compiler.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SDRAM_MODEREG_BURST_LENGTH_1 (0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2 (0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4 (0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8 (0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL (0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED (0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2 (0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3 (0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD (0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED (0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE (0x0200)

#define FLASH_CMD_FastReadQuad_IO 0xEB

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int sdram_command(uint32_t cmd, uint32_t refresh, uint32_t regval)
{
    FMC_SDRAM_CommandTypeDef sdram_cmd = {};
    sdram_cmd.CommandMode = cmd;
    sdram_cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    sdram_cmd.AutoRefreshNumber = refresh;
    sdram_cmd.ModeRegisterDefinition = regval;
    if (HAL_SDRAM_SendCommand(&hsdram1, &sdram_cmd, 100) != HAL_OK)
    {
        Error_Handler();
    }
    return HAL_OK;
}

int sdram_test()
{
    uint32_t sdram_size = 32 * 1024 * 1024;
    uint32_t *ptr = (uint32_t *)0xC0000000;

    for (uint32_t i = 0; i < sdram_size / sizeof(int); i++)
    {
        ptr[i] = i;
    }

    for (uint32_t i = 0; i < sdram_size / sizeof(int); i++)
    {
        if (ptr[i] != i)
            return -1;
    }

    for (uint32_t i = 0; i < sdram_size / sizeof(int); i++)
    {
        ptr[i] = 0;
    }

    return 0;
}

int sdram_init()
{
    sdram_command(FMC_SDRAM_CMD_CLK_ENABLE, 1, 0);
    HAL_Delay(1);
    sdram_command(FMC_SDRAM_CMD_PALL, 1, 0);
    sdram_command(FMC_SDRAM_CMD_AUTOREFRESH_MODE, 8, 0);
    uint32_t temp = SDRAM_MODEREG_BURST_LENGTH_1 | SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL | SDRAM_MODEREG_CAS_LATENCY_2 |
                    SDRAM_MODEREG_OPERATING_MODE_STANDARD | SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;
    sdram_command(FMC_SDRAM_CMD_LOAD_MODE, 1, temp);
    if (HAL_SDRAM_ProgramRefreshRate(&hsdram1, 761) != HAL_OK)
    {
        Error_Handler();
    }

    // HAL_Delay(1);
    // if (sdram_test() != 0)
    // {
    //     Error_Handler();
    // }
    return 0;
}

int qspiflash_init()
{
    QSPI_CommandTypeDef sCommand = {};
    QSPI_MemoryMappedTypeDef sMemMappedCfg = {};
    sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    sCommand.AddressSize = QSPI_ADDRESS_24_BITS;
    sCommand.AddressMode = QSPI_ADDRESS_4_LINES;
    sCommand.Instruction = FLASH_CMD_FastReadQuad_IO;
    sCommand.Address = 0;
    sCommand.NbData = 0;
    sCommand.DataMode = QSPI_DATA_4_LINES;
    sCommand.DummyCycles = 6;

    sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
    sMemMappedCfg.TimeOutPeriod = 0;

    if (HAL_QSPI_MemoryMapped(&hqspi, &sCommand, &sMemMappedCfg) != HAL_OK)
    {
        Error_Handler();
    }
    return 0;
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

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_FMC_Init();
  MX_QUADSPI_Init();
  MX_UART4_Init();
  /* USER CODE BEGIN 2 */
    sdram_init();
    qspiflash_init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

    uint32_t *flash_addr = (uint32_t *)0x90000000;
    uint32_t msp_addr = flash_addr[0];
    uint32_t entry_addr = flash_addr[1];

    if (msp_addr != 0xFFFFFFFF && entry_addr != 0xFFFFFFFF)
    {
        HAL_UART_DeInit(&huart4);
        __disable_irq();
        __set_MSP(msp_addr);
        __ASM volatile("bx %0" :: "r"(entry_addr));
    }

    Error_Handler();

    while (1)
    {
        HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
        HAL_Delay(300);
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_FMC|RCC_PERIPHCLK_QSPI;
  PeriphClkInitStruct.PLL2.PLL2M = 2;
  PeriphClkInitStruct.PLL2.PLL2N = 48;
  PeriphClkInitStruct.PLL2.PLL2P = 2;
  PeriphClkInitStruct.PLL2.PLL2Q = 3;
  PeriphClkInitStruct.PLL2.PLL2R = 3;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.FmcClockSelection = RCC_FMCCLKSOURCE_PLL2;
  PeriphClkInitStruct.QspiClockSelection = RCC_QSPICLKSOURCE_PLL2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
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
    HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);

    __BKPT(0);
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
