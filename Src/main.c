/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);


/* Function processing DMA Rx data. Counts how many capital and small letters are in sentence.
 * Result is supposed to be stored in global variable of type "letter_count_" that is defined in "main.h"
 *
 * @param1 - received sign
 */
void proccesDmaData(uint8_t sign);


/* Space for your global variables. */
#define MAX_MESSAGE_CHARS 35U
#define MESSAGE_QUEUE_SIZE 16U

typedef struct
{
	char text[MAX_MESSAGE_CHARS + 1U];
	letter_count_ letters;
} received_message_t;

static received_message_t messageQueue[MESSAGE_QUEUE_SIZE];
static volatile uint8_t messageHead = 0U;
static volatile uint8_t messageTail = 0U;
static char currentMessage[MAX_MESSAGE_CHARS + 1U];
static uint8_t currentLength = 0U;
static letter_count_ currentLetters = {0U, 0U};
static uint8_t receivingMessage = 0U;
static uint8_t replyBuffer[96];
volatile uint32_t appMillis = 0U;


int main(void)
{
  uint32_t lastReportTick;
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* Configure the system clock */
  SystemClock_Config();
  LL_SYSTICK_EnableIT();
  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  USART2_RegisterCallback(proccesDmaData);
  MX_USART2_UART_Init();
  lastReportTick = appMillis;

  /* Space for your local variables, callback registration ...*/

  while (1)
  {
	  /* Periodic transmission of information about DMA Rx buffer state.
	   * Transmission frequency - 0.5Hz.
	   * Message format - "Buffer capacity: %d bytes, occupied memory: %d bytes, load [in %]: %f%"
	   * Example message (what I wish to see in terminal) - Buffer capacity: 1000 bytes, occupied memory: 231 bytes, load [in %]: 23.1%
	   */

	  /* Valid text string information transmission.
	   * Transmission frequency - when new valid string is received.
	   * Message format - "Valid string: %s, lower-case: %d, upper-case: %d"
	   * Example message (what I wish to see in terminal) - Valid string: Platn15uborZnakov, lower-case: 13, upper-case: 2
	   */
	  if ((messageTail != messageHead)
			  && (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7) == 0U))
	  {
		  received_message_t message;
		  int replyLength;

		  /* The ISR publishes a slot only after filling the whole message. */
		  message = messageQueue[messageTail];
		  messageTail = (uint8_t)((messageTail + 1U) % MESSAGE_QUEUE_SIZE);
		  replyLength = snprintf((char *)replyBuffer, sizeof(replyBuffer),
				  "Valid string: %s, lower-case: %u, upper-case: %u\r\n",
				  message.text, (unsigned int)message.letters.small_letter,
				  (unsigned int)message.letters.capital_letter);
		  if ((replyLength > 0) && ((size_t)replyLength < sizeof(replyBuffer)))
		  {
			  USART2_PutBuffer(replyBuffer, (uint8_t)replyLength);
		  }
	  }

	  /* SysTick is 1 ms; 2000 ticks correspond to the requested 0.5 Hz. */
	  if (((uint32_t)(appMillis - lastReportTick) >= 2000U)
			  && (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7) == 0U))
	  {
		  uint32_t occupied = DMA_USART2_BUFFER_SIZE
				  - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_6);
		  uint32_t loadTenths = (occupied * 1000U
				  + DMA_USART2_BUFFER_SIZE / 2U) / DMA_USART2_BUFFER_SIZE;
		  int reportLength = snprintf((char *)replyBuffer, sizeof(replyBuffer),
				  "Buffer capacity: %u bytes, occupied memory: %u bytes, "
				  "load [in %%]: %u.%u%%\r\n",
				  (unsigned int)DMA_USART2_BUFFER_SIZE, (unsigned int)occupied,
				  (unsigned int)(loadTenths / 10U),
				  (unsigned int)(loadTenths % 10U));
		  if ((reportLength > 0) && ((size_t)reportLength < sizeof(replyBuffer)))
		  {
			  lastReportTick = appMillis;
			  USART2_PutBuffer(replyBuffer, (uint8_t)reportLength);
		  }
	  }
  }
  /* USER CODE END 3 */
}


void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);

  if(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_0)
  {
  Error_Handler();  
  }
  LL_RCC_HSI_Enable();

   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {
    
  }
  LL_RCC_HSI_SetCalibTrimming(16);
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
  {
  
  }
  LL_Init1msTick(8000000);
  LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
  LL_SetSystemCoreClock(8000000);
}

/*
 * Implementation of function processing data received via USART.
 */
void proccesDmaData(uint8_t sign)
{
	if (receivingMessage == 0U)
	{
		if (sign == '#')
		{
			receivingMessage = 1U;
			currentLength = 0U;
			currentLetters.small_letter = 0U;
			currentLetters.capital_letter = 0U;
		}
		return;
	}

	/* The terminating '$' must be among the first 35 chars after '#'. */
	if (sign == '$')
	{
		uint8_t nextHead = (uint8_t)((messageHead + 1U) % MESSAGE_QUEUE_SIZE);
		if (nextHead != messageTail)
		{
			received_message_t *slot = &messageQueue[messageHead];
			memcpy(slot->text, currentMessage, currentLength);
			slot->text[currentLength] = '\0';
			slot->letters = currentLetters;
			messageHead = nextHead;
		}
		receivingMessage = 0U;
		return;
	}

	++currentLength;
	if (currentLength >= MAX_MESSAGE_CHARS)
	{
		/* No '$' in the 35 characters after '#': discard the string. */
		receivingMessage = 0U;
		return;
	}

	currentMessage[currentLength - 1U] = (char)sign;
	if ((sign >= 'a') && (sign <= 'z'))
	{
		++currentLetters.small_letter;
	}
	else if ((sign >= 'A') && (sign <= 'Z'))
	{
		++currentLetters.capital_letter;
	}
}


void Error_Handler(void)
{

}

#ifdef  USE_FULL_ASSERT

void assert_failed(char *file, uint32_t line)
{ 

}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
