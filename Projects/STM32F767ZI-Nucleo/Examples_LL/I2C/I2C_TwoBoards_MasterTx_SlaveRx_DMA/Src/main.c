/**
  ******************************************************************************
  * @file    Examples_LL/I2C/I2C_TwoBoards_MasterTx_SlaveRx_DMA/Src/main.c
  * @author  MCD Application Team
  * @brief   This example describes how to send/receive bytes over I2C IP using
  *          the STM32F7xx I2C LL API.
  *          Peripheral initialization done using LL unitary services functions.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/** @addtogroup STM32F7xx_LL_Examples
  * @{
  */

/** @addtogroup I2C_TwoBoards_MasterTx_SlaveRx_DMA
  * @{
  */

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
/**
  * @brief Timeout value
  */
#if (USE_TIMEOUT == 1)
#define DMA_SEND_TIMEOUT_TC_MS               5
#ifdef SLAVE_BOARD
#define I2C_SEND_TIMEOUT_STOP_MS             5
#else /* MASTER BOARD */
#define I2C_SEND_TIMEOUT_SB_MS               5
#define I2C_SEND_TIMEOUT_ADDR_MS             5
#endif
#endif /* USE_TIMEOUT */

/**
  * @brief I2C devices settings
  */
/* Timing register value is computed with the STM32CubeMX Tool,
  * Fast Mode @400kHz with I2CCLK = 216 MHz,
  * rise time = 100ns, fall time = 20ns
  * Timing Value = (uint32_t)0x00A01E5D
  */
#define I2C_TIMING                           0x00A01E5D

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
#if (USE_TIMEOUT == 1)
uint32_t Timeout                             = 0; /* Variable used for Timeout management */
#endif /* USE_TIMEOUT */
const uint8_t aLedOn[]                       = "LED ON";

/**
  * @brief Variables related to SlaveReceive process
  */
__IO uint8_t  ubNbDataToReceive              = sizeof(aLedOn);
uint8_t       aReceiveBuffer[sizeof(aLedOn)] = {0};
__IO uint8_t  ubSlaveTransferComplete        = 0;

/**
  * @brief Variables related to MasterTransmit process
  */
__IO uint8_t  ubNbDataToTransmit             = sizeof(aLedOn);
uint8_t*      pTransmitBuffer                = (uint8_t*)aLedOn;
__IO uint8_t  ubMasterTransferComplete       = 0;
__IO uint8_t  ubButtonPress                  = 0;

/* Private function prototypes -----------------------------------------------*/
void     SystemClock_Config(void);
void     LED_Init(void);
void     LED_On(void);
void     LED_Off(void);
void     LED_Blinking(uint32_t Period);
void     Configure_DMA(void);

#ifdef SLAVE_BOARD
void     Configure_I2C_Slave(void);
void     Handle_I2C_Slave(void);
uint8_t  Buffercmp8(uint8_t* pBuffer1, uint8_t* pBuffer2, uint8_t BufferLength);

#else /* MASTER_BOARD */

void     Configure_I2C_Master(void);
void     UserButton_Init(void);
void     WaitForUserButtonPress(void);
void     Handle_I2C_Master(void);
#endif /* SLAVE_BOARD */
static void CPU_CACHE_Enable(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* Enable the CPU Cache */
  CPU_CACHE_Enable();

  /* Configure the system clock to 216 MHz */
  SystemClock_Config();

  /* Initialize LED1 */
  LED_Init();

  /* Set LED1 Off */
  LED_Off();

  /* Configure DMA1_Channel3 and DMA1_Channel6 (DMA IP configuration in transfer memory to peripheral (I2C2)  */
  Configure_DMA();

#ifdef SLAVE_BOARD
  /* Configure I2C2 (I2C IP configuration in Slave mode and related GPIO initialization) */
  Configure_I2C_Slave();

  /* Handle I2C2 events (Slave) */
  Handle_I2C_Slave();
#else /* MASTER_BOARD */
  /* Initialize User push-button in EXTI mode */
  UserButton_Init();

  /* Configure I2C1 (I2C IP configuration in Master mode and related GPIO initialization) */
  Configure_I2C_Master();

  /* Wait for User push-button press to start transfer */
  WaitForUserButtonPress();

  /* Handle I2C1 events (Master) */
  Handle_I2C_Master();
#endif /* SLAVE_BOARD */

  /* Infinite loop */
  while (1)
  {
  }
}

