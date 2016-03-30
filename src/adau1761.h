#ifndef CODEC_H_
#define CODEC_H_

#include "xparameters.h"
#include "zedboard_freertos.h"

//ADI I2S IP Base Address.
#define I2S_BASE_ADDR 0x43C10000

//ADI I2S IP register definitions
#define AXI_I2S_REG_RESET	0x00
#define AXI_I2S_REG_CTRL	0x04
#define AXI_I2S_REG_CLK_CTRL	0x08
#define AXI_I2S_REG_STATUS	0x10

#define AXI_I2S_REG_RX_FIFO	0x28
#define AXI_I2S_REG_TX_FIFO	0x2C

#define AXI_I2S_RESET_GLOBAL	(1<<0)
#define AXI_I2S_RESET_TX_FIFO	(1<<1)
#define AXI_I2S_RESET_RX_FIFO	(1<<2)

#define AXI_I2S_CTRL_TX_EN	(1<<0)
#define AXI_I2S_CTRL_RX_EN	(1<<1)

//sample rate and clocks
#define AXI_I2S_BITS_PER_FRAME 64
#define AXI_I2S_REF_CLK 12288000
#define AXI_I2S_RATE 20000

//register select helper
#define AXI_I2S_REGISTER(x) (I2S_BASE_ADDR + x)

//Audio channels
#define AUDIO_SAMPLES_STEREO 1
#define AUDIO_SAMPLES_MONO   0

typedef struct {
	XLlFifo ToI2S; // structure for FIFO to I2S
	XIicPs Iic;    // driver for I2C
} tAdau1761;


#define I2C_CLOCK   (400000)


/* ADAU internal registers */
enum audio_regs {
	R0_CLOCK_CONTROL								= 0x00,
	R1_PLL_CONTROL 									= 0x02,
	R2_DIGITAL_MIC_JACK_DETECTION_CONTROL 			= 0x08,
	R3_RECORD_POWER_MANAGEMENT						= 0x09,
	R4_RECORD_MIXER_LEFT_CONTROL_0 					= 0x0A,
	R5_RECORD_MIXER_LEFT_CONTROL_1 					= 0x0B,
	R6_RECORD_MIXER_RIGHT_CONTROL_0 				= 0x0C,
	R7_RECORD_MIXER_RIGHT_CONTROL_1 				= 0x0D,
	R8_LEFT_DIFFERENTIAL_INPUT_VOLUME_CONTROL 		= 0x0E,
	R9_RIGHT_DIFFERENTIAL_INPUT_VOLUME_CONTROL 		= 0x0F,
	R10_RECORD_MICROPHONE_BIAS_CONTROL 				= 0x10,
	R11_ALC_CONTROL_0								= 0x11,
	R12_ALC_CONTROL_1								= 0x12,
	R13_ALC_CONTROL_2								= 0x13,
	R14_ALC_CONTROL_3								= 0x14,
	R15_SERIAL_PORT_CONTROL_0 						= 0x15,
	R16_SERIAL_PORT_CONTROL_1 						= 0x16,
	R17_CONVERTER_CONTROL_0 						= 0x17,
	R18_CONVERTER_CONTROL_1 						= 0x18,
	R19_ADC_CONTROL									= 0x19,
	R20_LEFT_INPUT_DIGITAL_VOLUME 					= 0x1A,
	R21_RIGHT_INPUT_DIGITAL_VOLUME 					= 0x1B,
	R22_PLAYBACK_MIXER_LEFT_CONTROL_0 				= 0x1C,
	R23_PLAYBACK_MIXER_LEFT_CONTROL_1 				= 0x1D,
	R24_PLAYBACK_MIXER_RIGHT_CONTROL_0 				= 0x1E,
	R25_PLAYBACK_MIXER_RIGHT_CONTROL_1 				= 0x1F,
	R26_PLAYBACK_LR_MIXER_LEFT_LINE_OUTPUT_CONTROL 	= 0x20,
	R27_PLAYBACK_LR_MIXER_RIGHT_LINE_OUTPUT_CONTROL = 0x21,
	R28_PLAYBACK_LR_MIXER_MONO_OUTPUT_CONTROL 		= 0x22,
	R29_PLAYBACK_HEADPHONE_LEFT_VOLUME_CONTROL 		= 0x23,
	R30_PLAYBACK_HEADPHONE_RIGHT_VOLUME_CONTROL 	= 0x24,
	R31_PLAYBACK_LINE_OUTPUT_LEFT_VOLUME_CONTROL 	= 0x25,
	R32_PLAYBACK_LINE_OUTPUT_RIGHT_VOLUME_CONTROL 	= 0x26,
	R33_PLAYBACK_MONO_OUTPUT_CONTROL 				= 0x27,
	R34_PLAYBACK_POP_CLICK_SUPPRESSION 				= 0x28,
	R35_PLAYBACK_POWER_MANAGEMENT 					= 0x29,
	R36_DAC_CONTROL_0 								= 0x2A,
	R37_DAC_CONTROL_1 								= 0x2B,
	R38_DAC_CONTROL_2 								= 0x2C,
	R39_SERIAL_PORT_PAD_CONTROL 					= 0x2D,
	R40_CONTROL_PORT_PAD_CONTROL_0 					= 0x2F,
	R41_CONTROL_PORT_PAD_CONTROL_1 					= 0x30,
	R42_JACK_DETECT_PIN_CONTROL 					= 0x31,
	R67_DEJITTER_CONTROL 							= 0x36,
	R58_SERIAL_INPUT_ROUTE_CONTROL					= 0xF2,
	R59_SERIAL_OUTPUT_ROUTE_CONTROL					= 0xF3,
	R61_DSP_ENABLE									= 0xF5,
	R62_DSP_RUN										= 0xF6,
	R63_DSP_SLEW_MODES								= 0xF7,
	R64_SERIAL_PORT_SAMPLING_RATE 					= 0xF8,
	R65_CLOCK_ENABLE_0 								= 0xF9,
	R66_CLOCK_ENABLE_1 								= 0xFA
};



