/**
 *@file audioPlayer.c
 *
 *@brief
 *  - core module for audio player
 *
 * Target:   Xilinx Zynq Zedboard
 * Compiler/IDE: GCC - Xilinx SDK 2015.4
 *
 * @author:  Gunar Schirner
 *           Rohan Kangralkar
 * @date	03/08/2010
 *
 * LastChange:
 * $Id: audioPlayer.c 1009 2016-04-03 20:00:02Z surya2891 $
 *
 *******************************************************************************/

#include "audioPlayer.h"
#include "zedboard_freertos.h"
#include "audioRxTx.h"


/* number of chunks to allocate */
#define CHUNK_NUM 30

/* size of each chunk in bytes */
#define CHUNK_SIZE 512

/**
 * @def VOLUME_CHANGE_STEP
 * @brief Magnitude of change in the volume when increasing or decreasing
 */
#define VOLUME_CHANGE_STEP (4)
/**
 * @def VOLUME_MAX
 * @brief MAX volume possible is +6db Refer to COCDEC datasheet.
 */
#define VOLUME_MAX (0x7F)
/**
 * @def VOLUME_MIN
 * @brief MIN volume possible is -73db Refer to COCDEC datasheet.
 */
#define VOLUME_MIN (0x2F)
/** initialize audio player 
 *@param pThis  pointer to the AudioPlayer global instance.
 *
 *@return 0 success, non-zero otherwise
 **/
int audioPlayer_init(audioPlayer_t *pThis) {
    int status = 0;
    printf("[AP]: Init start\r\n");
    
    pThis->volume 		= VOLUME_MIN; /*default volume */
    pThis->frequency 	= 48000; /* default frequency */
    
    /* Init I2C/I2S/CODEC and AXI Streaming FIFO */
	Adau1761_Init(&pThis->codec);

	/* Allocate buffer pool and Init Chunk/freelist*/
	status = bufferPool_d_init(&pThis->bp, CHUNK_NUM, CHUNK_SIZE);
    if ( PASS != status ) {
        return FAIL;
    }

    /* Initialize the Audio RX/TX module*/
    status = audioRxTx_init(&pThis->Audio, &pThis->bp) ;
    if ( PASS != status) {
        return FAIL;
    }

    printf("[AP]: Init complete\r\n");
    return PASS;
}


/** audioPlayer task creation.
 *@param pThis  pointer to the globally declared and initialized audioPlayer object
 *
 *@return 0 success, non-zero otherwise
 **/
int audioPlayer_start(audioPlayer_t *pThis)
{
    printf("[AP]: startup \r\n");
	/* Audio Player task creation */
	xTaskCreate( audioPlayer_task, ( signed char * ) "HW", configMINIMAL_STACK_SIZE, pThis, tskIDLE_PRIORITY + 1 , NULL );
    return PASS;
}



/** main loop of audio player does not terminate
 *@param pThis  pointer to the globally declared and initialized audioPlayer object
 *
 *@return 0 success, non-zero otherwise
 **/
void audioPlayer_task (void *pArg) {
    
	print_message("Hello world",2);
	    print_message("How cool is this",3);

	int status = FAIL;
	audioPlayer_t *pThis = (audioPlayer_t *)  pArg;
	chunk_d_t *pChunk = NULL;

    /* Start the audio module (FIFO Interrupt Enable)*/
    status = audioRxTx_start(&pThis->Audio);
	if (status != 1) {
        return;
    }

	/* Main loop */
	while(1)
	{
    		/** Get Audio Chunk */
			audioRxTx_get(&pThis->Audio, &pChunk);

			/* Transmit the data that was received from the RX Queue */
			audioRxTx_put(&pThis->Audio, pChunk);
        }
	}