/**
  * @brief  This function configures the DMA Channels for I2C2
  * @note   This function is used to :
  *         -1- Enable DMA1 clock
  *         -2- Configure NVIC for DMA1 transfer complete/error interrupts
  *         -3- Configure the DMA1_Channel3 functional parameters
  *         -4- Configure the DMA1_Channel6 functional parameters
  *         -5- Enable DMA1 interrupts complete/error
  * @param   None
  * @retval  None
  */
void Configure_DMA(void)
{
  /* DMA1_Channel3 used for I2C2 Reception
   * DMA1_Channel6 used for I2C1 Transmission
   */
  /* (1) Enable the clock of DMA1 */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

  /* (2) Configure NVIC for DMA transfer complete/error interrupts */
  NVIC_SetPriority(DMA1_Stream3_IRQn, 0);
  NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  NVIC_SetPriority(DMA1_Stream6_IRQn, 0);
  NVIC_EnableIRQ(DMA1_Stream6_IRQn);

  
  /* (3) Configure the DMA1 functional parameters */
  LL_DMA_SetChannelSelection(DMA1, LL_DMA_STREAM_3, LL_DMA_CHANNEL_7);

  LL_DMA_ConfigTransfer(DMA1, LL_DMA_STREAM_3, LL_DMA_DIRECTION_PERIPH_TO_MEMORY | \
                                                LL_DMA_PRIORITY_HIGH              | \
                                                LL_DMA_MODE_NORMAL                | \
                                                LL_DMA_PERIPH_NOINCREMENT           | \
                                                LL_DMA_MEMORY_INCREMENT           | \
                                                LL_DMA_PDATAALIGN_BYTE            | \
                                                LL_DMA_MDATAALIGN_BYTE);
  LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_3, ubNbDataToTransmit);
  LL_DMA_ConfigAddresses(DMA1, LL_DMA_STREAM_3, (uint32_t)LL_I2C_DMA_GetRegAddr(I2C2, LL_I2C_DMA_REG_DATA_RECEIVE), (uint32_t)&(aReceiveBuffer), LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_STREAM_3));

  /* (4) Configure the DMA1_Channel6 functional parameters */
  
  LL_DMA_SetChannelSelection(DMA1, LL_DMA_STREAM_6, LL_DMA_CHANNEL_1);
  
  LL_DMA_ConfigTransfer(DMA1, LL_DMA_STREAM_6, LL_DMA_DIRECTION_MEMORY_TO_PERIPH | \
                                                LL_DMA_PRIORITY_HIGH              | \
                                                LL_DMA_MODE_NORMAL                | \
                                                LL_DMA_PERIPH_NOINCREMENT           | \
                                                LL_DMA_MEMORY_INCREMENT           | \
                                                LL_DMA_PDATAALIGN_BYTE            | \
                                                LL_DMA_MDATAALIGN_BYTE);
  LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_6, ubNbDataToTransmit);
  LL_DMA_ConfigAddresses(DMA1, LL_DMA_STREAM_6, (uint32_t)pTransmitBuffer, (uint32_t)LL_I2C_DMA_GetRegAddr(I2C1, LL_I2C_DMA_REG_DATA_TRANSMIT), LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_STREAM_6));

  /* (4) Enable DMA interrupts complete/error */
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_3);
  LL_DMA_EnableIT_TE(DMA1, LL_DMA_STREAM_3);
  LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_6);
  LL_DMA_EnableIT_TE(DMA1, LL_DMA_STREAM_6);
}

