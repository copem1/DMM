#ifndef SRC_UART_H_
#define SRC_UART_H_

void UART_init(void);
void UART_print(char data);
void UART_print_str(char *str);
void UART_ESC_code(char *num, char letter);


#endif /* SRC_UART_H_ */
