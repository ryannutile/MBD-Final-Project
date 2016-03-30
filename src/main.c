/**
 *@file main.c
 *
 *@brief
 *  - Test audio loopback
 * 
 * 1. Cofigure I2C controller to communicate with codec
 * 2. reads file via JTAG
 * 3. Playback the audio data
 * 4. transfer data using DMA ch.4
 *
 * Target:   TLL6537v1-1      
 * Compiler: VDSP++     Output format: VDSP++ "*.dxe"
 *
 * @author    Rohan Kangralkar, ECE, Northeastern University  (03/11/09)
 * @date 03/15/2009
 *
 * LastChange:
 * $Id: main.c 984 2016-03-15 14:29:21Z schirner $
 *
 *******************************************************************************/

#include "audioSample.h"
#include "audioPlayer.h"
#include "bufferPool_d.h"
#include "zedboard_freertos.h"

#include "adau1761.h"
#define VOLUME_MIN (0x2F)



#define numChunks 561
#define chunkSize 511

/******************************************************************************
 *                     GLOBALS
 *****************************************************************************/
/**
 * @var audioPlayer
 * @brief  global audio player object
 */
    
audioPlayer_t            audioPlayer;


int main(void)
{
    printf("[MAIN]: Starting Audio Player\r\n");

    audioPlayer_init(&audioPlayer);

    audioPlayer_start(&audioPlayer);


	// start the OS scheduler to kick off the tasks.
	vTaskStartScheduler();
	return(0);

}
