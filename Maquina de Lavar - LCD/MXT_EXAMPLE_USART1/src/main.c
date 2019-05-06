/**
 * \file
 *
 * \brief Example of usage of the maXTouch component with USART
 *
 * This example shows how to receive touch data from a maXTouch device
 * using the maXTouch component, and display them in a terminal window by using
 * the USART driver.
 *
 * Copyright (c) 2014-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

/**
 * \mainpage
 *
 * \section intro Introduction
 * This simple example reads data from the maXTouch device and sends it over
 * USART as ASCII formatted text.
 *
 * \section files Main files:
 * - example_usart.c: maXTouch component USART example file
 * - conf_mxt.h: configuration of the maXTouch component
 * - conf_board.h: configuration of board
 * - conf_clock.h: configuration of system clock
 * - conf_example.h: configuration of example
 * - conf_sleepmgr.h: configuration of sleep manager
 * - conf_twim.h: configuration of TWI driver
 * - conf_usart_serial.h: configuration of USART driver
 *
 * \section apiinfo maXTouch low level component API
 * The maXTouch component API can be found \ref mxt_group "here".
 *
 * \section deviceinfo Device Info
 * All UC3 and Xmega devices with a TWI module can be used with this component
 *
 * \section exampledescription Description of the example
 * This example will read data from the connected maXTouch explained board
 * over TWI. This data is then processed and sent over a USART data line
 * to the board controller. The board controller will create a USB CDC class
 * object on the host computer and repeat the incoming USART data from the
 * main controller to the host. On the host this object should appear as a
 * serial port object (COMx on windows, /dev/ttyxxx on your chosen Linux flavour).
 *
 * Connect a terminal application to the serial port object with the settings
 * Baud: 57600
 * Data bits: 8-bit
 * Stop bits: 1 bit
 * Parity: None
 *
 * \section compinfo Compilation Info
 * This software was written for the GNU GCC and IAR for AVR.
 * Other compilers may or may not work.
 *
 * \section contactinfo Contact Information
 * For further information, visit
 * <A href="http://www.atmel.com/">Atmel</A>.\n
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

//############################################################################################################
// INCLUDES

#include <asf.h>
#include <includes.h>

//############################################################################################################
// DEFINES

#define YEAR        0
#define MOUNTH      0
#define DAY         0
#define WEEK        0
#define HOUR        0
#define MINUTE      0
#define SECOND      0

#define MAX_ENTRIES        3
#define STRING_LENGTH     70
#define USART_TX_MAX_LENGTH     0xff

#define LED_PIO_ID	   ID_PIOC
#define LED_PIO        PIOC
#define LED_PIN		   8
#define LED_PIN_MASK   (1<<LED_PIN)

#define BUT_PIO_ID	   ID_PIOA
#define BUT_PIO        PIOA
#define BUT_PIN		   11
#define BUT_PIN_MASK   (1<<BUT_PIN)

//############################################################################################################
// STRUCTS
typedef struct {
	uint8_t   state;
	tImage icon1;
	tImage icon2;
	uint16_t x0;
	uint16_t y0;
	void (*callback)();
} button;

struct ili9488_opt_t g_ili9488_display_opt;

//###############################################################################################################
//VARIAVEIS GLOBAIS
volatile uint8_t locked = 1;
volatile uint8_t flag_led = 0;
volatile uint8_t wash_mode = 0;
volatile uint8_t cleanScreen = 0;
volatile uint8_t isWashing = 0;
volatile uint32_t minute = 0;
volatile uint32_t second = 0;
volatile uint8_t washingLockScreen = 0;

button *buttons2[] ;

int wash_times[] = {0,0,0,0,0};
//###############################################################################################################
//CONFIGURAR E ETC

//DESENHA A FONTE EM TEXTO NA TELA
void font_draw_text(tFont *font, const char *text, int x, int y, int spacing) {
	char *p = text;
	while(*p != NULL) {
		char letter = *p;
		int letter_offset = letter - font->start_char;
		if(letter <= font->end_char) {
			tChar *current_char = font->chars + letter_offset;
			ili9488_draw_pixmap(x, y, current_char->image->width, current_char->image->height, current_char->image->data);
			x += current_char->image->width + spacing;
		}
		p++;
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
}

/**
 * \brief Set maXTouch configuration
 *
 * This function writes a set of predefined, optimal maXTouch configuration data
 * to the maXTouch Xplained Pro.
 *
 * \param device Pointer to mxt_device struct
 */
