#include "audioRxTx.h"
#include "adau1761.h"
#include "zedboard_freertos.h"
#include "audioPlayer.h"

// 0 - Line In; 1 - MIC; 2 - Line In and MIC
/* Reset the Zedboard to switch between these options - Expect some sinusoidal noise when using MIC at the beginning */
#define LINE_IN 0
#define MIC 1
#define LINE_MIC 2


/* Initializes I2C/I2S/CODEC and AXI Streaming FIFO */
unsigned char Adau1761_Init(tAdau1761 *pThis)
{
	/* init PS I2C driver */
	Adau1761_I2CMaster_Init(pThis, 0, I2C_CLOCK);

	/* init ADAU1761 Codec to default settings */
	Adau1761_Codec_Init(pThis);

	/* init PL I2S */
	Adau1761_IIS_Init(pThis);

	/* init PL AXI streaming FIFO */
	Adau1761_FIFO_Init(pThis);

	/* Audio Input Path Source Select - MIC/Line IN, L/R Input Volume  */
	Adau1761_InSelect(pThis, LINE_MIC, 0xFF, 0xFF);

	return PASS;
}

/* Refer Page 29 - Record Signal Path */
void Adau1761_InSelect(tAdau1761 *pThis, unsigned short In_Sel, unsigned short L_In_Vol, unsigned short R_In_Vol){

	switch(In_Sel){

	case MIC:
			/* MIC configurations - Refer Page 30 */
			Adau1761_RegWrite(pThis, R8_LEFT_DIFFERENTIAL_INPUT_VOLUME_CONTROL, L_In_Vol); //Set Input Volume - Check datasheet for more options.
			Adau1761_RegWrite(pThis, R9_RIGHT_DIFFERENTIAL_INPUT_VOLUME_CONTROL, R_In_Vol);
			Adau1761_RegWrite(pThis, R10_RECORD_MICROPHONE_BIAS_CONTROL, 0x01); // Bias Control enabled and set to default.
			Adau1761_RegWrite(pThis, R11_ALC_CONTROL_0, 0x13); // ALC controls PGA - Here its set to stereo.
			Adau1761_RegWrite(pThis, R5_RECORD_MIXER_LEFT_CONTROL_1, 0x10); //20dB LDBOOST, Line In Disabled.
			Adau1761_RegWrite(pThis, R7_RECORD_MIXER_RIGHT_CONTROL_1, 0x10);//20dB LD Boost, Line In Disabled.
			break;
	case LINE_IN:
			/* Line IN and LD Boost (output of PGA) Configurations */
			Adau1761_RegWrite(pThis, R5_RECORD_MIXER_LEFT_CONTROL_1, 0x07); //Mute Mic, Enable Line In.
			Adau1761_RegWrite(pThis, R7_RECORD_MIXER_RIGHT_CONTROL_1, 0x07);//Mute Mic, Enable Line In.
			break;

	case LINE_MIC:
			/* MIC configurations - Refer Page 30 */
			Adau1761_RegWrite(pThis, R8_LEFT_DIFFERENTIAL_INPUT_VOLUME_CONTROL, L_In_Vol);
			Adau1761_RegWrite(pThis, R9_RIGHT_DIFFERENTIAL_INPUT_VOLUME_CONTROL, R_In_Vol);
			Adau1761_RegWrite(pThis, R10_RECORD_MICROPHONE_BIAS_CONTROL, 0x01); // Bias Control enabled and set to default.
			Adau1761_RegWrite(pThis, R11_ALC_CONTROL_0, 0x13); // ALC controls PGA - Here its set to stereo.
			/* Line IN and LD Boost (output of PGA) Configurations */
			Adau1761_RegWrite(pThis, R5_RECORD_MIXER_LEFT_CONTROL_1, 0x17); //Zero gain LD Boost, Enable Line In.
			Adau1761_RegWrite(pThis, R7_RECORD_MIXER_RIGHT_CONTROL_1, 0x17);//Zero gain LD Boost, Enable Line In.
			break;

	default:
		printf("Input Path Select Exception\n");
	}
}

/* Output Volume Control - HPH Vol could be controlled the same way */
void AudioPlayer_SetOut_LineVol(tAdau1761 *pThis, unsigned short Line_Vol){

	/* LINE OUT Vol Control - Range: 0x03 - 0xFF (-57dB to 6dB) */
	Adau1761_RegWrite(pThis, R31_PLAYBACK_LINE_OUTPUT_LEFT_VOLUME_CONTROL, Line_Vol);
	Adau1761_RegWrite(pThis, R32_PLAYBACK_LINE_OUTPUT_RIGHT_VOLUME_CONTROL, Line_Vol);

}

