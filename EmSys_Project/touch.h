#ifndef TOUCH_H_
#define TOUCH_H_

#define BUTTON_X_START 3562
#define BUTTON_X_END 4000

#define CURSOR_Y_TOP 1152
#define HOLD_Y_TOP 1728
#define WAVE_Y_TOP 2256
#define TOGGLE_Y_TOP 3200
#define TOGGLE_Y_BOTTOM 3560

#define CURSOR_BUTTON 1
#define HOLD_BUTTON 2
#define WAVE_BUTTON 3
#define TOGGLE_BUTTON 4

void Touch_IRQ_Init(void);
uint16_t touch_spi_transfer(uint8_t command);
uint16_t read_touch_x(void);
uint16_t read_touch_y(void);
uint8_t check_touch_buttons(uint16_t x, uint16_t y);

#endif /* TOUCH_H_ */