static void mxt_init(struct mxt_device *device)
{
	enum status_code status;

	/* T8 configuration object data */
	uint8_t t8_object[] = {
		0x0d, 0x00, 0x05, 0x0a, 0x4b, 0x00, 0x00,
		0x00, 0x32, 0x19
	};

	/* T9 configuration object data */
	uint8_t t9_object[] = {
		0x8B, 0x00, 0x00, 0x0E, 0x08, 0x00, 0x80,
		0x32, 0x05, 0x02, 0x0A, 0x03, 0x03, 0x20,
		0x02, 0x0F, 0x0F, 0x0A, 0x00, 0x00, 0x00,
		0x00, 0x18, 0x18, 0x20, 0x20, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x02,
		0x02
	};

	/* T46 configuration object data */
	uint8_t t46_object[] = {
		0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x03,
		0x00, 0x00
	};
	
	/* T56 configuration object data */
	uint8_t t56_object[] = {
		0x02, 0x00, 0x01, 0x18, 0x1E, 0x1E, 0x1E,
		0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
		0x1E, 0x1E, 0x1E, 0x1E, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00
	};

	/* TWI configuration */
	twihs_master_options_t twi_opt = {
		.speed = MXT_TWI_SPEED,
		.chip  = MAXTOUCH_TWI_ADDRESS,
	};

	status = (enum status_code)twihs_master_setup(MAXTOUCH_TWI_INTERFACE, &twi_opt);
	Assert(status == STATUS_OK);

	/* Initialize the maXTouch device */
	status = mxt_init_device(device, MAXTOUCH_TWI_INTERFACE,
			MAXTOUCH_TWI_ADDRESS, MAXTOUCH_XPRO_CHG_PIO);
	Assert(status == STATUS_OK);

	/* Issue soft reset of maXTouch device by writing a non-zero value to
	 * the reset register */
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_COMMANDPROCESSOR_T6, 0)
			+ MXT_GEN_COMMANDPROCESSOR_RESET, 0x01);

	/* Wait for the reset of the device to complete */
	delay_ms(MXT_RESET_TIME);

	/* Write data to configuration registers in T7 configuration object */
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_POWERCONFIG_T7, 0) + 0, 0x20);
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_POWERCONFIG_T7, 0) + 1, 0x10);
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_POWERCONFIG_T7, 0) + 2, 0x4b);
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_POWERCONFIG_T7, 0) + 3, 0x84);

	/* Write predefined configuration data to configuration objects */
	mxt_write_config_object(device, mxt_get_object_address(device,
			MXT_GEN_ACQUISITIONCONFIG_T8, 0), &t8_object);
	mxt_write_config_object(device, mxt_get_object_address(device,
			MXT_TOUCH_MULTITOUCHSCREEN_T9, 0), &t9_object);
	mxt_write_config_object(device, mxt_get_object_address(device,
			MXT_SPT_CTE_CONFIGURATION_T46, 0), &t46_object);
	mxt_write_config_object(device, mxt_get_object_address(device,
			MXT_PROCI_SHIELDLESS_T56, 0), &t56_object);

	/* Issue recalibration command to maXTouch device by writing a non-zero
	 * value to the calibrate register */
	mxt_write_config_reg(device, mxt_get_object_address(device,
			MXT_GEN_COMMANDPROCESSOR_T6, 0)
			+ MXT_GEN_COMMANDPROCESSOR_CALIBRATE, 0x01);
}

//MUDA O VALOR NO PINO
void pin_toggle(Pio *pio, uint32_t mask){
	if(pio_get_output_data_status(pio, mask))
	pio_clear(pio, mask);
	else
	pio_set(pio,mask);
}

//###############################################################################################################

//DESENHA A TELA BRANCA DE FUNDO
void draw_screen(void) {
	ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
	ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH-1, ILI9488_LCD_HEIGHT-1);
}

//PINTA QUADRADO BRANCO SEM O LOCK
void draw_lockscreen(void) {
	ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
	ili9488_draw_filled_rectangle(93, 0, ILI9488_LCD_WIDTH-1, 93);
	ili9488_draw_filled_rectangle(0, 94, ILI9488_LCD_WIDTH-1, ILI9488_LCD_HEIGHT-1);
}

//TROCA O ICON DO BUTTON DESENHADO
void draw_icon_button(button b) {
	if(b.state == 2) {
		ili9488_draw_pixmap(b.x0, b.y0, b.icon2.width, b.icon2.height, b.icon2.data);
	} else if(b.state == 1){
		ili9488_draw_pixmap(b.x0, b.y0, b.icon1.width, b.icon1.height, b.icon1.data);
	}
}

