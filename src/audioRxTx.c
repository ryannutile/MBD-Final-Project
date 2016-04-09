/**
 *@file audioTx.c
 *
 *@brief
 *  - Transmit/Recieve samples from FIFO
 *
 * Target:   Xilinx Zynq Zedboard.
 * Compiler: Xilinx SDK 2015.4     Output format: elf
 *
 * @author  Gunar Schirner
 *          Rohan Kangralkar
 * @date 03/15/2009
 *
 * LastChange:
 * $Id: audioRxTx.c 1009 2016-04-03 20:00:02Z surya2891 $
 *
 *******************************************************************************/
#include "audioRxTx.h"
#include "bufferPool_d.h"


/* Init RX/TX Queue */
int audioRxTx_init(audioRxTx_t *pThis, bufferPool_d_t *pBuffP)
{
    if ( NULL == pThis || NULL == pBuffP) {
        printf("[A_RX/TX]: Failed Init\r\n");
        return -1;
    }

    pThis->pPending     = NULL;
    pThis->pBuffP       = pBuffP;

    pThis->running      = 0;    // Disable ISR mode - first chunk to be sent to FIFO by polling.
    //Create Tx_Queue
    pThis->tx_queue = xQueueCreate(AUDIOTX_QUEUE_DEPTH, sizeof(chunk_d_t*));
    //Create RX_Queue.
    pThis->rx_queue = xQueueCreate(AUDIORX_QUEUE_DEPTH, sizeof(chunk_d_t*));
    printf("[A_RX/TX]: Init complete\r\r\n");

    return PASS;
}


/* Init FIFO interrupt */
int audioRxTx_start(audioRxTx_t *pThis)
{
     
	/* initialize interrupt handler */
	XScuGic *pGic; // pointer to GIC interrupt driver
	pGic = prvGetInterruptControllerInstance(); // retrieve pointer to initialized instance

	// connect FIFO interrupt handler
	XScuGic_Connect(pGic, XPS_FPGA15_INT_ID, (Xil_ExceptionHandler) audioRxTx_isr, (void*) pThis);

	// enable IRQ interrupt at GIC
	XScuGic_Enable(pGic, XPS_FPGA15_INT_ID);

	// define priority and trigger type for AXI Stream FIFO IRQ
	XScuGic_SetPriorityTriggerType(pGic, XPS_FPGA15_INT_ID, 0xA0, 0x3);

	/* Enable IRQ in processor core  */
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

    return 1;
}



/** audioRxTx get
 *   copies a filled chunk into pChunk
 *   blocking call, blocks if queue is empty
 *     - get from queue
 *     - copy in to pChunk
 *     - release chunk to buffer pool
 * Parameters:
 * @param pThis  pointer to own object
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int audioRxTx_get(audioRxTx_t *pThis, chunk_d_t **pChunk)
{
	/* Wait until there's Chunk in the RX Queue */
	while ((xQueueReceive(pThis->rx_queue, pChunk, 10))!=pdTRUE);

    return 0;
}

/** Audio ISR (FIFO)
 *
 *
 * Parameters:
 * @param pThisArg  Initialized Audio (TX/RX) object
 *
 * @return None 
 */
