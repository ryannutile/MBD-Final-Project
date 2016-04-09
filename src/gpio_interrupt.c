/**
 *@file gpio_interrupt.c
 *
 *@brief
 *  - Introduction lab for the TLL6527 platform
 *  - Gpio input example using interrupts.The leds corresponding to the push buttons are turned on/off
 * 
 *
 * Compiler: VDSP++     Output format: VDSP++ "*.dxe"
 *
 *@author    Rohan Kangralkar, ECE, Northeastern University
 *@date      01/16/2010
 *
 * LastChange:
 * $Id: gpio_interrupt.c 928 2016-01-30 19:38:14Z velu.s $
 *
 *******************************************************************************/


#include "zedboard_freertos.h"
#include "gpio_interrupt.h"
#include "audioPlayer.h"
#include <stdbool.h>
#include <stdio.h>

/* user GPIO Interrupt handler */
static void gpio_intrHandler(void *pRef);

/* setup interrupt connection */
static int gpio_setupInts(void);

/* Define QueueHandle */
QueueHandle_t gCountUpdateQ;

/* Define AudioPlayer */
audioPlayer_t *gAudioPlayer;

/* GPIO Task */
static void gpio_task( void *pvParameters )
{
	// Pointer to the volume update flag
	int *volChangePtr;
	// Holder for the value of the volume update flag
	int volChange;

	/* setup interrupts
	 * Note: needs to be called in task context as GIC is initialized
	 * upon starting of multi tasking.
	 */
	gpio_setupInts();
	
	/* Create a queue capable of containing 10 unsigned long values. */
	gCountUpdateQ = xQueueCreate(10, sizeof(int));
	if (gCountUpdateQ == 0)
	{
		printf("ERROR CREATING QUEUE.");
	}
	else
	{
		printf("Queue Created.\n");
	}
	
	for(;;){
		// Receive the volume update flag from Queue
		if( xQueueReceive(gCountUpdateQ, &( volChangePtr), ( TickType_t )1000) )
		{
			volChange = (int)volChangePtr;
			// If flag set high, increase volume
			if(volChange){
				audioPlayer_volumeIncrease(gAudioPlayer);
			}
			// Else, decrease volume
			else{
				audioPlayer_volumeDecrease(gAudioPlayer);
			}
		}
	}


	// suspend this task. All activities are in interrupts.
	vTaskSuspend(NULL);
}


/* Connect interrupt handler */
static int gpio_setupInts(void) {

	// pointer to driver structure
	XScuGic *pGIC;
	// get pointer to GIC (already initialized at OS startup)
	pGIC = prvGetInterruptControllerInstance();
	// connect own interrupt handler to GIC handler
	XScuGic_Connect(pGIC, GPIO_INTERRUPT_ID,
	(Xil_ExceptionHandler) gpio_intrHandler,(void *) NULL);
	// Enable interrupt at GIC
	XScuGic_Enable(pGIC, GPIO_INTERRUPT_ID);
	/* Enable IRQ at core (should be enabled anyway)*/
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

	return XST_SUCCESS;
}

/**
 * Function that is called when an interrupt happens
 *
 * Parameters:
 *
 * @return void
 */
static void gpio_intrHandler(void *pRef)
{
	// Initialize flag for signaling a volume update is required
	static int flag = 0;
	
	// Initialize ints for holding the time of the last button pushes
	static uint32_t lastTimeUp = 0;
	static uint32_t lastTimeDown = 0;

	//Get Current Time
	uint32_t currentTime = xTaskGetTickCountFromISR();

	/* Read interrupt status of the GPIO pins */
	u8 button_Ints = (* (u32 *) GPIO_INT_STAT_2) >> 16;
	
	// Clear interrupts
	(* (u32 *) GPIO_INT_STAT_2) &= 0xffffffff;

    // Save GPIO input data for comparison
	u32 temp = *(u32 *)GPIO_DATA_RO_2;
    
    // If down button pressed and enough time has passed
	if ((currentTime > lastTimeDown + 20) && (button_Ints & 0b100) && !(temp >> 16 & 0x40000 ) )
	{
		// Update Stored Time Value
		lastTimeDown = currentTime;
		
		// Update flag for decrease
		flag = 0;
		
		/* Send the counter value to Queue */
		xQueueSendFromISR( gCountUpdateQ,( void * ) &flag, NULL);
	}

	// If up button pressed and enough time has passed
	if((currentTime > lastTimeUp + 20) && (button_Ints & 0b10000) && !(temp >> 16 & 0x100000 ) )
	{
		// Update Stored Time Value
		lastTimeUp = currentTime;
		
		// Update flag for increase
		flag = 1;

		/* Send the counter value to Queue */
		xQueueSendFromISR( gCountUpdateQ,( void * ) &flag, NULL);
	}
}



/**
 * Initialize the GPIO
 *
 * Parameters:
 *
 * @return void
 */
void gpio_init(audioPlayer_t *audioPlayer) {

	/* OutEnable for LEDs  */
	//* (volatile u32 *)GPIO_DIRM_2 |= 0b11111111;
	//* (volatile u32 *)GPIO_OEN_2 |= 0b11111111;

	/* disable interrupts before configuring new ints */
	* (volatile u32 *)GPIO_INT_DIS_2 = 0xffffffff;

	* (volatile u32 *)GPIO_INT_TYPE_2 = 0x140000; // edge-sensitive - Only Up/Down Buttons
	* (volatile u32 *)GPIO_INT_POLARITY_2 = 0x140000; // rising-edge
	* (volatile u32 *)GPIO_INT_ANY_2 = 0; // only rising-edge

	/* enable input bits */
	* (volatile u32 *)GPIO_INT_EN_2 = 0x140000;

	/* disable LEDs on startup */
	//*(u8 *)GPIO_DATA_2 &= 0b00000000;

	gAudioPlayer = audioPlayer;
}

/**
 *
 *
 * Parameters:
 *
 * @return void
 */
void gpio_start(void)
{
	xTaskCreate( gpio_task, ( signed char * ) "HW", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1 , NULL );
}