#ifdef SLAVE_BOARD
/**
  * @brief  This function configures I2C2 in Slave mode.
  * @note   This function is used to :
  *         -1- Enables GPIO clock and configures the I2C2 pins.
  *         -2- Enable the I2C2 peripheral clock and I2C2 clock source.
  *         -3- Configure I2C2 functional parameters.
  *         -4- Enable DMA reception requests and I2C2.
  * @note   Peripheral configuration is minimal configuration from reset values.
  *         Thus, some useless LL unitary functions calls below are provided as
  *         commented examples - setting is default configuration from reset.
  * @param  None
  * @retval None
  */
void Configure_I2C_Slave(void)
{
  uint32_t timing = 0;

  /* (1) Enables GPIO clock and configures the I2C2 pins **********************/
  /*    (SCL on PB.10, SDA on PB.11)                     **********************/

  /* Enable the peripheral clock of GPIOB */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);

  /* Configure SCL Pin as : Alternate function, High Speed, Open drain, Pull up */
  LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_10, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_10, LL_GPIO_AF_4);
  LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_10, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_10, LL_GPIO_OUTPUT_OPENDRAIN);
  LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_10, LL_GPIO_PULL_UP);

  /* Configure SDA Pin as : Alternate function, High Speed, Open drain, Pull up */
  LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_11, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_11, LL_GPIO_AF_4);
  LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_11, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_11, LL_GPIO_OUTPUT_OPENDRAIN);
  LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_11, LL_GPIO_PULL_UP);

  /* (2) Enable the I2C2 peripheral clock and I2C2 clock source ***************/

  /* Enable the peripheral clock for I2C2 */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C2);

  /* Set I2C2 clock source as SYSCLK */
  LL_RCC_SetI2CClockSource(LL_RCC_I2C2_CLKSOURCE_SYSCLK);

  /* (3) Configure I2C2 functional parameters ********************************/

  /* Disable I2C2 prior modifying configuration registers */
  LL_I2C_Disable(I2C2);

  /* Configure the SDA setup, hold time and the SCL high, low period */
  /* Timing register value is computed with the STM32CubeMX Tool,
    * Fast Mode @400kHz with I2CCLK = 216 MHz,
    * rise time = 100ns, fall time = 20ns
    * Timing Value = (uint32_t)0x00A01E5D
    */
  timing = __LL_I2C_CONVERT_TIMINGS(0x0, 0xA, 0x0, 0x1E, 0x5D);
  LL_I2C_SetTiming(I2C2, timing);

  /* Configure the Own Address1 :
   *  - OwnAddress1 is SLAVE_OWN_ADDRESS
   *  - OwnAddrSize is LL_I2C_OWNADDRESS1_7BIT
   *  - Own Address1 is enabled
   */
  LL_I2C_SetOwnAddress1(I2C2, SLAVE_OWN_ADDRESS, LL_I2C_OWNADDRESS1_7BIT);
  LL_I2C_EnableOwnAddress1(I2C2);

  /* Enable Clock stretching */
  /* Reset Value is Clock stretching enabled */
  //LL_I2C_EnableClockStretching(I2C2);

  /* Configure Digital Noise Filter */
  /* Reset Value is 0x00            */
  //LL_I2C_SetDigitalFilter(I2C2, 0x00);

  /* Enable Analog Noise Filter           */
  /* Reset Value is Analog Filter enabled */
  //LL_I2C_EnableAnalogFilter(I2C2);

  /* Enable General Call                  */
  /* Reset Value is General Call disabled */
  //LL_I2C_EnableGeneralCall(I2C2);

  /* Configure the 7bits Own Address2               */
  /* Reset Values of :
   *     - OwnAddress2 is 0x00
   *     - OwnAddrMask is LL_I2C_OWNADDRESS2_NOMASK
   *     - Own Address2 is disabled
   */
  //LL_I2C_SetOwnAddress2(I2C2, 0x00, LL_I2C_OWNADDRESS2_NOMASK);
  //LL_I2C_DisableOwnAddress2(I2C2);

  /* Enable Peripheral in I2C mode */
  /* Reset Value is I2C mode */
  //LL_I2C_SetMode(I2C2, LL_I2C_MODE_I2C);

  /* (4) Enable DMA reception requests and I2C2 *******************************/
  LL_I2C_EnableDMAReq_RX(I2C2);
  LL_I2C_Enable(I2C2);
}