/* init the axi_iis_adi component */
void Adau1761_IIS_Init(tAdau1761 *pThis) {

	unsigned char bclk_div =0x0;
	unsigned int bclk_rate=0x0;

	/* Reset I2S TX/RX */
	Xil_Out32(AXI_I2S_REGISTER(AXI_I2S_REG_RESET), (AXI_I2S_RESET_TX_FIFO | AXI_I2S_RESET_RX_FIFO));

	bclk_rate = AXI_I2S_RATE * AXI_I2S_BITS_PER_FRAME;
	bclk_div = (AXI_I2S_REF_CLK / bclk_rate) / 2 -1;

	/* set I2S sampling frequency */
	Adau1761_IIS_SetSamplingFreq(pThis, bclk_div);
	/* Enable I2S TX */
	Adau1761_IIS_TX_Enable();
}

void Adau1761_IIS_TX_Enable()
{
	 Xil_Out32(AXI_I2S_REGISTER(AXI_I2S_REG_CTRL), (AXI_I2S_CTRL_TX_EN | AXI_I2S_CTRL_RX_EN));
}

/* Init I2S clock, sampling freq */
void Adau1761_IIS_SetSamplingFreq(tAdau1761 *pThis, unsigned char bclk_div ){

	unsigned char word_size = AXI_I2S_BITS_PER_FRAME / 2 - 1;
	//configure I2S clock dividers
	Xil_Out32(AXI_I2S_REGISTER(AXI_I2S_REG_CLK_CTRL), (word_size<<16)|bclk_div);

}


/* ---------------------------------------------------------------------------- *
 * 								AUDIO_CODEC_CONFIG - ADAU1761					*
 * ---------------------------------------------------------------------------- *
 * Configures Audio codes's internal PLL. With MCLK = 10 MHz it configures the
 * PLL for a VCO frequency = 49.152 MHz, and an audio sample rate of 48 KHz.
 * ---------------------------------------------------------------------------- */
void Adau1761_Codec_Init(tAdau1761 *pThis) {

	unsigned char u8TxData[8], u8RxData[6];

	// Disable Core Clock
	Adau1761_RegWrite(pThis, R0_CLOCK_CONTROL, 0x0E);

	/* 	MCLK = 12.288 MHz
		R = 0100 = 4

		PLL required output = 1024x48 KHz
		(PLLout)			= 49.152 MHz

		PLLout/MCLK			= 49.152 MHz/12.288 MHz
							= 4 */

	// Write 6 bytes to R1 @ CODEC's Config Register address: 0x4002
	u8TxData[0] = 0x40; // Register write address [15:8]
	u8TxData[1] = 0x02; // Register write address [7:0]
	u8TxData[2] = 0x00; // byte 6 - M[15:8]
	u8TxData[3] = 0x00; // byte 5 - M[7:0]
	u8TxData[4] = 0x00; // byte 4 - N[15:8]
	u8TxData[5] = 0x00; // byte 3 - N[7:0]
	u8TxData[6] = 0x20; // byte 2 - 7 = reserved, bits 6:3 = R[3:0], 2:1 = X[1:0], 0 = PLL operation mode
	u8TxData[7] = 0x01; // byte 1 - 7:2 = reserved, 1 = PLL Lock, 0 = Core clock enable

	// Write bytes to PLL Control register R1 @ 0x4002
	XIicPs_MasterSendPolled(&(pThis->Iic), u8TxData, 8, (IIC_SLAVE_ADDR >> 1));
	while(XIicPs_BusIsBusy(&pThis->Iic));

	// CODEC's Config Register address set: 0x4002
	u8TxData[0] = 0x40;
	u8TxData[1] = 0x02;

	// Poll PLL Lock bit
	do {
		XIicPs_MasterSendPolled(&pThis->Iic, u8TxData, 2, (IIC_SLAVE_ADDR >> 1));
		while(XIicPs_BusIsBusy(&pThis->Iic));
		XIicPs_MasterRecvPolled(&pThis->Iic, u8RxData, 6, (IIC_SLAVE_ADDR >> 1));
		while(XIicPs_BusIsBusy(&pThis->Iic));
	}
	while((u8RxData[5] & 0x02) == 0); // while not locked

	Adau1761_RegWrite(pThis, R0_CLOCK_CONTROL, 0x0F);	// 1111
												// bit 3:		CLKSRC = PLL Clock input
												// bits 2:1:	INFREQ = 1024 x fs
												// bit 0:		COREN = Core Clock enabled

	//Initialize ADAU1761 control registers. (Refer to Page 51 of the ADAU1761 data sheet)

	/*Initialize CODEC I2S port*/
	Adau1761_RegWrite(pThis, R16_SERIAL_PORT_CONTROL_1, 0x00);

	/* Set ADC/DAC sampling rate - 32kHz*/
	Adau1761_RegWrite(pThis, R17_CONVERTER_CONTROL_0, 0x05);
	Adau1761_RegWrite(pThis, R64_SERIAL_PORT_SAMPLING_RATE, 0x05);

	/*ADC/DAC CNTL - 2 ADC/DAC enabled; others set to default */
	Adau1761_RegWrite(pThis, R19_ADC_CONTROL, 0x13);
	Adau1761_RegWrite(pThis, R36_DAC_CONTROL_0, 0x03);

	/* No POWER MANAGEMENT set here - all enabled for now */
	Adau1761_RegWrite(pThis, R35_PLAYBACK_POWER_MANAGEMENT, 0x03);

    /* Input/Output routes of ADC/DAC, clock control */
	Adau1761_RegWrite(pThis, R58_SERIAL_INPUT_ROUTE_CONTROL, 0x01);
	Adau1761_RegWrite(pThis, R59_SERIAL_OUTPUT_ROUTE_CONTROL, 0x01);
	Adau1761_RegWrite(pThis, R65_CLOCK_ENABLE_0, 0x7F);
	Adau1761_RegWrite(pThis, R66_CLOCK_ENABLE_1, 0x03);


	/* Mixer - Enable's sources that influence the play back/Audio Input path - Refer Page 29 and 35 */

	/* Audio Input Mixer */
	Adau1761_RegWrite(pThis, R4_RECORD_MIXER_LEFT_CONTROL_0, 0x01); //Mixer 1 Enable.
	Adau1761_RegWrite(pThis, R6_RECORD_MIXER_RIGHT_CONTROL_0, 0x01);//Mixer 2 Enable.


	/* Play back Path Mixer to the DAC's */
	Adau1761_RegWrite(pThis, R22_PLAYBACK_MIXER_LEFT_CONTROL_0, 0x21);
	Adau1761_RegWrite(pThis, R24_PLAYBACK_MIXER_RIGHT_CONTROL_0, 0x41);
	Adau1761_RegWrite(pThis, R26_PLAYBACK_LR_MIXER_LEFT_LINE_OUTPUT_CONTROL, 0x03);
	Adau1761_RegWrite(pThis, R27_PLAYBACK_LR_MIXER_RIGHT_LINE_OUTPUT_CONTROL, 0x09);

	/* Volume Control Options */

	/* HPH OUT Vol Control - Range: 0x03 - 0xFF (-57dB to 6dB) */
	Adau1761_RegWrite(pThis, R29_PLAYBACK_HEADPHONE_LEFT_VOLUME_CONTROL, 0xE7);
	Adau1761_RegWrite(pThis, R30_PLAYBACK_HEADPHONE_RIGHT_VOLUME_CONTROL, 0xE7);

	/* LINE OUT Vol Control - Range: 0x03 - 0xFF (-57dB to 6dB) */
	Adau1761_RegWrite(pThis, R31_PLAYBACK_LINE_OUTPUT_LEFT_VOLUME_CONTROL, 0xFF);
	Adau1761_RegWrite(pThis, R32_PLAYBACK_LINE_OUTPUT_RIGHT_VOLUME_CONTROL, 0xFF);

}