/* Define slave address for the ADAU1761 audio controller 8 */
#define IIC_SLAVE_ADDR			0x76

/* FIFO MMR Address resolutions */
#define FIFO_BASE_ADDR 0x43C00000

#define FIFO_TX_VAC 0x0000000c  /**< Transmit Vacancy */
#define FIFO_TX_DATA 0x00000010  /**< Transmit Data */
#define FIFO_TX_LENGTH  0x00000014  /**< Transmit Length */
#define FIFO_TX_RESET 0x00000008  /**< Transmit Reset */
#define FIFO_TX_DES  0x0000002C  /**< Transmit Destination  */

#define FIFO_RX_VAC 0x0000001c  /**< Receive Occupancy */
#define FIFO_RX_DATA 0x00000020  /**< Receive Data */
#define FIFO_RX_RESET 0x00000018  /**< Receive Reset */
#define FIFO_RX_LENGTH  0x00000024  /**< Receive Length */
#define FIFO_RX_DES  0x00000030  /**< Receive Destination  */

#define FIFO_TX_RESET_VALUE 0x000000a5 /**< Transmit reset value */
#define FIFO_RX_RESET_VALUE 0x000000a5 /**< Receive reset value */

#define FIFO_LLR_OFFSET  0x00000028  /**< Local Link Reset */
#define FIFO_LLR_RESET_VALUE         0x000000a5 /**< Local Link reset value */

#define FIFO_INT_ENABLE  0x00000004  /**< Interrupt Enable Register */
#define FIFO_INT_STATUS  0x00000000  /**< Interrupt Enable Register */

/** bits within int status */
#define FIFO_INT_RFPE (0x01 << 19) /**< Receive FIFO Programmable Empty  */
#define FIFO_INT_RFPF (0x01 << 20) /**< Receive FIFO Programmable Full   */
#define FIFO_INT_TFPE (0x01 << 21) /**< Transmit FIFO Programmable Empty */
#define FIFO_INT_TFPF (0x01 << 22) /**< Transmit FIFO Programmable Full  */

/* Prototype Functions */
unsigned char adau1761_init(tAdau1761 *pThis);

void ReadSamples(unsigned int * sampleBuffer, unsigned int sampleCount);


#endif