#else /* MASTER_BOARD */

/**
  * @brief  This function configures I2C1 in Master mode.
  * @note   This function is used to :
  *         -1- Enables GPIO clock and configures the I2C1 pins.
  *         -2- Enable the I2C1 peripheral clock and I2C1 clock source.
  *         -3- Configure I2C1 functional parameters.
  *         -4- Enable DMA transmission requests and I2C1.
  * @note   Peripheral configuration is minimal configuration from reset values.
  *         Thus, some useless LL unitary functions calls below are provided as
  *         commented examples - setting is default configuration from reset.
  * @param  None
  * @retval None
  */
void Configure_I2C_Master(void)
{
  /* (1) Enables GPIO clock and configures the I2C1 pins **********************/
  /*    (SCL on PB.6, SDA on PB.9)                     **********************/

  /* Enable the peripheral clock of GPIOB */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);

  /* Configure SCL Pin as : Alternate function, High Speed, Open drain, Pull up */
  LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_6, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_0_7(GPIOB, LL_GPIO_PIN_6, LL_GPIO_AF_4);
  LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_6, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_6, LL_GPIO_OUTPUT_OPENDRAIN);
  LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_6, LL_GPIO_PULL_UP);

  /* Configure SDA Pin as : Alternate function, High Speed, Open drain, Pull up */
  LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_9, LL_GPIO_MODE_ALTERNATE);
  LL_GPIO_SetAFPin_8_15(GPIOB, LL_GPIO_PIN_9, LL_GPIO_AF_4);
  LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_9, LL_GPIO_SPEED_FREQ_HIGH);
  LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_9, LL_GPIO_OUTPUT_OPENDRAIN);
  LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_9, LL_GPIO_PULL_UP);

  /* (2) Enable the I2C1 peripheral clock and I2C1 clock source ***************/

  /* Enable the peripheral clock for I2C1 */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

  /* Set I2C1 clock source as SYSCLK */
  LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_SYSCLK);

  /* (3) Configure I2C1 functional parameters ********************************/

  /* Disable I2C1 prior modifying configuration registers */
  LL_I2C_Disable(I2C1);

  /* Configure the SDA setup, hold time and the SCL high, low period */
  /* (uint32_t)0x00A01E5D = I2C_TIMING*/
  LL_I2C_SetTiming(I2C1, I2C_TIMING);

  /* Configure the Own Address1                   */
  /* Reset Values of :
   *     - OwnAddress1 is 0x00
   *     - OwnAddrSize is LL_I2C_OWNADDRESS1_7BIT
   *     - Own Address1 is disabled
   */
  //LL_I2C_SetOwnAddress1(I2C1, 0x00, LL_I2C_OWNADDRESS1_7BIT);
  //LL_I2C_DisableOwnAddress1(I2C1);

  /* Enable Clock stretching */
  /* Reset Value is Clock stretching enabled */
  //LL_I2C_EnableClockStretching(I2C1);

  /* Configure Digital Noise Filter */
  /* Reset Value is 0x00            */
  //LL_I2C_SetDigitalFilter(I2C1, 0x00);

  /* Enable Analog Noise Filter           */
  /* Reset Value is Analog Filter enabled */
  //LL_I2C_EnableAnalogFilter(I2C1);

  /* Enable General Call                  */
  /* Reset Value is General Call disabled */
  //LL_I2C_EnableGeneralCall(I2C1);

  /* Configure the 7bits Own Address2               */
  /* Reset Values of :
   *     - OwnAddress2 is 0x00
   *     - OwnAddrMask is LL_I2C_OWNADDRESS2_NOMASK
   *     - Own Address2 is disabled
   */
  //LL_I2C_SetOwnAddress2(I2C1, 0x00, LL_I2C_OWNADDRESS2_NOMASK);
  //LL_I2C_DisableOwnAddress2(I2C1);

  /* Configure the Master to operate in 7-bit or 10-bit addressing mode */
  /* Reset Value is LL_I2C_ADDRESSING_MODE_7BIT                         */
  //LL_I2C_SetMasterAddressingMode(I2C1, LL_I2C_ADDRESSING_MODE_7BIT);

  /* Enable Peripheral in I2C mode */
  /* Reset Value is I2C mode */
  //LL_I2C_SetMode(I2C1, LL_I2C_MODE_I2C);

  /* (4) Enable DMA transmission requests and I2C1 ****************************/
  LL_I2C_EnableDMAReq_TX(I2C1);
  LL_I2C_Enable(I2C1);
}

