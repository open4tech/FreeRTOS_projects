/*
    Source code for the example from Open4Tech.com article:
    https://open4tech.com/freertos-led-blinking-and-button-polling/

    The program is developed/tested using:
     MCUXpresso IDE v11.1.1
     SDK Version 2.6.1 for LPCXpresso54102
     FreeRTOS Kernel V10.2.0
 */

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "event_groups.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"

#include "pin_mux.h"
#include <stdbool.h>
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define led_toggle_task_PRIO (configMAX_PRIORITIES - 1)
#define btn_press_task_PRIO (configMAX_PRIORITIES - 1)

#define BTN_PRESSED (0)
#define BTN_NOT_PRESSED (1)

#define READ_BTN_PIN_LEVEL() GPIO_PinRead(GPIO, BOARD_SW1_GPIO_PORT, BOARD_SW1_GPIO_PIN)
#define LED_OFF()            GPIO_PinWrite(GPIO, BOARD_LED_BLUE_GPIO_PORT, BOARD_LED_BLUE_GPIO_PIN, 1)
#define LED_TOGGLE()         GPIO_PortToggle(GPIO, BOARD_LED_BLUE_GPIO_PORT, 1u << BOARD_LED_BLUE_GPIO_PIN);
//Button pressed event bit(flag)
#define BTN_PRESSED_Pos                         0U
#define BTN_PRESSED_Msk                         (1UL << BTN_PRESSED_Pos)
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void led_toggle_task( void * pvParameters );
static void btn_read_task(void *pvParameters);

/* Declare a variable to hold the created event group. */
static        EventGroupHandle_t xFlagsEventGroup;

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Application entry point.
 */
int main(void)
{
	/* Init board hardware. */
	/* attach 12 MHz clock to USART0 (debug console) */
	CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);
	GPIO_PortInit(GPIO, BOARD_SW1_GPIO_PORT);
	GPIO_PortInit(GPIO, BOARD_LED_BLUE_GPIO_PORT);
	BOARD_InitPins();
	BOARD_BootClockPLL150M(); /* Rev B device can only support max core frequency to 96Mhz.
                                Rev C device can support 150Mhz,use BOARD_BootClockPLL150M() to boot core to 150Mhz.
                                DEVICE_ID1 register in SYSCON shows the device version.
                                More details please refer to user manual and errata. */
	BOARD_InitDebugConsole();

	/* create the event group. */
	xFlagsEventGroup = xEventGroupCreate();

	xTaskCreate(led_toggle_task, "LED_toggle_task", configMINIMAL_STACK_SIZE + 10, NULL, led_toggle_task_PRIO, NULL);
	xTaskCreate(btn_read_task, "button_read_task", configMINIMAL_STACK_SIZE + 10, NULL, btn_press_task_PRIO, NULL);

	vTaskStartScheduler();
	for (;;)
		;
}

/*!
 * @brief Task responsible for toggling the LED and switching it OFF.
 *
 */
static void led_toggle_task( void * pvParameters )
{
	TickType_t last_wake_time;
	//Set toggle interval to 200ms
	const TickType_t toggle_interval = 200/portTICK_PERIOD_MS;
	EventBits_t event_bits;
	uint8_t toggle_en = 0;

	// Initialize the last_wake_time variable with the current time
	last_wake_time = xTaskGetTickCount();

	for( ;; )
	{
		// Wait for the next cycle.
		vTaskDelayUntil( &last_wake_time, toggle_interval );
		//Get the button pressed event bit and clear it.
		event_bits = xEventGroupClearBits( xFlagsEventGroup,BTN_PRESSED_Msk);
		if (event_bits & BTN_PRESSED_Msk) {
			toggle_en = ~toggle_en;
			/* if toggling was enabled then we turn off the LED
			   if toggling was disabled, then the led was turned off anyway */
			LED_OFF();
		}
		if (toggle_en) {
			LED_TOGGLE();
		}
	}
}


/*!
 * @brief Task responsible for reading a button state (0 or 1).
 * 		  The task sets an event flag in case the button is pressed.
 */
static void btn_read_task(void *pvParameters)
{
	TickType_t last_wake_time;
	uint8_t curr_state,prev_state = BTN_NOT_PRESSED; //State for the button
	//Set toggle interval to 20ms
	const TickType_t sample_interval = 20/portTICK_PERIOD_MS;

	// Initialize the last_wake_time variable with the current time
	last_wake_time = xTaskGetTickCount();

	for( ;; )
	{
		// Wait for the next cycle.
		vTaskDelayUntil( &last_wake_time, sample_interval );
		// Get the level on button pin
		curr_state = READ_BTN_PIN_LEVEL();
		if ((curr_state == BTN_PRESSED) && (prev_state == BTN_NOT_PRESSED)) {
			xEventGroupSetBits(xFlagsEventGroup, BTN_PRESSED_Msk);
		}
		prev_state = curr_state;
	}
}
