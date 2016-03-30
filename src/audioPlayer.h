/**
 *@file audioPlayer.h
 *
 *@brief
 *  - core module for audio player
 *
 * Target:   TLL6527v1-0      
 * Compiler: VDSP++     Output format: VDSP++ "*.dxe"
 *
 * @author  Gunar Schirner
 *          Rohan Kangralkar
 * @date 03/15/2009
 *
 * LastChange:
 * $Id: audioPlayer.h 812 2013-03-12 02:28:57Z ovaskevi $
 *
 *******************************************************************************/
#ifndef _AUDIO_PLAYER_H_
#define _AUDIO_PLAYER_H_

#include "bufferPool_d.h"
#include "audioRx.h"
#include "audioTx.h"
#include "adau1761.h"

/** audioPlayer object
 */
typedef struct {
  audioRx_t      	rx;  /* receive object */
  audioTx_t      	tx;  /* transmit object */
  bufferPool_d_t   	bp;  /* buffer pool */
  int 					volume;	/* Volume of the audio player */
  unsigned int 	frequency;	/* Frequency of the audio player */
  chunk_d_t            *chunk;  /* Chunk for copy */
  tAdau1761 codec;  /* audio codec */
} audioPlayer_t;

/** initialize audio player 
 *@param pThis  pointer to own object 
 *
 *@return 0 success, non-zero otherwise
 **/
int audioPlayer_init(audioPlayer_t *pThis);

/** startup phase after initialization 
 *@param pThis  pointer to own object 
 *
 *@return 0 success, non-zero otherwise
 **/
int audioPlayer_start(audioPlayer_t *pThis);

/** main loop of audio player does not terminate
 *@param pThis  pointer to own object 
 *
 *@return 0 success, non-zero otherwise
 **/
void audioPlayer_task(void *pArg);

#endif