#endif /* SLAVE_BOARD */

/**
  * @brief  Initialize LED1.
  * @param  None
  * @retval None
  */
void LED_Init(void)
{
  /* Enable the LED1 Clock */
  LED1_GPIO_CLK_ENABLE();

  /* Configure IO in output push-pull mode to drive external LED1 */
  LL_GPIO_SetPinMode(LED1_GPIO_PORT, LED1_PIN, LL_GPIO_MODE_OUTPUT);
  /* Reset value is LL_GPIO_OUTPUT_PUSHPULL */
  //LL_GPIO_SetPinOutputType(LED1_GPIO_PORT, LED1_PIN, LL_GPIO_OUTPUT_PUSHPULL);
  /* Reset value is LL_GPIO_SPEED_FREQ_LOW */
  //LL_GPIO_SetPinSpeed(LED1_GPIO_PORT, LED1_PIN, LL_GPIO_SPEED_FREQ_LOW);
  /* Reset value is LL_GPIO_PULL_NO */
  //LL_GPIO_SetPinPull(LED1_GPIO_PORT, LED1_PIN, LL_GPIO_PULL_NO);
}

/**
  * @brief  Turn-on LED1.
  * @param  None
  * @retval None
  */
void LED_On(void)
{
  /* Turn LED1 on */
  LL_GPIO_SetOutputPin(LED1_GPIO_PORT, LED1_PIN);
}

/**
  * @brief  Turn-off LED1.
  * @param  None
  * @retval None
  */
void LED_Off(void)
{
  /* Turn LED1 off */
  LL_GPIO_ResetOutputPin(LED1_GPIO_PORT, LED1_PIN);
}

/**
  * @brief  Set LED1 to Blinking mode for an infinite loop (toggle period based on value provided as input parameter).
  * @param  Period : Period of time (in ms) between each toggling of LED
  *   This parameter can be user defined values. Pre-defined values used in that example are :   
  *     @arg LED_BLINK_FAST : Fast Blinking
  *     @arg LED_BLINK_SLOW : Slow Blinking
  *     @arg LED_BLINK_ERROR : Error specific Blinking
  * @retval None
  */
void LED_Blinking(uint32_t Period)
{
  /* Turn LED1 on */
  LL_GPIO_SetOutputPin(LED1_GPIO_PORT, LED1_PIN);

  /* Toggle IO in an infinite loop */
  while (1)
  {
    LL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);  
    LL_mDelay(Period);
  }
}


#ifdef SLAVE_BOARD
/**
  * @brief  This Function handle Slave events to perform a reception process
  * @note  This function is composed in different steps :
  *        -1- Wait ADDR flag and check address match code and direction
  *             -1.1- Enable DMA transfer(before clearing ADDR FLag).
  *        -2- Loop until end of transfer completed (DMA TC raised).
  *        -3- Loop until end of slave reception completed (STOP flag raised).
  *        -4- Clear pending flags, check Data consistency.
  * @param  None
  * @retval None
  */
