#include "display.h"
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "glcdfont.c"

#define CS_LOW()   DISPLAY_PORT &= ~(1 << DISPLAY_CS)   // driver for setting Chip Select low
#define CS_HIGH()  DISPLAY_PORT |=  (1 << DISPLAY_CS)   // driver for setting Chip Select high
#define DC_LOW()   DISPLAY_PORT &= ~(1 << DISPLAY_DC)   // driver for setting Data/Command low     low  -> command
#define DC_HIGH()  DISPLAY_PORT |=  (1 << DISPLAY_DC)   // driver for setting Data/Command high    high -> data
#define RST_LOW()  DISPLAY_PORT &= ~(1 << DISPLAY_RST)  // driver for setting Reset low
#define RST_HIGH() DISPLAY_PORT |=  (1 << DISPLAY_RST)  // driver for setting Reset high

#define PLOT_X_START   20
#define PLOT_X_END    260
#define PLOT_Y_TOP     67
#define PLOT_Y_BOTTOM 207
#define PLOT_X_SPACING 10  

#define VOLTAGE_X_BEGIN 0
#define VOLTAGE_X_END 184
#define VOLTAGE_Y_BEGIN 0
#define VOLTAGE_Y_END 35

#define MAX_POINTS 25

typedef struct {      // struct which holds the point data, saved after scaling to the voltage and plotting them
	uint16_t x;       // x coord
	uint16_t y;       // y coord
	float value;      // voltage value
} PlotPoint;

PlotPoint plot_coords[MAX_POINTS];
int plot_count = 0;

uint16_t display_width = 240;
uint16_t display_height = 320;

static uint16_t fg_color = 0xFFFF;
static uint16_t bg_color = 0x0000;
static uint8_t text_size = 1;

uint8_t display_rotation = 0;
int cursor_active = 0;
int cursor_position = 0;


void display_send_command(uint8_t cmd) {
	CS_LOW(); 
	DC_LOW(); 
	SPI_transfer(cmd); 
	CS_HIGH();
}

void display_data(uint8_t data) {
	CS_LOW(); 
	DC_HIGH(); 
	SPI_transfer(data);
	 CS_HIGH();
}

void display_command_start(void) {
	DC_LOW();
	CS_LOW();
}

void display_command_end(void) {
	CS_HIGH();
	_delay_us(20);
}

void display_data_start(void) {
	DC_HIGH();
	CS_LOW();
}

void display_data_end(void) {
	CS_HIGH();
	_delay_us(20);
}

void display_write_byte(uint8_t data) {
	SPI_transfer(data);
}

static const uint8_t PROGMEM initcmd[] = { // display initiation sequence
	0xEF, 3, 0x03, 0x80, 0x02,
	0xCF, 3, 0x00, 0xC1, 0x30,
	0xED, 4, 0x64, 0x03, 0x12, 0x81,
	0xE8, 3, 0x85, 0x00, 0x78,
	0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
	0xF7, 1, 0x20,
	0xEA, 2, 0x00, 0x00,
	ILI9341_PWCTR1  , 1, 0x23,
	ILI9341_PWCTR2  , 1, 0x10,
	ILI9341_VMCTR1  , 2, 0x3e, 0x28,
	ILI9341_VMCTR2  , 1, 0x86,
	ILI9341_MADCTL  , 1, 0x48,
	ILI9341_VSCRSADD, 1, 0x00,
	ILI9341_PIXFMT  , 1, 0x55,
	ILI9341_FRMCTR1 , 2, 0x00, 0x18,
	ILI9341_DFUNCTR , 3, 0x08, 0x82, 0x27,
	0xF2, 1, 0x00,
	ILI9341_GAMMASET , 1, 0x01,
	ILI9341_GMCTRP1 , 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08,
	0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
	ILI9341_GMCTRN1 , 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07,
	0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
	ILI9341_SLPOUT  , 0x80,
	ILI9341_DISPON  , 0x80,
	0x00
};

