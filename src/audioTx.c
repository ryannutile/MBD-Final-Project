/**
 *@file audioTx.c
 *
 *@brief
 *  - receive audio samples from DMA
 *
 * Target:   TLL6527v1-0      
 * Compiler: VDSP++     Output format: VDSP++ "*.dxe"
 *
 * @author  Gunar Schirner
 *          Rohan Kangralkar
 * @date 03/15/2009
 *
 * LastChange:
 * $Id: audioTx.c 814 2013-03-12 03:59:36Z ovaskevi $
 *
 *******************************************************************************/
#include "audioTx.h"
#include "bufferPool_d.h"


/** Initialize audio tx
 *    - get pointer to buffer pool
 *    - register interrupt handler
 *    - initialize TX queue

 * Parameters:
 * @param pThis  pointer to own object
 * @param pBuffP  pointer to buffer pool to take and return chunks from
 * @param pIsrDisp   pointer to interrupt dispatcher to get ISR registered
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int audioTx_init(audioTx_t *pThis, bufferPool_d_t *pBuffP)
{
    // Parameter checking
    if ( NULL == pThis || NULL == pBuffP) {
        printf("[ATX]: Initialization failed.\n");
        return -1;
    }
    
    // store pointer to buffer pool for later access     
    pThis->pBuffP       = pBuffP;

    pThis->pPending     = NULL; // nothing pending
    pThis->running      = 0;    // DMA turned off by default
    
    // Initialization queue
    pThis->queue = xQueueCreate(AUDIOTX_QUEUE_DEPTH, sizeof(chunk_d_t*));

    // note ISR registration done in _start
    
    printf("[ARX]: TX Initialization complete.\n");
    
    return 1;
}



/** start audio tx
 *   - empty for now
 * Parameters:
 * @param pThis  pointer to own object
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int audioTx_start(audioTx_t *pThis)
{
     
	/* initialize interrupt handler */

	XScuGic *pGic; // pointer to GIC interrupt driver
	pGic = prvGetInterruptControllerInstance(); // retrieve pointer to initialized instance

	// connect own interrupt handler
	XScuGic_Connect(pGic, XPS_FPGA15_INT_ID, (Xil_ExceptionHandler) audioTx_isr, (void*) pThis);

	// enable IRQ interrupt at GIC
	XScuGic_Enable(pGic, XPS_FPGA15_INT_ID);

	// define priority and trigger type for AXI Stream FIFO IRQ
	XScuGic_SetPriorityTriggerType(pGic, XPS_FPGA15_INT_ID, 0xA0, 0x3);

	/* Enable IRQ in processor core  */
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

	// Interrupt Enable
	*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_INT_ENABLE) = 0x0 | (0x1 << 21);
    return 1;
}



/** audio tx isr  (to be called from dispatcher) 
 *   - get chunk from tx queue
 *   - if valid, release old pending chunk to buffer pool
 *   - configure DMA
 *   - if not valid, configure DMA to replay same chunk again
 * Parameters:
 * @param pThis  pointer to own object
 *
 * @return None 
 */
void audioTx_isr(void *pThisArg)
{
    // create local casted pThis to avoid casting on every single access
	audioTx_t  *pThis = (audioTx_t*) pThisArg;

    /* check int type and clear interrupt */
	if (*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_INT_STATUS) & (0x1 << 21))
	{
		*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_INT_STATUS) = 0x1 << 21;
	}
	else
	{
		printf("Error in AudioTx_isr\n");
	}

    /* if queue is EMPTY
     *  - set signal that ISR is not running
     *  - return */
	if (xQueueIsQueueEmptyFromISR(pThis->queue) )
	{
		pThis->running = 0;
		return;
	}
	else
	{
		/* receive pointer to chunk structure from Tx_queue,
		Note: when ISR running, audioTx_put should send the chunk to Tx_queue */
		chunk_d_t *pChunk;
		xQueueReceiveFromISR(pThis->queue, &pChunk, NULL);

		/* how many samples does the chunk contain ? */
		unsigned numSamples = pChunk->bytesUsed / 4;
		/* check if sufficient space in device FIFO*/

		/* copy samples in chuck into FIFO */
		int i;
		for (i = 0; i < numSamples; i++)
		{
			*(u32*)(FIFO_BASE_ADDR + FIFO_TX_DATA) = pChunk->u32_buff[i] << 16;
			*(u32*)(FIFO_BASE_ADDR + FIFO_TX_LENGTH) = 1;
		}

		/* chunk has been copied into the FIFO, release chunk now. Return chunk to buffer pool. */
		bufferPool_d_release_from_ISR(pThis->pBuffP, pChunk);
	}
}



/** audio tx put
 *   copies filled pChunk into the TX queue for transmission
 *   if queue is full, then chunk is dropped
 * Parameters:
 * @param pThis  pointer to own object
 *
 * @return Zero on success.
 * Negative value on failure.
 */
int audioTx_put(audioTx_t *pThis, chunk_d_t *pChunk)
{
    
    if ( NULL == pThis || NULL == pChunk ) {
        printf("[TX]: Put failed.\n");
        return -1;
    }
    if (!pThis->running)
    {
		/* how many samples does the chunk contain ? */
		unsigned numSamples = pChunk->bytesUsed / 4;

		/* check if sufficient space in device FIFO*/
		while (numSamples*2 > (*(u32*)(FIFO_BASE_ADDR + FIFO_TX_VAC)) );

		/* copy samples in chunk into FIFO */
		int i;
		for (i = 0; i < numSamples; i++)
		{
			*(u32*)(FIFO_BASE_ADDR + FIFO_TX_DATA) = pChunk->u32_buff[i] << 16;
			*(u32*)(FIFO_BASE_ADDR + FIFO_TX_LENGTH) = 1;
		}
		/* chunk has been copied into the FIFO, release chunk now. Return chunk to buffer pool. */
		bufferPool_d_release(pThis->pBuffP, pChunk);

		pThis->running = 1;
    }
    else
    {
    	xQueueSend( pThis->queue, &pChunk, NULL );
    }
    return 0;
}