void Handle_I2C_Slave(void)
{

  /* (1) Wait ADDR flag and check address match code and direction ************/
  while(!LL_I2C_IsActiveFlag_ADDR(I2C2))
  {
  }

  /* Verify the Address Match with the OWN Slave address */
  if(LL_I2C_GetAddressMatchCode(I2C2) == SLAVE_OWN_ADDRESS)
  {
    /* Verify the transfer direction, a write direction, Slave enters receiver mode */
    if(LL_I2C_GetTransferDirection(I2C2) == LL_I2C_DIRECTION_WRITE)
    {
      /* (1.1) Enable DMA transfer (before clearing ADDR FLag) ****************/
      LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_3);
      /* Clear ADDR flag value in ISR register */
      LL_I2C_ClearFlag_ADDR(I2C2);
    }
    else
    {
      /* Clear ADDR flag value in ISR register */
      LL_I2C_ClearFlag_ADDR(I2C2);

      /* Call Error function */
      Error_Callback();
    }
  }
  else
  {
    /* Clear ADDR flag value in ISR register */
    LL_I2C_ClearFlag_ADDR(I2C2);
      
    /* Call Error function */
    Error_Callback();
  }

  /* (4) Loop until end of transfer completed (DMA TC raised) *****************/

#if (USE_TIMEOUT == 1)
  Timeout = DMA_SEND_TIMEOUT_TC_MS;
#endif /* USE_TIMEOUT */

  /* Loop until DMA transfer complete event */
  while(!ubSlaveTransferComplete)
  {
#if (USE_TIMEOUT == 1)
    /* Check Systick counter flag to decrement the time-out value */
    if (LL_SYSTICK_IsActiveCounterFlag()) 
    {
      if(Timeout-- == 0)
      {
        /* Time-out occurred. Set LED to blinking mode */
        LED_Blinking(LED_BLINK_SLOW);
      }
    }
#endif /* USE_TIMEOUT */
  }
  
  /* (5) Loop until end of slave reception completed (STOP flag raised) *******/

#if (USE_TIMEOUT == 1)
  Timeout = I2C_SEND_TIMEOUT_STOP_MS;
#endif /* USE_TIMEOUT */

  /* Loop until STOP flag is raised  */
  while(!LL_I2C_IsActiveFlag_STOP(I2C2))
  {
#if (USE_TIMEOUT == 1)
    /* Check Systick counter flag to decrement the time-out value */
    if (LL_SYSTICK_IsActiveCounterFlag()) 
    {
      if(Timeout-- == 0)
      {
        /* Time-out occurred. Set LED1 to blinking mode */
        LED_Blinking(LED_BLINK_SLOW);
      }
    }
#endif /* USE_TIMEOUT */
  }

  /* (6) Clear pending flags, Data consistency are checking into Slave process */

  /* End of I2C_SlaveReceiver_MasterTransmitter_DMA Process */
  LL_I2C_ClearFlag_STOP(I2C2);

  /* Check if data request to turn on the LED1 */
  if(Buffercmp8((uint8_t*)aReceiveBuffer, (uint8_t*)aLedOn, (ubNbDataToReceive-1)) == 0)
  {
    /* Turn LED1 On:
     * Expected bytes have been received
     * Slave Rx sequence completed successfully
     */
    LED_On();
  }
  else
  {
    /* Call Error function */
    Error_Callback();
  }
}

/**
  * @brief  Compares two 8-bit buffers and returns the comparison result.
  * @param  pBuffer1: pointer to the source buffer to be compared to.
  * @param  pBuffer2: pointer to the second source buffer to be compared to the first.
  * @param  BufferLength: buffer's length.
  *    - 0: Comparison is OK (the two Buffers are identical)
  *    - Value different from 0: Comparison is NOK (Buffers are different)
  */
uint8_t Buffercmp8(uint8_t* pBuffer1, uint8_t* pBuffer2, uint8_t BufferLength)
{
  while (BufferLength--)
  {
    if (*pBuffer1 != *pBuffer2)
    {
      return 1;
    }

    pBuffer1++;
    pBuffer2++;
  }

  return 0;
}

#else /* MASTER_BOARD */

/**
  * @brief  Configures User push-button in GPIO or EXTI Line Mode.
  * @param  None 
  * @retval None
  */
