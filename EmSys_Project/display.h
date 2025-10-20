#ifndef DISPLAY_H
#define DISPLAY_H

#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>

// Pin definitions (match Arduino wiring)
#define DISPLAY_CS   PB6  // Arduino pin 12
#define DISPLAY_RST  PB5  // Arduino pin 11
#define DISPLAY_DC   PB4  // Arduino pin 10

#define DISPLAY_PORT PORTB
#define DISPLAY_DDR  DDRB

#define ILI9341_SWRESET   0x01
#define ILI9341_SLPOUT    0x11
#define ILI9341_DISPON    0x29
#define ILI9341_CASET     0x2A
#define ILI9341_PASET     0x2B
#define ILI9341_RAMWR     0x2C
#define ILI9341_MADCTL    0x36
#define ILI9341_PIXFMT    0x3A
#define ILI9341_FRMCTR1   0xB1
#define ILI9341_DFUNCTR   0xB6
#define ILI9341_PWCTR1    0xC0
#define ILI9341_PWCTR2    0xC1
#define ILI9341_VMCTR1    0xC5
#define ILI9341_VMCTR2    0xC7
#define ILI9341_GAMMASET  0x26
#define ILI9341_GMCTRP1   0xE0
#define ILI9341_GMCTRN1   0xE1
#define ILI9341_VSCRDEF   0x33
#define ILI9341_VSCRSADD  0x37

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_BGR 0x08

#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_ORANGE  0xFD20 

// Initialization
void Display_Init(void);

// Command/data helpers
void display_send_command(uint8_t cmd);
void display_data(uint8_t data);
void display_command_start(void);
void display_command_end(void);
void display_data_start(void);
void display_data_end(void);
void display_write_byte(uint8_t data);

// Drawing
void display_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void display_set_rotation(uint8_t mode);

void set_text_size(uint8_t size);

// Color control
void display_set_color(uint16_t color);
void display_set_background(uint16_t color);

// Pixel-level color write
void display_write_color(uint16_t color);
void display_write(void);
void display_write_background(void);

// Text rendering
void display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void display_draw_char(char c, uint16_t x, uint16_t y);
void display_print(const char* str, uint16_t x, uint16_t y);

void display_fill_color(uint16_t color);

void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void display_draw_line(int x0, int y0, int x1, int y1, uint16_t color);

uint16_t scale_voltage_to_y(float value, float v_min, float v_max);
void plot_points_digital(float points[], int count, float v_max, float v_min);
void plot_points_line(float points[], int count, float v_max, float v_min);
void clear_plot();

void draw_cursor();
void erase_cursor();
void move_cursor(int direction);
float get_cursor_voltage();

void draw_indicator_leds(float voltage);

void erase_voltage_zone();
#endif
