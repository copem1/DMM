/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "string.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>

#include "UART.h"
#include "ADC.h"
#include "DMM.h"

#define BLANK ""
#define DELAY 500000
#define MAX_IDX 9
#define DC_SAMPLE_COUNT 1838
#define AC_SAMPLE_COUNT 36764

uint16_t eoc_flag;
uint16_t adc_val;
uint16_t count = 0;
uint16_t dc_conv_arr[DC_SAMPLE_COUNT];
uint16_t ac_conv_arr[AC_SAMPLE_COUNT];


int timerflag;
uint32_t time;
uint32_t prev_t;
uint32_t CLOCKSPEED = 24000000; // 24 MHz


void SystemClock_Config(void);

//void timer_init(void);

//void comp_init(void);

//void toString(uint32_t value, char *str, int max_index);

void display_freq(char *freq_str);

void display_DC(char *DC_voltage);

void display_RMS(char *RMS_voltage);

void display_P2P(char *P2P_voltage); // peak to peak voltage

void display_dcvoltage(float dc_voltage);

void display_acvoltage(float ac_voltage);

void build_graph(void);

int main(void)
{


  HAL_Init();

  SystemClock_Config();

  // initialize FSM
  typedef enum {
	  WAIT,
	  FREQUENCY,
	  DC,
	  AC

  } state_type; // create variable with predefined state values

  state_type state = WAIT; // initialize to WAIT state

  UART_init();
  ADC_init();
  build_graph();

  comp_init();
  timer_init();

  uint32_t wave_period;

  while (1)
  {
	  switch(state) {
	  	  case(WAIT):
	  	  	  if (timerflag == 1) {
	  	  		  state = FREQUENCY;
	  	  	  }
	  	  	  else {
	  	  		  state = WAIT;
	  	  	  }
//	  	  	  else if (eoc_flag == 1) {
//	  	  		  state = AC;
//	  	  	  }
//	  	  	  else if () {}

//	  	  	  break; // skip other states, carries current state through next while loop iteration

	  	  case(FREQUENCY):
//				 if (timerflag == 1) {
				 wave_period = time - prev_t; // find period in decimal

				 uint32_t freq = (CLOCKSPEED / wave_period); // convert to frequency
				 // calibrate frequency
				 if (freq > 550) {
					 freq = freq + 8;
				 }
				 else if (freq > 200 && freq <= 550) {
					 freq = freq + 4;
				 }
				 else {
					 freq = freq + 1;
				 }

				 char freq_str[9];
				 toString(freq, freq_str, 9); // convert frequency to string
				 display_freq(freq_str);
				//		 UART_print_str(freq_str); // print on REALTERM
				//		 UART_print_str("Hz\n");
				 timerflag = 0;
				 for (uint32_t i = DELAY; i > 0; i--); 	// wait at least 20 (us) microseconds
//				 }
	  	  	  	 state = DC;

	  	  case(DC):
	  		    //disable timer interrupts in NVIC
			    NVIC->ISER[0] |= (0 << (TIM2_IRQn & 0x1F));
	  	  	  	for (int i = 0; i < DC_SAMPLE_COUNT; i++) {
					if (count < DC_SAMPLE_COUNT){
							  dc_conv_arr[count] = adc_val; // put conversion value in to the arr
							  count += 1;
							  ADC1->CR |= ADC_CR_ADSTART; // restart the conversion
							  eoc_flag = 0; // clear the flag used in ISR
					}
					//count = DC_SAMPLE_COUNT;
					if (count == DC_SAMPLE_COUNT){
							  uint32_t avg_int = find_avg(dc_conv_arr);
							  float avg_float = calibrate_volt(avg_int);
							  char DC_str[10];
							  //toString(avg_int, DC_str, 9); // convert frequency to string
							  floatToString(avg_float, DC_str, 2);
							  for (uint32_t i = DELAY; i > 0; i--); 	// wait at least 20 (us) microseconds
							  display_DC(DC_str);
							  UART_ESC_code(BLANK, 'H'); // top left
							  UART_ESC_code("9", 'B'); // 9 down
							  UART_ESC_code("38", 'C'); // 30 right
							  UART_ESC_code("1", 'K'); // clear line to the left
							  display_dcvoltage(avg_float);
							  count = 0;
					}
	  	  	  	}

	  		    //enable timer interrupts in NVIC
			    NVIC->ISER[0] |= (1 << (TIM2_IRQn & 0x1F));
	  	  	  	state = AC;

	  	  case(AC):
				//disable timer interrupts in NVIC
				//NVIC->ISER[0] |= (0 << (TIM2_IRQn & 0x1F));
	  	  	  	//count = 0;
	  		  	for (int i = 0; i < AC_SAMPLE_COUNT; i++) {
					if (count < AC_SAMPLE_COUNT) {
						if (eoc_flag == 1) {
							eoc_flag = 0; // clear the flag used in ISR
							ac_conv_arr[count] = adc_val;
							count += 1;
							ADC1->CR |= ADC_CR_ADSTART; // restart the conversion
						}
					}
					else if (count == AC_SAMPLE_COUNT) {
						//uint32_t rms_value = find_rms(ac_conv_arr);
						uint32_t p2p_value = find_p2p(ac_conv_arr);
						float p2p_float = calibrate_volt(p2p_value);
						if (p2p_float > 3) {
							p2p_float = 3.0;
						}
						float rms_float = (1/(2 * sqrt(2)) * p2p_float);
						char P2P_str[10];
						char RMS_str[10];
						floatToString(p2p_float, P2P_str, 2);
						floatToString(rms_float, RMS_str, 2);
						for (uint32_t i = DELAY; i > 0; i--); 	// wait at least 20 (us) microseconds
						display_P2P(P2P_str);
						display_RMS(RMS_str);
						UART_ESC_code(BLANK, 'H'); // top left
						UART_ESC_code("9", 'B'); // 9 down
						UART_ESC_code("40", 'C'); // 30 right
						UART_ESC_code("0", 'K'); // clear line to the right
						display_acvoltage(p2p_float);
						count = 0;
					}
	  		  	}
	  	  	  	state = WAIT;
				  //enable timer interrupts in NVIC
				  //NVIC->ISER[0] |= (1 << (TIM2_IRQn & 0x1F));

	  }
	  // AC mode one of the states


	 // collect samples with ADC using state machine possibly
  }

} // end of main