/* ---------------------------------------------------------------------------- *
 * 									PS-IicConfig()									*
 * ---------------------------------------------------------------------------- *
 * IIC initialization with clock configuration.
 * ---------------------------------------------------------------------------- */
unsigned char Adau1761_I2CMaster_Init(tAdau1761 *pThis, unsigned int I2C_DeviceId, unsigned int I2C_CLK)
{
	XIicPs_Config *Config;

	Config = XIicPs_LookupConfig(I2C_DeviceId);
	XIicPs_CfgInitialize(&(pThis->Iic), Config, Config->BaseAddress);
	XIicPs_SetSClk(&(pThis->Iic), I2C_CLK);

	return PASS;
}


/* init the AXI streaming FIFO */
void Adau1761_FIFO_Init(tAdau1761 *pThis)
{
	//Reset AXI-Streaming FIFO Transmit side
	*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_TX_RESET) = FIFO_TX_RESET_VALUE;
	//Initialize the TX FIFO buffer with 0
	*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_TX_DES) = 0x00;

	//Reset AXI-Streaming FIFO Transmit side
	*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_RX_RESET) = FIFO_RX_RESET_VALUE;
	//Initialize the TX FIFO buffer with 0
	*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_RX_DES) = 0x00;

	/* Reset the core and generate the external reset by writing to the Local Link Reset Register. */
	//*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_LLR_OFFSET) = FIFO_LLR_RESET_VALUE;

	/* clear all pending interrupts */
	*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_INT_STATUS) = 0xffffffff;

	/* Enable TFPE interrupt to propagate */
	*(volatile u32 *) (FIFO_BASE_ADDR + FIFO_INT_ENABLE) =  (FIFO_INT_RFPF) | (FIFO_INT_TFPE);

}

/* ---------------------------------------------------------------------------- *
 * 								Adau1761_RegWrite									*
 * ---------------------------------------------------------------------------- *
 * Function to write one byte (8-bits) to one of the registers in the Audio
 * Controller via I2C
 * ---------------------------------------------------------------------------- */
void Adau1761_RegWrite(tAdau1761 *pThis, unsigned char u8RegAddr, unsigned char u8Data) {

	unsigned char u8TxData[3];

	u8TxData[0] = 0x40;
	u8TxData[1] = u8RegAddr;
	u8TxData[2] = u8Data;

	XIicPs_MasterSendPolled(&pThis->Iic, u8TxData, 3, (IIC_SLAVE_ADDR >> 1));
	while(XIicPs_BusIsBusy(&pThis->Iic));
}