//DESENHA DE ACORDO COM STRUCT
void draw_wash_mode(t_ciclo cicles[] ,uint8_t mode) {
	
	if (mode >= 0){
		
		char nome[32];
		char enxagueTempo[32];
		char enxagueQnt[32];
		char centrifugacaoRPM[32];
		char centrifugacaoTempo[32];
		
		//LIMPAR A TELA
		if (cleanScreen){
			ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
			ili9488_draw_filled_rectangle(20, 100, ILI9488_LCD_WIDTH-1, 235);
			cleanScreen = 0;
		}
		sprintf(nome,"%s",cicles[mode].nome);
		font_draw_text(&calibri_24, nome, 20, 100, 1);
		
		sprintf(enxagueTempo,"%d minutos",cicles[mode].enxagueTempo);
		font_draw_text(&calibri_24, enxagueTempo, 20, 130, 1);
		
		sprintf(enxagueQnt,"%d enxagues",cicles[mode].enxagueQnt);
		font_draw_text(&calibri_24, enxagueQnt, 20, 155, 1);
		
		sprintf(centrifugacaoRPM,"%d RPM",cicles[mode].centrifugacaoRPM);
		font_draw_text(&calibri_24, centrifugacaoRPM, 20, 185, 1);
		
		sprintf(centrifugacaoTempo,"%d minutos",cicles[mode].centrifugacaoTempo);
		font_draw_text(&calibri_24, centrifugacaoTempo, 20, 210, 1);
	}
}

//DESENHA O BOT�O DA LISTA
void draw_buttons(button b[], int size){
	for (int i = 0; i < size; i++){
		draw_icon_button(b[i]);
	}	
}

//DESENHA A MENSAGEM DE FECHAR A PORTA
void draw_closeDoor(int shouldIDrawTheCloseTheDoorMessage){
	char aviso[32];
	
	if (shouldIDrawTheCloseTheDoorMessage) {
		draw_screen();
	
		sprintf(aviso,"%s","FECHAR PORTA!");
		font_draw_text(&calibri_24, aviso, 110, 100, 1);
	}
	else {
		ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));
		ili9488_draw_filled_rectangle(110, 100, ILI9488_LCD_WIDTH-1, 130);
	}
}

//DESENHA O TIMER
void draw_timer(){
	
	char tim[32];
		
	sprintf(tim,"%02d:%02d",minute, second);
	font_draw_text(&calibri_24, tim, ILI9488_LCD_WIDTH/2 - 30, ILI9488_LCD_HEIGHT - 210, 1);
}

//DESENHA O DISPLAY GERAL
void draw_display(button b[], int size, t_ciclo cicles[] ,uint8_t mode) {
	if(locked){
		draw_icon_button(b[0]);
		
		//COMEÇOU A LAVAGEM
		if (isWashing == 1){
			draw_timer(minute,second);
		}
		//TERMINOU A LAVAGEM
		else if (isWashing == 2){
			font_draw_text(&calibri_24, "yah boi terminou", ILI9488_LCD_WIDTH/2 - 30, ILI9488_LCD_HEIGHT - 210, 1);
		}
	}else{
		draw_buttons(b,size);
		draw_wash_mode(cicles,mode);
	}
	
}

//###############################################################################################################
//CONVERTS
uint32_t convert_axis_system_x(uint32_t touch_y) {
	// entrada: 4096 - 0 (sistema de coordenadas atual)
	// saida: 0 - 320
	return ILI9488_LCD_WIDTH - ILI9488_LCD_WIDTH*touch_y/4096;
}

uint32_t convert_axis_system_y(uint32_t touch_x) {
	// entrada: 0 - 4096 (sistema de coordenadas atual)
	// saida: 0 - 320
	return ILI9488_LCD_HEIGHT*touch_x/4096;
}

//###############################################################################################################
//HANDLERS
/**
*  Handle Interrupcao botao 1
*/
static void Button1_Handler(uint32_t id, uint32_t mask){
	flag_led = !flag_led;
	pin_toggle(LED_PIO, LED_PIN_MASK);
}

/**
* \brief Interrupt handler for the RTC. Refresh the display.
*/
void RTC_Handler(void)
{
	uint32_t ul_status = rtc_get_status(RTC);
	uint16_t hour;
	uint16_t m;
	uint16_t se;
	//INTERRUP��O POR SEGUNDO
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		rtc_clear_status(RTC, RTC_SCCR_SECCLR);
		//MUDA AS VARIAVEIS DE ACORDO COM O TEMPO SE TIVER LAVANDO
		if (isWashing==1){
			if (second == 0){
				minute--;
				second = 59;
				}else{
				second--;
			}
		}
		
	}
	
	//INTERRUP��O POR ALARME
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		//Terminou a lavagem mudando valor da variavel para o mesmo
		if (isWashing == 1){
			isWashing = 2 ;
		}

		rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
			
	}
	
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
	
}