void TIM2_IRQHandler(void)
{
    if(TIM2->SR & TIM_SR_CC4IF) // there is an interrupt
    {
        GPIOC->ODR ^= GPIO_ODR_OD0;
        prev_t = time;
        time = (TIM2->CCR4); // storing timer value in global variable
        TIM2->SR &= ~(TIM_SR_CC4IF); // clear flag
        timerflag = 1;
    }
}

void ADC1_2_IRQHandler(void) {
	if((ADC1->ISR & ADC_ISR_EOC) != 0){
		adc_val = ADC1->DR;
		eoc_flag = 1;
	}
}

void display_freq(char *freq_str) {
	UART_ESC_code(BLANK, 'H'); // top left
	UART_print_str("FREQUENCY: ");
	UART_print_str(freq_str);
}

void display_DC(char *DC_voltage) {
	UART_ESC_code(BLANK, 'H'); // top left
	UART_ESC_code("20", 'C'); // right
	UART_print_str("DC Voltage: ");
	UART_print_str(DC_voltage);
}

void display_RMS(char *RMS_voltage) {
	UART_ESC_code(BLANK, 'H'); // top left
	UART_ESC_code("42", 'C'); // 30 right
	UART_print_str("RMS Value: ");
	UART_print_str(RMS_voltage);
}

void display_P2P(char *P2P_voltage) {
	UART_ESC_code(BLANK, 'H'); // top left
	UART_ESC_code("61", 'C'); // 30 right
	UART_print_str("P2P Value: ");
	UART_print_str(P2P_voltage);
}

void display_dcvoltage(float dc_voltage) {
	UART_ESC_code(BLANK, 'H'); // top left
	UART_ESC_code("9", 'B'); // 9 down
	UART_ESC_code("2", 'K'); // clear line
	int volt_int = (int)(dc_voltage * 12);
	for (int i = 0; i < volt_int + 1; i++) {
		UART_print_str("#");
	}
//	UART_ESC_code("9", 'B'); // 9 down
//	UART_ESC_code("38", 'C'); // 1 right
//	UART_ESC_code("1", 'K'); // clear line to the left
}

void display_acvoltage(float ac_voltage) {
	UART_ESC_code(BLANK, 'H'); // top left
	UART_ESC_code("9", 'B'); // 1 down
	//UART_ESC_code("2", 'K'); // clear line
	UART_ESC_code("41", 'C'); // 30 right
	int volt_int = (int)(ac_voltage * 12);
	for (int i = 0; i < volt_int + 1; i++) {
		UART_print_str("#");
	}
}

void build_graph(void) {
	UART_ESC_code("2", 'B'); // 2 down
	//UART_ESC_code("1", 'C'); // 1 right
	UART_ESC_code("15", 'D'); // 15 left
	UART_print_str("|-----------------DC----------------|");
	UART_print_str("  X |-----------------AC----------------|");
	UART_ESC_code("77", 'D'); // left
	for (int i = 0; i < 8; i++) {
		UART_ESC_code("1", 'B'); // 1 down
		UART_ESC_code("38", 'C'); // 1 right
		UART_print_str("X");
		UART_ESC_code("39", 'D'); // left
	}
	UART_ESC_code("50", 'D'); // left
	UART_print_str("|-----|-----|-----|-----|-----|-----|");
	UART_print_str("  X |-----|-----|-----|-----|-----|-----|");
	UART_ESC_code("1", 'B'); // 1 down
	UART_ESC_code("80", 'D'); // left
	//UART_ESC_code("1", 'C'); // 1 right
	UART_print_str("0    0.5   1.0   1.5   2.0   2.5   3.0|X|");
	UART_print_str("0    0.5   1.0   1.5   2.0   2.5   3.0");

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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_9;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
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
