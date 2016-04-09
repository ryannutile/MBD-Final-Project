#include "zedboard_freertos.h"
#include "gpio_pwm.h"

/* user TTC Interrupt handler */
static void ttc_intrHandler(void *pRef);

/* setup interrupt connection */
static int ttc_setupInt(void);

/* Define QueueHandle */
QueueHandle_t tCountUpdateQ;

static void ttc_task( void *pvParameters )
{
	// Setup the interrupt
	ttc_setupInt();
	
	// Pointer to the volume update flag
	int *oledUpdatePtr;
	
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
		if( xQueueReceive(tCountUpdateQ, &(oled_updatePtr), ( TickType_t )1000) )
		{
			uint32_t * levelData = audioPlayer_getLevels();
			oled_updateDisplay(levelData);
		}
	}
	
	/* suspend this task, all activities in interrupts */
	vTaskSuspend(NULL);
}

/* Connect interrupt handler */
static int ttc_setupInt(void) {

	// pointer to driver structure
	XScuGic *pGIC;
	// get pointer to GIC (already initialized at OS startup)
	pGIC = prvGetInterruptControllerInstance();
	// connect own interrupt handler to GIC handler
	XScuGic_Connect(pGIC, GPIO_INTERRUPT_ID,
	(Xil_ExceptionHandler) ttc_intrHandler,(void *) NULL);											// Need to find timer interrupt ID (XParameters.h)
	// Enable interrupt at GIC
	XScuGic_Enable(pGIC, GPIO_INTERRUPT_ID);
	/* Enable IRQ at core (should be enabled anyway)*/
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

	return XST_SUCCESS;
}

static void ttc_intrHandler(void *pRef)
{
	// Clear interrupt
	(* (uint8_t *) TTC_T0_INT_STATUS) &= 0b11111110; 												// does this addressing make sense? it's an 8-bit wide register
	
	int flag = 1;
	xQueueSendFromISR( tCountUpdateQ,( void * ) &flag, NULL);
}

void ttc_init(void)
{
	uint32_t ClockCntrl, CounterCntrl ,Period;

																									/* Need to determine prescaler value */
	
	//Set Clock Control 7bits, Enable Prescaler(Bit0) and give the Prescaler Value(Bit4:0)
	ClockCntrl =  0b0001011;
	*((volatile uint32_t*)(SET_CLK_CNTRL_VAL)) = ClockCntrl;

	/* Set Counter Control 7bits, Wave_pol(Bit6), Wave_en(Bit5), RST(Bit4), Match(Bit3), DEC(Bit2), INT(Bit1), DIS(Bit0) */
	CounterCntrl = 0b1110010;
	*((volatile uint32_t*)(SET_CNT_CNTRL_VAL)) = CounterCntrl;
	
	Period = 165812;
	*((volatile uint32_t*)(SET_INTERVAL_VAL)) = Period;												/* Need to determine interval value (update freq) */
	
	uint8_t intEn = (* (uint32_t *) TTC_T0_INT_EN);
	
	// Enable interval interrupt for TTC0
	(* (uint32_t *) TTC_T0_INT_EN) |= 0b00000001;
	
}

void ttc_start(void)
{
	//Create a task. This task can be removed if there isn't a need to run any tasks.
	xTaskCreate( ttc_task, ( signed char * ) "HW", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1 , NULL );
}