void Display_Init(void) {
	DISPLAY_DDR |= (1 << DISPLAY_CS) | (1 << DISPLAY_DC) | (1 << DISPLAY_RST);
	CS_HIGH();	
	DC_HIGH();
	RST_HIGH();

	RST_LOW();
	_delay_ms(20);
	RST_HIGH();
	_delay_ms(150);

	const uint8_t *addr = initcmd;
	uint8_t cmd, x, numArgs;
	while ((cmd = pgm_read_byte(addr++)) > 0) {
		x = pgm_read_byte(addr++);
		numArgs = x & 0x7F;

		display_command_start();
		display_write_byte(cmd);
		display_command_end();

		if (numArgs) {
			display_data_start();
			for (uint8_t i = 0; i < numArgs; i++) {
				display_write_byte(pgm_read_byte(addr++));
			}
			display_data_end();
		}

		if (x & 0x80) {
			_delay_ms(150);
		}
	}
}

void display_set_rotation(uint8_t mode) {  // set the display orientation using MADCTL (Memory Acess Control Reg)
	display_rotation = mode % 4;
	uint8_t madctl = 0;

	switch (display_rotation) {
		case 0: 
			madctl = MADCTL_MX | MADCTL_BGR; 
			display_width = 240; 
			display_height = 320; 
			break;
		case 1: 
			madctl = MADCTL_MV | MADCTL_BGR; 
			display_width = 320; 
			display_height = 240; 
			break;
		case 2: 
			madctl = MADCTL_MY | MADCTL_BGR; 
			display_width = 240; 
			display_height = 320; 
			break;
		case 3: 
			madctl = MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR; 
			display_width = 320; 
			display_height = 240; 
			break;
	}

	display_send_command(ILI9341_MADCTL);
	display_data(madctl);
}

void display_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
	uint16_t x2 = x + w - 1;
	uint16_t y2 = y + h - 1;

	display_send_command(ILI9341_CASET);
	display_data(x >> 8); display_data(x & 0xFF);
	display_data(x2 >> 8); display_data(x2 & 0xFF);

	display_send_command(ILI9341_PASET);
	display_data(y >> 8); display_data(y & 0xFF);
	display_data(y2 >> 8); display_data(y2 & 0xFF);

	display_send_command(ILI9341_RAMWR);
}

void set_text_size(uint8_t size) {
	if (size < 1) size = 1;
	text_size = size;
}

void display_set_color(uint16_t color) {
	fg_color = color;
}

void display_set_background(uint16_t color) {
	bg_color = color;
}

void display_write_color(uint16_t color) {
	display_data_start();
	display_write_byte(color >> 8);
	display_write_byte(color & 0xFF);
	display_data_end();
}

void display_write(void) {
	display_write_color(fg_color);
}

void display_write_background(void) {
	display_write_color(bg_color);
}

void display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
	uint8_t hi = color >> 8;
	uint8_t lo = color & 0xFF;

	display_set_window(x, y, w, h);
	display_send_command(ILI9341_RAMWR);
	display_data_start();

	for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
		SPI_transfer(hi);
		SPI_transfer(lo);
	}

	display_data_end();
}

void display_draw_char(char c, uint16_t x, uint16_t y) {
	if (c < 32 || c > 127) return;

	for (uint8_t col = 0; col < 5; col++) {
		uint8_t line = pgm_read_byte(&font[c * 5 + col]);
		for (uint8_t row = 0; row < 8; row++) {
			uint16_t px = (line & (1 << row)) ? fg_color : bg_color;
			display_fill_rect(x + col * text_size, y + row * text_size, text_size, text_size, px);
		}
	}

	// Add spacing
	display_fill_rect(x + 5 * text_size, y, text_size, 8 * text_size, bg_color);

}

void display_print(const char* str, uint16_t x, uint16_t y) {
	while (*str) {
		display_draw_char(*str++, x, y);
		x += 6 * text_size;
	}
}