int mxt_handler(struct mxt_device *device, uint16_t *x, uint16_t *y)
{
	/* USART tx buffer initialized to 0 */
	char tx_buf[STRING_LENGTH * MAX_ENTRIES] = {0};
	uint8_t i = 0; /* Iterator */
	uint8_t found =0;

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
		
		//printf("%d", conv_x);
		//printf("%d", conv_y);
		//printf("\nstatus do evento: %d",touch_event.status);
		if (touch_event.status == 192)
			found = 1;
		
		*x = conv_x;
		*y = conv_y;
		
		/* Format a new entry in the data string that will be sent over USART */
		//sprintf(buf, "Nr: %1d, X:%4d, Y:%4d, Status:0x%2x conv X:%3d Y:%3d\n\r",
				//touch_event.id, touch_event.x, touch_event.y,
				//touch_event.status, conv_x, conv_y);
		//update_screen(conv_x, conv_y);

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
	
	return(found);
}

//###############################################################################################################


//###############################################################################################################
//CALL BACKS
void callback_lock(button *b){
	printf("\nCALLBACK DO LOCK");
	b->state = b->state == 1 ? 2 : 1;
	locked = !locked;	
	isWashing = 0;
	draw_lockscreen();

}

void callback_wash_buttons(button *b, uint8_t index){
	b->state = b->state == 1 ? 2 : 1;
	wash_mode = index-2;
	cleanScreen = 1;
	isWashing = 0;
}

void callback_fast_wash(button *b){
	b->state = b->state == 1 ? 2 : 1;
	cleanScreen = 1;
}

void handler_wash_buttons(button buttons[], int size){
	for (int i = 2; i<size; i++){
		buttons[i].state = 1;
	}
}

void callback_start(button *b, uint8_t index){
	b->state = b->state == 1 ? 2 : 1;
	
	if (flag_led){
		//SETA A FLAG DE LAVANDO
		printf("flag led ativado: %d",flag_led);
		uint16_t hour;
		uint16_t m;
		uint16_t se;
		
		isWashing = 1;	
		washingLockScreen = 1;
		draw_closeDoor(0);
		
		rtc_get_time(RTC,&hour,&m,&se);
		rtc_set_time_alarm(RTC, 1, hour, 1, m + wash_times[index-2], 1, se);
		locked = 1;
		buttons2[0]->state = 1;
		draw_lockscreen();


	}else{
		draw_closeDoor(1);
	}
	
}

//###############################################################################################################
//FUN��ES

void BUT_init(void){
	/* config. pino botao em modo de entrada */
	pmc_enable_periph_clk(BUT_PIO_ID);
	pio_set_input(BUT_PIO, BUT_PIN_MASK, PIO_PULLUP | PIO_DEBOUNCE);

	/* config. interrupcao em borda de descida no botao do kit */
	/* indica funcao (but_Handler) a ser chamada quando houver uma interrup??o */
	pio_enable_interrupt(BUT_PIO, BUT_PIN_MASK);
	pio_handler_set(BUT_PIO, BUT_PIO_ID, BUT_PIN_MASK, PIO_IT_FALL_EDGE, Button1_Handler);

	/* habilita interrup?c?o do PIO que controla o botao */
	/* e configura sua prioridade                        */
	NVIC_EnableIRQ(BUT_PIO_ID);
	NVIC_SetPriority(BUT_PIO_ID, 1);	
}

