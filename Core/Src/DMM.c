#include "main.h"
#include "string.h"
#include "UART.h"
#include "DMM.h"

#define MAX_IDX 9

void timer_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN; // enable clock for GPIOC
    // set up GPIOC0 as output /
    GPIOC->MODER &= ~(GPIO_MODER_MODE0); // clear MODE0
    GPIOC->MODER |= (1 << GPIO_MODER_MODE0_Pos); // set MODE0 to 01 (output mode)
    GPIOC->OTYPER &= ~(GPIO_OTYPER_OT0); // set OTYPE0 to push-pull
    GPIOC->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED0); // set OSPEED0 to low speed
    GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD0); // set PUPD0 to no pull-up/pull-down
    GPIOC->ODR |= (GPIO_ODR_OD0); // set initial value of GPIOC0 to high
    // end of gpioc config/

    // enable timer clock
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

    TIM2->CCMR2 |= (1 << TIM_CCMR2_CC4S_Pos);

    // set filter duration to 4 samples /
    TIM2->CCMR2 &= ~(TIM_CCMR2_IC4F);
    TIM2->CCMR2 |= (0x0011 << TIM_CCMR2_IC4F_Pos);

    // select the edge of active transition /
    TIM2->CCER &= ~(TIM_CCER_CC4NP | TIM_CCER_CC4P | TIM_CCER_CC4E);

    // set pre-scaler to 0
    TIM2->CCMR2 &= ~(TIM_CCMR2_IC4PSC);

    // enable capture/compare enable
    TIM2->CCER |= TIM_CCER_CC4E;
    TIM2->DIER |= TIM_DIER_CC4IE;

    // connecting COMP_OUT to TIM2 */
    TIM2->OR1  &= ~(TIM2_OR1_TI4_RMP);
    TIM2->OR1  |= (1 << TIM2_OR1_TI4_RMP_Pos);

    TIM2->SR &= ~(TIM_SR_UIF | TIM_SR_CC4IF); // clear update interrupt flag

    //enable interrupts in NVIC
    NVIC->ISER[0] |= (1 << (TIM2_IRQn & 0x1F));

    __enable_irq();    // enable interrupts globally

     TIM2->CR1 |= (TIM_CR1_CEN);
}

void toString(uint32_t value, char *str, int max_index)

{
    char nums[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    if (value == 0)
    {
        str[0] = '0';
        str[1] = '\0';
    }
    else
    {
        uint32_t num = value;
        str[MAX_IDX - 1] = '\0'; // setting index 8 to null
        int idx = MAX_IDX - 2;   // start adding from the end of the array
        int size = 0 ;
        // adding individual characters into string /
        while(num)
        {
            char toprint = nums[num % 10];
            str[idx] = toprint;
            idx -= 1;
            num /= 10;
            size += 1;
        }
        // moving stuff so it can be printed */
        if(size < MAX_IDX - 1) // if the number is less than 8 digits
        {
            int gap = MAX_IDX - size - 1;
            for(int i = 0; i < size + 1; i++)
            {
                str[i] = str[i+ gap];
            }
            str[size] = '\0';
        }
    }
}

void comp_init(void) {
    /* GPIOB PB0 output configuration

	GPIOB PB2 input configuration
	GPIOB PB1 as reference voltage configuration*/
	RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOBEN);
	GPIOB->MODER   &= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE2| GPIO_MODER_MODE1); // clearing the registers// set PB0 to alternate function mode, PB2 to analog mode, PB1 to analog mode
	GPIOB->MODER   |= (2 << GPIO_MODER_MODE0_Pos| 3 << GPIO_MODER_MODE2_Pos| 3 << GPIO_MODER_MODE1_Pos);
	GPIOB->OTYPER  &= ~(GPIO_OTYPER_OT0);
	GPIOB->OSPEEDR &= ~(GPIO_OSPEEDR_OSPEED0 | GPIO_OSPEEDR_OSPEED2| GPIO_OSPEEDR_OSPEED1);
	GPIOB->OSPEEDR |=  (3 << GPIO_OSPEEDR_OSPEED0_Pos | 3 << GPIO_OSPEEDR_OSPEED2_Pos| 3 << GPIO_OSPEEDR_OSPEED1_Pos); // set to high speed
	GPIOB->AFR[0]  &= ~(GPIO_AFRL_AFSEL0);         // clear register
	GPIOB->AFR[0]  |=  (12 << GPIO_AFRL_AFSEL0_Pos);   // set AF12 for PB0

    /* comparator configuration */
    COMP1->CSR |= (6 << COMP_CSR_INMSEL_Pos); // set the input selector to PB1
    COMP1->CSR &= ~(COMP_CSR_SCALEN);          // set bit to 01
    COMP1->CSR |= (COMP_CSR_BRGEN);
    COMP1->CSR &= ~(COMP_CSR_POLARITY);       // set polarity to 0
    COMP1->CSR |= (COMP_CSR_INPSEL);          // set to 1, selecting PB2 as the input pin
    COMP1->CSR |= (3 << COMP_CSR_HYST_Pos);   // set to high hysterisis (11) for now
    COMP1->CSR &= ~(COMP_CSR_PWRMODE);        // set to high speed mode


    COMP1->CSR |= (COMP_CSR_EN);              // start comparing
}