void UserButton_Init(void)
{
  /* Enable the BUTTON Clock */
  USER_BUTTON_GPIO_CLK_ENABLE();

  /* Configure GPIO for BUTTON */
  LL_GPIO_SetPinMode(USER_BUTTON_GPIO_PORT, USER_BUTTON_PIN, LL_GPIO_MODE_INPUT);
  LL_GPIO_SetPinPull(USER_BUTTON_GPIO_PORT, USER_BUTTON_PIN, LL_GPIO_PULL_NO);

  /* Connect External Line to the GPIO*/
  USER_BUTTON_SYSCFG_SET_EXTI();

  /* Enable a rising trigger External line 13 Interrupt */
  USER_BUTTON_EXTI_LINE_ENABLE();
  USER_BUTTON_EXTI_FALLING_TRIG_ENABLE();

  /* Configure NVIC for USER_BUTTON_EXTI_IRQn */
  NVIC_EnableIRQ(USER_BUTTON_EXTI_IRQn); 
  NVIC_SetPriority(USER_BUTTON_EXTI_IRQn, 0x03);  
}

/**
  * @brief  Wait for User push-button press to start transfer.
  * @param  None 
  * @retval None
  */
  /*  */
void WaitForUserButtonPress(void)
{
  while (ubButtonPress == 0)
  {
    LL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);
    LL_mDelay(LED_BLINK_FAST);
  }
  /* Turn LED1 off */
  LL_GPIO_ResetOutputPin(LED1_GPIO_PORT, LED1_PIN);
}

/**
  * @brief  This Function handle Master events to perform a transmission process
  * @note  This function is composed in different steps :
  *        -1- Enable DMA transfer.
  *        -2- Initiate a Start condition to the Slave device.
  *        -3- Loop until end of transfer completed (DMA TC raised).
  *        -4- Loop until end of master transfer completed (STOP flag raised).
  *        -5- Clear pending flags, Data consistency are checking into Slave process.
  * @param  None
  * @retval None
  */
void Handle_I2C_Master(void)
{
  /* (1) Enable DMA transfer **************************************************/
  LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_6);
  /* (2) Initiate a Start condition to the Slave device ***********************/

  /* Master Generate Start condition for a write request:
   *  - to the Slave with a 7-Bit SLAVE_OWN_ADDRESS
   *  - with a auto stop condition generation when transmit all bytes
   */
  LL_I2C_HandleTransfer(I2C1, SLAVE_OWN_ADDRESS, LL_I2C_ADDRSLAVE_7BIT, ubNbDataToTransmit, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);

  /* (3) Loop until end of transfer completed (DMA TC raised) *****************/

#if (USE_TIMEOUT == 1)
  Timeout = DMA_SEND_TIMEOUT_TC_MS;
#endif /* USE_TIMEOUT */

  /* Loop until DMA transfer complete event */
  while(!ubMasterTransferComplete)
  {
#if (USE_TIMEOUT == 1)
    /* Check Systick counter flag to decrement the time-out value */
    if (LL_SYSTICK_IsActiveCounterFlag()) 
    {
      if(Timeout-- == 0)
      {
        /* Time-out occurred. Set LED to blinking mode */
        LED_Blinking(LED_BLINK_SLOW);
      }
    }
#endif /* USE_TIMEOUT */
  }
  
  /* (4) Loop until end of master transfer completed (STOP flag raised) *******/

#if (USE_TIMEOUT == 1)
  Timeout = I2C_SEND_TIMEOUT_STOP_MS;