void RTC_init(){
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(RTC, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(RTC, YEAR, MOUNTH, DAY, WEEK);
	rtc_set_time(RTC, HOUR, MINUTE, SECOND);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(RTC_IRQn);
	NVIC_ClearPendingIRQ(RTC_IRQn);
	NVIC_SetPriority(RTC_IRQn, 0);
	NVIC_EnableIRQ(RTC_IRQn);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(RTC,  RTC_IER_ALREN);
	rtc_enable_interrupt(RTC,  RTC_IER_SECEN);

}

void LED_init(int estado){
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_set_output(LED_PIO, LED_PIN_MASK, estado, 0, 0 );
};

//CALCULA O TEMPO DE LAVAGEM DO CICLO
int wash_time(t_ciclo cicles[], uint8_t mode){
	int t = cicles[mode].enxagueTempo * cicles[mode].enxagueQnt + cicles[mode].centrifugacaoTempo;
	
	if (cicles[mode].heavy){
		t = t*1.2;
	}
	
	return 1;
}

int isPressed(button b, uint16_t x, uint16_t y){
	tImage icon = b.state == 1 ? b.icon1 : b.icon2;
	
	if (x >= b.x0 && x <= (b.x0 + icon.width)){
		if (y >= b.y0 && y <= (b.y0 + icon.height)){
			return 1;
		}
	}
	return 0;
}

int touch_buttons(button b[],uint8_t size , uint16_t xTouch, uint16_t yTouch){
	for(int i = 0; i < size; i++){
		if(isPressed(b[i], xTouch, yTouch)){
			printf("Botao pressionado");
			if(locked && i !=0){
				return size+1;
			}
			return i;
		}
	}
	return size + 1;
}

//###############################################################################################################

int main(void){
	
	sysclk_init(); /* Initialize system clocks */
	WDT->WDT_MR = WDT_MR_WDDIS; // WatchDog
	board_init();  /* Initialize board */
	LED_init(0); // Inicializa LED ligado
	BUT_init(); // Inicializando Botao

	button b_lock =	 {.x0 = 0, .y0 = 0, .state = 1, .icon1 = lock_white, .icon2 = unlock_white, .callback = callback_lock};
	button b_centrifuga = {.x0 = ILI9488_LCD_WIDTH - 93, .y0 = ILI9488_LCD_HEIGHT - 195, .state = 1, .icon1 = centrifuge, .icon2 = centrifuge_click, .callback = callback_wash_buttons};
	button b_fast = {.x0 = 110, .y0 = ILI9488_LCD_HEIGHT - 195, .state = 1, .icon1 = fast, .icon2 = fast_click, .callback = callback_wash_buttons};
	button b_slow = {.x0 = 0, .y0 = ILI9488_LCD_HEIGHT - 195, .state = 1, .icon1 = heavy, .icon2 = heavy_click, .callback = callback_wash_buttons};
	button b_daily = {.x0 = 45, .y0 = ILI9488_LCD_HEIGHT - 98, .state = 1, .icon1 = daily, .icon2 = daily_click, .callback = callback_wash_buttons};
	button b_enxague = {.x0 = ILI9488_LCD_WIDTH - 138, .y0 = ILI9488_LCD_HEIGHT - 98, .state = 1, .icon1 = water, .icon2 = water_click, .callback = callback_wash_buttons};
	button b_start = {.x0 = 320 - 93, .y0 = 0, .state = 1, .icon1 = clean, .icon2 = clean_click, .callback = callback_start};
	
	const uint8_t size = 7;

	button buttons[] = {b_lock, b_start, b_fast, b_centrifuga, b_slow, b_enxague, b_daily};	
	
	for (int i = 0; i<size;i++)
	{
		buttons2[i]=&buttons[i];
	}
	
	struct mxt_device device; /* Device data container */

	t_ciclo cicles[] = {c_rapido, c_centrifuga,c_pesado, c_enxague,c_diario};
	
	uint8_t cicles_size = 5;

	/* Initialize the USART configuration struct */
	const usart_serial_options_t usart_serial_options = {
		.baudrate     = USART_SERIAL_EXAMPLE_BAUDRATE,
		.charlength   = USART_SERIAL_CHAR_LENGTH,
		.paritytype   = USART_SERIAL_PARITY,
		.stopbits     = USART_SERIAL_STOP_BIT
	};

	configure_lcd();
	draw_screen();
	
	/** Configura RTC */
	RTC_init();
	/* Initialize the mXT touch device */
	mxt_init(&device);
	
	/* Initialize stdio on USART */
	stdio_serial_init(USART_SERIAL_EXAMPLE, &usart_serial_options);

	for (int i = 0; i<cicles_size;i++)
	{
		wash_times[i]=wash_time(cicles,i);
	}
	
	isWashing = 0;
	flag_led = 0;

	while (true) {
		/* Check for any pending messages and run message handler if any
		 * message is found in the queue */
		uint16_t x,y;
		draw_display(buttons, size, cicles, wash_mode);
		if (mxt_is_message_pending(&device)) {
			uint8_t found = mxt_handler(&device, &x, &y);
			if(found){
				uint8_t index = touch_buttons(buttons, size, x, y);
				if (index != (size+1)){
					handler_wash_buttons(buttons,size);
					buttons[index].callback(&buttons[index], index, cicles, buttons);

				}
				
				if (isWashing==1){
					//CALCULA O TEMPO EM MINUTOS DO CICLO ESCOLHIDO
					minute = wash_time(cicles, wash_mode);	
				}
				
			}
		
		}
		
	}

	return 0;
}
