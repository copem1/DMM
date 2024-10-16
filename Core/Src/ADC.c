#include "main.h"
#include "ADC.h"
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#define OUT_PORT GPIOA
#define ADC_PORT GPIOA
#define DELAY 40000
#define DC_SAMPLE_COUNT 1838
#define AC_SAMPLE_COUNT 36764

void ADC_init(void) {
	// ----------------- Configure GPIOA -----------------
	// ADC12_IN5 -> PA0
	// enable clock for GPIOA
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	// enable analog switch register for PA0 (channel)
	ADC_PORT->ASCR |= GPIO_ASCR_ASC0;
	// set mode to analog mode (11)
	ADC_PORT->MODER &= ~(GPIO_MODER_MODE0);
	ADC_PORT->MODER |= (3 << GPIO_MODER_MODE0_Pos);
	// Fast Speed (11)
	ADC_PORT->OSPEEDR &= ~(GPIO_MODER_MODE0);
	ADC_PORT->OSPEEDR |= (3 << GPIO_OSPEEDR_OSPEED0_Pos);


	// ----------------- Configure ADC -----------------
	// enable ADC clock
	RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;

	// ADC will run at the same speed as CPE (HCLK / 1), Prescaler = 1
	ADC123_COMMON->CCR = (1 << ADC_CCR_CKMODE_Pos);

	// Power up ADC
	ADC1->CR &= ~(ADC_CR_DEEPPWD); 				// set deep power down mode low
	ADC1->CR |= (ADC_CR_ADVREGEN); 				// set voltage regulator high
	for (uint32_t i = DELAY; i > 0; i--); 			// wait at least 20 (us) microseconds

	// calibrate the ADC
	ADC1->CR &= ~(ADC_CR_ADEN | ADC_CR_ADCALDIF); // ensure ADC is disabled and single-ended mode
	ADC1->CR |= (ADC_CR_ADCAL); 					// start calibration
	while (ADC1->CR & ADC_CR_ADCAL); 				// wait for calibration to finish

	// configure for single-ended mode on channel 5
	// must be set before enabling the ADC
	ADC1->DIFSEL &= ~(ADC_DIFSEL_DIFSEL_5); 		// using ADC123_IN5 channel

	// enable ADC (writing a 1, sets to 0)
	ADC1->ISR |= (ADC_ISR_ADRDY); 				// clear ready bit with a 1
	ADC1->CR |= (ADC_CR_ADEN); 					// enable ADC
	while (!(ADC1->ISR & ADC_ISR_ADRDY)); 		// wait for ready flag = 1
	ADC1->ISR |= (ADC_ISR_ADRDY);				 	// clear ready bit with a 1

	// check caution on p.521 for GPIOx_ASCR register

	// configure ADC
	ADC1->SQR1 = (5 << ADC_SQR1_SQ1_Pos);  		// set sequence 1 for 1 conversion on channel 5

	// 12-bit resolution, software trigger, right align, single conversion mode
	ADC1->CFGR = 0;

	// sampling time for channel 5 to 2.5 clocks
	ADC1->SMPR1 = 0x111;

	// enable interrupts for end of conversion
	ADC1->IER |= (ADC_IER_EOC);
	ADC1->ISR |= (ADC_ISR_EOC); 					// clear flag with a 1

	// enable interrupt in the NVIC
	NVIC->ISER[0] = (1 << (ADC1_2_IRQn & 0x1F));

	// enable interrupts globally
	__enable_irq();

	// configure GPIO for channel 5
	/////////////////////////////////////////////////


	// start a conversion
	ADC1->CR |= (ADC_CR_ADSTART);


}

// TOTAL_CNT = 20

uint16_t find_max(uint16_t arr[]) {
	uint16_t pos = 0;
	uint16_t max = arr[pos];
	while (pos < AC_SAMPLE_COUNT){
		if (arr[pos] > max){
			max = arr[pos];
		}
		pos++;
	}
	return max;
}

uint16_t find_min(uint16_t arr[]){
	uint16_t pos = 0;
	uint16_t min = arr[pos];
	while (pos < AC_SAMPLE_COUNT){
		if (arr[pos] < min){
			min = arr[pos];
		}
		pos++;
	}
	return min;
}

uint32_t find_avg(uint16_t arr[]){
	uint16_t pos = 0;
	uint32_t val = 0;
	while (pos < DC_SAMPLE_COUNT){
		val += arr[pos];
		pos++;
	}
	uint32_t avg = val / DC_SAMPLE_COUNT;
	return avg;
}


uint32_t find_rms(uint16_t arr[]){
	uint16_t pos = 0;
	uint32_t val = 0;
	while (pos < AC_SAMPLE_COUNT){
		val += (arr[pos] * arr[pos]);
		pos++;
	}
	uint32_t avg = sqrt((val / AC_SAMPLE_COUNT));
	return avg;
}

uint32_t find_p2p(uint16_t arr[]){
	uint32_t p2p_value = find_max(arr) - find_min(arr);
	return p2p_value;
}


float calibrate_volt(uint16_t num){
	float res = (821 * num - 5060) / 1000000.0;
	return res;
}

// Converts a float to a string with the specified number of decimal places
void floatToString(float num, char* result, int decimalPlaces) {
	int i = 0;
	int integerPart = (int)num;
	float decimalPart = num - integerPart;
	// Handle negative numbers
	if (integerPart < 0) {
		result[i++] = '-';
		integerPart = -integerPart;
		decimalPart = -decimalPart;
	}
	// Convert integer part to string
	int j = i;
	do {
		result[j++] = integerPart % 10 + '0';
		integerPart /= 10;
	} while (integerPart);
	// Reverse the integer part
	for (int k = i; k < j / 2; k++) {
		char temp = result[k];
		result[k] = result[j - k - 1];
		result[j - k - 1] = temp;
	}
	// Add decimal point if required
	if (decimalPlaces > 0) {
		result[j++] = '.';
	}
	// Convert decimal part to string
	for (int k = 0; k < decimalPlaces; k++) {
		decimalPart *= 10;
		result[j++] = (int)decimalPart % 10 + '0';
	}
	// Null terminate the string
	result[j] = '\0';
}

//uint16_t ADC_compute_voltage(uint16_t sample) {
//	/* Convert measured value into milli-volts */
//	return sample * (3300.0/4095.0);
//}
//
//float ADC_compute_voltagef(uint16_t sample) {
//	/* Convert measured value into volts */
//	return sample * (3.30/4095.0);
//}