void display_fill_color(uint16_t color) {
	uint8_t hi = color >> 8;
	uint8_t lo = color & 0xFF;

	display_set_window(0, 0, display_width, display_height);
	display_send_command(ILI9341_RAMWR);
	display_data_start();

	for (uint32_t i = 0; i < (uint32_t)display_width * display_height; i++) {
		SPI_transfer(hi);
		SPI_transfer(lo);
	}

	display_data_end();
}

void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
	display_fill_rect(x, y, 1, 1, color);
}

void display_draw_line(int x0, int y0, int x1, int y1, uint16_t color) {
	int dx = abs(x1 - x0);
	int dy = -abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx + dy;

	while (1) {
		display_draw_pixel(x0, y0, color);
		if (x0 == x1 && y0 == y1) break;
		int e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
	}
}

uint16_t scale_voltage_to_y(float value, float v_min, float v_max) {   // scale the viewform viewer's range automatically to better see the wave variations when differences are small
	if (value < v_min) value = v_min;
	if (value > v_max) value = v_max;

	float scale = (value - v_min) / (v_max - v_min);
	return PLOT_Y_BOTTOM - (uint16_t)(scale * (PLOT_Y_BOTTOM - PLOT_Y_TOP));
}


void plot_points_digital(float points[], int count, float v_max, float v_min) {
	display_fill_rect(PLOT_X_START - 5, PLOT_Y_TOP - 5, PLOT_X_END - PLOT_X_START + 5, PLOT_Y_BOTTOM - PLOT_Y_TOP + 5, COLOR_BLACK);
	
	if (count < 1) return;

	uint16_t x = PLOT_X_START;
	plot_count = (count > MAX_POINTS) ? MAX_POINTS : count;

	for (int i = 0; i < plot_count; i++) {                        // go through every point
		uint16_t y = scale_voltage_to_y(points[i], v_min, v_max);

		plot_coords[i].x = x;             // save coordinates and value inside the struct array
		plot_coords[i].y = y;
		plot_coords[i].value = points[i];

		display_draw_line(x, 207, x, y, COLOR_GREEN); // draw vertical line from bottom to voltage level

		if (i < plot_count - 1) {                                             // horizontal line to connecting the points
			uint16_t next_y = scale_voltage_to_y(points[i + 1], v_min, v_max);
			display_draw_line(x, y, x + 10, next_y, COLOR_GREEN);
		}

		display_draw_line(x - 2, y - 2, x + 2, y + 2, COLOR_BLUE); // draw X marker on point position
		display_draw_line(x - 2, y + 2, x + 2, y - 2, COLOR_BLUE);

		x += PLOT_X_SPACING;
		if (x > PLOT_X_END) break;
	}
	
	if(cursor_active)
		draw_cursor();
}

void plot_points_line(float points[], int count, float v_max, float v_min) {
	display_fill_rect(PLOT_X_START - 2, PLOT_Y_TOP - 2, PLOT_X_END - PLOT_X_START + 5, PLOT_Y_BOTTOM - PLOT_Y_TOP + 5, COLOR_BLACK);

	if (count < 2) return;

	uint16_t x = PLOT_X_START;
	plot_count = (count > MAX_POINTS) ? MAX_POINTS : count;

	for (int i = 0; i < plot_count; i++) {
    uint16_t x0 = x;
    uint16_t y0 = scale_voltage_to_y(points[i], v_min, v_max);

    plot_coords[i].x = x0;
    plot_coords[i].y = y0;
    plot_coords[i].value = points[i];

    // Draw X marker
    display_draw_line(x0 - 2, y0 - 2, x0 + 2, y0 + 2, COLOR_BLUE);
    display_draw_line(x0 - 2, y0 + 2, x0 + 2, y0 - 2, COLOR_BLUE);

    // Draw line to next point if it exists
    if (i < plot_count - 1) {
        uint16_t x1 = x + PLOT_X_SPACING;
        uint16_t y1 = scale_voltage_to_y(points[i + 1], v_min, v_max);
        display_draw_line(x0, y0, x1, y1, COLOR_GREEN);
    }

    x += PLOT_X_SPACING;
    if (x > PLOT_X_END) break;
}
	
	if(cursor_active)
		draw_cursor();
}

void clear_plot(){ // clear the waveform viewer area
	display_fill_rect(PLOT_X_START - 2, PLOT_Y_TOP - 20, PLOT_X_END - PLOT_X_START + 5, PLOT_Y_BOTTOM + 20, COLOR_BLACK);
}

void draw_cursor() {
	cursor_active = 1;

	uint16_t x = plot_coords[cursor_position].x;
	uint16_t y = plot_coords[cursor_position].y;

	display_draw_line(x, PLOT_Y_TOP, x, PLOT_Y_BOTTOM, COLOR_RED);

	// draw red X at cursor point
	display_draw_line(x - 2, y - 2, x + 2, y + 2, COLOR_RED);
	display_draw_line(x - 2, y + 2, x + 2, y - 2, COLOR_RED);
}

void erase_cursor(){
	display_draw_line(plot_coords[cursor_position].x, PLOT_Y_TOP, plot_coords[cursor_position].x, PLOT_Y_BOTTOM, COLOR_BLACK);
	display_draw_line(plot_coords[cursor_position].x - 2, plot_coords[cursor_position].y - 2, plot_coords[cursor_position].x + 2, plot_coords[cursor_position].y + 2, COLOR_BLUE);
	display_draw_line(plot_coords[cursor_position].x - 2, plot_coords[cursor_position].y + 2, plot_coords[cursor_position].x + 2, plot_coords[cursor_position].y - 2, COLOR_BLUE);
}

void move_cursor(int direction) {
	if (!cursor_active || plot_count == 0) return;

	erase_cursor();

	// restore blue X at old point
	display_draw_line(plot_coords[cursor_position].x - 2, plot_coords[cursor_position].y - 2, plot_coords[cursor_position].x + 2, plot_coords[cursor_position].y + 2, COLOR_BLUE);
	display_draw_line(plot_coords[cursor_position].x - 2, plot_coords[cursor_position].y + 2, plot_coords[cursor_position].x + 2, plot_coords[cursor_position].y - 2, COLOR_BLUE);

	// update cursor position
	cursor_position += direction;
	if (cursor_position < 0) cursor_position = 0;
	if (cursor_position >= plot_count) cursor_position = plot_count - 1;

	// draw new cursor line
	uint16_t new_x = plot_coords[cursor_position].x;
	uint16_t new_y = plot_coords[cursor_position].y;
	display_draw_line(new_x, PLOT_Y_TOP, new_x, PLOT_Y_BOTTOM, COLOR_RED);

	// draw red X at new point
	display_draw_line(new_x - 2, new_y - 2, new_x + 2, new_y + 2, COLOR_RED);
	display_draw_line(new_x - 2, new_y + 2, new_x + 2, new_y - 2, COLOR_RED);
}

float get_cursor_voltage(){
	return plot_coords[cursor_position].value;
}

void draw_indicator_leds(float voltage){
	set_text_size(2);
	int led_count = (int)(voltage / 2.4f);  // led every 2.4V
	if (led_count > 10) 
		led_count = 10;
	
	int current_x = 200;
	for(int i = 0; i < 10; i++){
		if(i < 6)
			display_set_color(COLOR_GREEN);
		else if(i < 9)
			display_set_color(COLOR_ORANGE);
		else
			display_set_color(COLOR_RED);
		if(i < led_count){
			display_print("*", current_x + i * 12, 5);
			display_print("*", current_x + i * 12, 15);	
		}else{
			display_print(" ", current_x + i * 12, 5);
			display_print(" ", current_x + i * 12, 15);
		}
	}
}

void erase_voltage_zone(){
	display_fill_rect(VOLTAGE_X_BEGIN, VOLTAGE_Y_BEGIN, VOLTAGE_X_END, VOLTAGE_Y_END, COLOR_BLACK);
}



