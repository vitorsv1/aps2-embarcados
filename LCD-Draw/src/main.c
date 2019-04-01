#include "asf.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ioport.h"
#include "logo.h"
#include "icones/lavagem.h"
#include "icones/lock.h"
#include "icones/unlock.h"
#include "icones/lock-grey.h"
#include "icones/unlock-grey.h"
#include "conf_board.h"
#include "conf_example.h"
#include "conf_uart_serial.h"

typedef struct {
	const uint8_t *data;
	uint16_t width;
	uint16_t height;
	uint8_t dataSize;
} tImage;

const uint32_t BUTTON_LOCK_X = 220;
const uint32_t BUTTON_LOCK_SIZE = 93;


#define STRING_EOL    "\r\n"
#define STRING_HEADER "-- SAME70 LCD DEMO --"STRING_EOL	\
	"-- "BOARD_NAME " --"STRING_EOL	\
	"-- Compiled: "__DATE__ " "__TIME__ " --"STRING_EOL

struct ili9488_opt_t g_ili9488_display_opt;

/**
}
/**
 * \brief Configure UART console.
 */
static void configure_console(void)
{
	const usart_serial_options_t uart_serial_options = {
		.baudrate =		CONF_UART_BAUDRATE,
		.charlength =	CONF_UART_CHAR_LENGTH,
		.paritytype =	CONF_UART_PARITY,
		.stopbits =		CONF_UART_STOP_BITS,
	};

	/* Configure UART console. */
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	stdio_serial_init(CONF_UART, &uart_serial_options);
}

void draw_locked(uint32_t clicked){
	static uint32_t last_state = 255;
	if(clicked == last_state) return;
	
	// desenha imagem cadeado fechado na posicao X=5 e Y=150
	//ili9488_draw_pixmap(20, 1, 93, 93, image_data_lock);
		
	// desenha imagem cadeado aberto na posicao X= e Y=150
	ili9488_draw_pixmap(220, 1, 93, 93, image_data_unlock);
	
	if(clicked) {
		ili9488_draw_pixmap(220, 1, 93, 93, image_data_unlockgrey);
		} else {
		ili9488_set_foreground_color(COLOR_CONVERT(COLOR_GREEN));
		ili9488_draw_filled_rectangle(220,1,93,93);
	}
	last_state = clicked;
}

void update_screen(uint32_t tx, uint32_t ty) {
	if(tx >=  && tx <= BUTTON_X + BUTTON_W/2) {
		if(ty >= BUTTON_Y-BUTTON_H/2 && ty <= BUTTON_Y) {
			draw_button(0);
			} else if(ty > BUTTON_Y && ty < BUTTON_Y + BUTTON_H/2) {
			draw_button(1);
		}
	}
}


static void configure_lcd(void){
	/* Initialize display parameter */
	g_ili9488_display_opt.ul_width = ILI9488_LCD_WIDTH;
	g_ili9488_display_opt.ul_height = ILI9488_LCD_HEIGHT;
	g_ili9488_display_opt.foreground_color = COLOR_CONVERT(COLOR_WHITE);
	g_ili9488_display_opt.background_color = COLOR_CONVERT(COLOR_WHITE);

	/* Initialize LCD */
	ili9488_init(&g_ili9488_display_opt);
	ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH-1, ILI9488_LCD_HEIGHT-1);
	//ili9488_set_foreground_color(COLOR_CONVERT(COLOR_TOMATO));
	//ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH-1, 120-1);
	//ili9488_draw_filled_rectangle(0, 360, ILI9488_LCD_WIDTH-1, 480-1);
	//ili9488_draw_pixmap(0, 50, 319, 129, logoImage);
	
}

void mxt_handler(struct mxt_device *device)
{
	/* USART tx buffer initialized to 0 */
	char tx_buf[STRING_LENGTH * MAX_ENTRIES] = {0};
	uint8_t i = 0; /* Iterator */

	/* Temporary touch event data struct */
	struct mxt_touch_event touch_event;

	/* Collect touch events and put the data in a string,
	 * maximum 2 events at the time */
	do {
		/* Temporary buffer for each new touch event line */
		char buf[STRING_LENGTH];
	
		/* Read next next touch event in the queue, discard if read fails */
		if (mxt_read_touch_event(device, &touch_event) != STATUS_OK) {
			continue;
		}
		
		 // eixos trocados (quando na vertical LCD)
		uint32_t conv_x = convert_axis_system_x(touch_event.y);
		uint32_t conv_y = convert_axis_system_y(touch_event.x);
		
		/* Format a new entry in the data string that will be sent over USART */
		sprintf(buf, "Nr: %1d, X:%4d, Y:%4d, Status:0x%2x conv X:%3d Y:%3d\n\r",
				touch_event.id, touch_event.x, touch_event.y,
				touch_event.status, conv_x, conv_y);
		update_screen(conv_x, conv_y);

		/* Add the new string to the string buffer */
		strcat(tx_buf, buf);
		i++;

		/* Check if there is still messages in the queue and
		 * if we have reached the maximum numbers of events */
	} while ((mxt_is_message_pending(device)) & (i < MAX_ENTRIES));

	/* If there is any entries in the buffer, send them over USART */
	if (i > 0) {
		usart_serial_write_packet(USART_SERIAL_EXAMPLE, (uint8_t *)tx_buf, strlen(tx_buf));
	}
}

/**
 * \brief Main application function.
 *
 * Initialize system, UART console, network then start weather client.
 *
 * \return Program return value.
 */
int main(void)
{
	// array para escrita no LCD
	uint8_t stingLCD[256];
	
	/* Initialize the board. */
	sysclk_init();
	board_init();
	ioport_init();
	
	/* Initialize the UART console. */
	configure_console();
	printf(STRING_HEADER);

    /* Inicializa e configura o LCD */
	configure_lcd();

    /* Escreve na tela Computacao Embarcada 2018 */
	//ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
	//ili9488_draw_filled_rectangle(0, 300, ILI9488_LCD_WIDTH-1, 315);
	//ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLACK));
	
	//sprintf(stingLCD, "Computacao Embarcada %d", 2018);
	//ili9488_draw_string(10, 300, stingLCD);
	
	// desenha imagem lavagem na posicao X=80 e Y=150
	ili9488_draw_pixmap(120, 1, 93, 93, image_data_lavagem);
	

	
	// Linha para separar
	ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLACK));
	ili9488_draw_line(0, 100, ILI9488_LCD_WIDTH-1, 100);
	
	while (1) {
		
		if (mxt_is_message_pending(&device)) {
			mxt_handler(&device);
		}
	}
	return 0;
}