#endif /* USE_TIMEOUT */

  /* Loop until STOP flag is raised  */
  while(!LL_I2C_IsActiveFlag_STOP(I2C1))
  {
#if (USE_TIMEOUT == 1)
    /* Check Systick counter flag to decrement the time-out value */
    if (LL_SYSTICK_IsActiveCounterFlag()) 
    {
      if(Timeout-- == 0)
      {
        /* Time-out occurred. Set LED1 to blinking mode */
        LED_Blinking(LED_BLINK_SLOW);
      }
    }
#endif /* USE_TIMEOUT */
  }

  /* (5) Clear pending flags, Data consistency are checking into Slave process */

  /* End of I2C_SlaveReceiver_MasterTransmitter_DMA Process */
  LL_I2C_ClearFlag_STOP(I2C1);

  /* Turn LED1 On:
   *  - Expected bytes have been sent
   *  - Master Tx sequence completed successfully
   */
  LED_On();
}
#endif /* SLAVE_BOARD */

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 216000000
  *            HCLK(Hz)                       = 216000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSI Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 432
  *            PLL_P                          = 2
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 7
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  /* Enable HSE clock */
  LL_RCC_HSE_EnableBypass();
  LL_RCC_HSE_Enable();
  while(LL_RCC_HSE_IsReady() != 1)
  {
  };

  /* Set FLASH latency */
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_7);

  /* Enable PWR clock */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* Activation OverDrive Mode */
  LL_PWR_EnableOverDriveMode();
  while(LL_PWR_IsActiveFlag_OD() != 1)
  {
  };

  /* Activation OverDrive Switching */
  LL_PWR_EnableOverDriveSwitching();
  while(LL_PWR_IsActiveFlag_ODSW() != 1)
  {
  };

  /* Main PLL configuration and activation */
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_8, 432, LL_RCC_PLLP_DIV_2);
  LL_RCC_PLL_Enable();
  while(LL_RCC_PLL_IsReady() != 1)
  {
  };

  /* Sysclk activation on the main PLL */
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  };

  /* Set APB1 & APB2 prescaler */
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);

  /* Set systick to 1ms */
  SysTick_Config(216000000 / 1000);

  /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
  SystemCoreClock = 216000000;
}
/******************************************************************************/
/*   IRQ HANDLER TREATMENT Functions                                          */
/******************************************************************************/
#ifdef SLAVE_BOARD
/**
  * @brief  Function called from DMA1 IRQ Handler
  * @note   This function is executed when the transfer complete interrupt
  *         is generated
  * @retval None
  */
void DMA1_Transfer_Complete_Callback()
{
  /* DMA transfer completed */
  ubSlaveTransferComplete = 1;
}

/**
  * @brief  Function called from DMA1 IRQ Handler
  * @note   This function is executed when the transfer error interrupt
  *         is generated during DMA transfer
  * @retval None
  */
void DMA1_Transfer_Error_Callback()
{
  /* Disable DMA1_Stream3_IRQn */
  NVIC_DisableIRQ(DMA1_Stream3_IRQn);
                  
  /* Error detected during DMA transfer */
  LED_Blinking(LED_BLINK_ERROR);
}

#else /* MASTER_BOARD */

/**
  * @brief  Function to manage User push-button
  * @param  None
  * @retval None
  */
void UserButton_Callback(void)
{
  /* Update User push-button variable : to be checked in waiting loop in main program */
  ubButtonPress = 1;
}

/**
  * @brief  Function called from DMA1 IRQ Handler
  * @note   This function is executed when the transfer complete interrupt
  *         is generated
  * @retval None
  */
void DMA1_Transfer_Complete_Callback()
{
  /* DMA transfer completed */
  ubMasterTransferComplete = 1;
}

/**
  * @brief  Function called from DMA1 IRQ Handler
  * @note   This function is executed when the transfer error interrupt
  *         is generated during DMA transfer
  * @retval None
  */
void DMA1_Transfer_Error_Callback()
{
  /* Disable DMA1_Stream6_IRQn */
  NVIC_DisableIRQ(DMA1_Stream6_IRQn);

  /* Error detected during DMA transfer */
  LED_Blinking(LED_BLINK_ERROR);
}
#endif /* SLAVE_BOARD */

/**
  * @brief  Function called in case of error detected in I2C IT Handler
  * @param  None
  * @retval None
  */
void Error_Callback(void)
{
  /* Disable DMA1_Stream3_IRQn and DMA1_Stream6_IRQn */
  NVIC_DisableIRQ(DMA1_Stream3_IRQn);
  NVIC_DisableIRQ(DMA1_Stream6_IRQn);

  /* Unexpected event : Set LED1 to Blinking mode to indicate error occurs */
  LED_Blinking(LED_BLINK_ERROR);
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */

