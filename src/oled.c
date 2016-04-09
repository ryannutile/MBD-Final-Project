
#include "oled.h"

void oled_init(){
	string line;
	uint8_t num_bitmaps[8][5];
	
	// write 1111-11 to first and last column
	
	for(int i = 1; i < OLED_SCRN_WIDTH-1){
		// write 100--001 to each column
	}
	
	
	for(int i = 0; i < OLED_BAND_NUM; i++){
		for(int j = 0; j < OLED_NUM_WIDTH; j++){
			// write num_bitmaps[i][j] to OLED 
		}
	}
}

// 52 pixels available - need to map percentage to this
void oled_updateDisplay(uint_32t * levelData){
	
	uint32_t writeData = 1;
	
	// For each band that we wish to display
	for(int i = 0; i < OLED_BAND_NUM; i++){
		
		for(int j = 0; j < levelData[i]){
			writeData |= 1;
			writeData << 1;
		}
		
		writeData << OLED_NUM_HEIGHT + 2;
		
		// For each column within the band
		
		for(int j = 0; j < OLED_BAND_WIDTH){
			// write the level to each column in the band
			column &= CLEAR_BITMASK;
			column |= writeData;
			
		}
	}
}