void audioRxTx_isr(void *pThisArg) {
	audioRxTx_t *pThis = (audioRxTx_t*) pThisArg;
	chunk_d_t *pChunk = NULL;
	unsigned int samplesInChunk;

	/* Read FIFO Interrupt Status */
	unsigned int intStatus =
			*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_INT_STATUS);

														/* Tx FIFO programmable empty hit */
	if (intStatus & FIFO_INT_TFPE) {
		/* clear TFPE interrupt */
		*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_INT_STATUS) = FIFO_INT_TFPE;

		/* Check if Tx queue is EMPTY
		 *  - set signal that ISR is not running
		 *  - return */
		if (xQueueIsQueueEmptyFromISR(pThis->tx_queue) != pdFALSE) {
			pThis->running = 0; /* indicate that ISR is no longer running */
			return;
		}

		// Tx Queue is not empty - Receive pointer to the next chunk.
		xQueueReceiveFromISR(pThis->tx_queue, &pChunk, NULL);

		/* how many samples does the chunk contain ? */
		samplesInChunk = pChunk->bytesUsed/sizeof(unsigned int);

		// This check might not really be needed - as we have recieved a TPFE trigger from FIFO
		if (samplesInChunk  > (*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_TX_VAC))) {
			printf("audioTx_isr: insufficient space in FIFO\n");
			// should anyway never happen

			// if TX FIFO Does not have the space promised, just drop the chunk
			bufferPool_d_release_from_ISR(pThis->pBuffP, pChunk);
			return;
		}
		/* Transmit the chunk data to the TX FIFO */
		u32 samplNr = 0;
		for (samplNr = 0; samplNr < samplesInChunk; samplNr++) {

			*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_TX_DATA) =
					((unsigned int)pChunk->u16_buff[samplNr]) << 16;
			*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_TX_LENGTH) = 0x1;
		}

		/* Return chunk to buffer pool free list */
		bufferPool_d_release_from_ISR(pThis->pBuffP, pChunk);
	}

														/* RX FIFO programmable Full hit */
	if (intStatus & FIFO_INT_RFPF) {
		/* Clear RFPF interrupt */
		*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_INT_STATUS) = FIFO_INT_RFPF;
		/* is Queue is full ? */
		if ((xQueueIsQueueFullFromISR(pThis->rx_queue)) != pdFALSE) {
			printf(
					"The RX queue is full, and no more incoming data could be captured\n");
			*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_INT_ENABLE) = 0x0;
			return;
		}

		while (bufferPool_d_acquire_ISR(pThis->pBuffP, &pChunk) != 1) {
			printf("RX ISR: buffer pool empty\n\n\n\n\n");
			*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_INT_ENABLE) = 0x0;
			return;
		}

		// How many samples in FIFO?
		samplesInChunk =  *(volatile u32 *) (FIFO_BASE_ADDR + FIFO_RX_OCC);

		// more samples in FIFO than fitting into chunk? Limit
		if(samplesInChunk > pChunk->bytesMax/4){
			samplesInChunk = pChunk->bytesMax/4;
		}

		/* Read the Audio RX samples.*/
		u32 samplNr = 0;
		for (samplNr = 0; samplNr < samplesInChunk; samplNr++) {

			// read from FIFO and right shift by 16 (8 LSBs are 0 anyway, plus we want to use 16 bit only).
			// symmetric to tx side.
			pChunk->u16_buff[samplNr] = (unsigned short) ((*(volatile u32 *) (FIFO_BASE_ADDR
			                                        + FIFO_RX_DATA)) >> 16);
		}

		/* indicate max fill level */
		pChunk->bytesUsed= samplesInChunk * 4;

		xQueueSendFromISR( pThis->rx_queue, &pChunk, NULL);
		return;

	}

														/* did neither RX nor TX interrupt hit? */
	if( !(intStatus & (FIFO_INT_RFPF | FIFO_INT_TFPE))) {
		/* An interrupt other than the one we enabled has triggered. */
		printf("audioTx_isr: unknown int reason %x\n", intStatus);
		/* clear all ints just to get back to normal */
		*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_INT_STATUS) = intStatus;
		return;

	}
}



/** audio tx put
 *   Copies filled pChunk into the TX queue for transmission
 *    if queue is full, then chunk is dropped 
 * Parameters:
 * @param pThis  Instance of the Audio (RX/TX) object.
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int audioRxTx_put(audioRxTx_t *pThis, chunk_d_t *pChunk)
{
    unsigned int sampleNr = 0;
    if ( NULL == pThis || NULL == pChunk ) {
        printf("[TX]: Chunk/Audio objects not initialized \r\n");
        return -1;
    }
    
    
    /* ISR/polled execution ? */
    if ( 0 == pThis->running ) {
    	unsigned int samplesInChunk = pChunk->bytesUsed/sizeof(unsigned int);

    	/* Do polled transfer - by checking TX VACANCY in the FIFO */
    	while( samplesInChunk > (*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_TX_VAC)));

    	for(sampleNr=0;sampleNr < samplesInChunk;sampleNr++) {
        	*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_TX_DATA) = ((unsigned int)pChunk->u16_buff[sampleNr]) << 16;
        	*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_TX_LENGTH) = 0x1;
        }

        /* chunk data has been copied into the TX FIFO, release chunk to the free list*/
        bufferPool_d_release((pThis->pBuffP), pChunk);

        /* Enable TX ISR mode */
        pThis->running = 1;
        return 0;
    }

    else{
        /* ISR is already running, add chunk to TX queue and it will be processed in ISR*/
    	while(xQueueSend( pThis->tx_queue,  &pChunk, ( TickType_t ) 100) != pdPASS) {
    		printf("TX Queue is full, loop until TX FIFO drains.\n");
    	}
    }
    return 0;

}
