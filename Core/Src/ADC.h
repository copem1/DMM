#ifndef SRC_ADC_H_
#define SRC_ADC_H_
#define TOTAL_CNT 20

void ADC_init(void);
uint16_t find_max(uint16_t arr[]);
uint16_t find_min(uint16_t arr[]);
uint32_t find_avg(uint16_t arr[]);
uint32_t find_rms(uint16_t arr[]);
uint32_t find_p2p(uint16_t arr[]);
float calibrate_volt(uint16_t num);
void floatToString(float num, char* result, int decimalPlaces);
//uint16_t ADC_compute_voltage(uint16_t sample);
//float ADC_compute_voltagef(uint16_t sample);


#endif /* SRC_ADC_H_ */
