#include "main.h"
#include <stdio.h>
#include <SD.h>
#include "string.h"
#include "gps_neo6.h"
#include "usart.h"
#include "fatfs.h"

extern SD_CARD SdCard;
extern uint32_t Timer;

void MountSD(void)
{


	SdCard.FatFsResult = f_mount(&SdCard.SdFatFs, "", 1);

			//
		    // FatFS mount init error check
		    //
		    if(SdCard.FatFsResult != FR_OK)
		    {
		    	  SdCard.bytes = sprintf(SdCard.data, "FatFS mount error.\n\r");
		    	  HAL_UART_Transmit(&huart2, (uint8_t*)SdCard.data, SdCard.bytes, 1000);
		    }
		    else
		    {
		    	SdCard.bytes = sprintf(SdCard.data, "FatFS mounted.\n\r");
		    	  HAL_UART_Transmit(&huart2, (uint8_t*)SdCard.data, SdCard.bytes, 1000);

		    	  //
		    	  // Open file on SD for writing
		    	  //
		    	  SdCard.FatFsResult = f_open(&SdCard.SdCardFile, "dane_gps.txt", FA_WRITE|FA_CREATE_ALWAYS);

		    	  //
		    	  // File open error check
		    	  //
		    	  if(SdCard.FatFsResult != FR_OK)
		    	  {
		    		  SdCard.bytes = sprintf(SdCard.data, "No dane_gps.txt file. Can't create.\n\r");
		    		  HAL_UART_Transmit(&huart2, (uint8_t*)SdCard.data, SdCard.bytes, 1000);
		    	  }
		    	  else
		    	  {
		    		  SdCard.bytes = sprintf(SdCard.data, "File opened.\n\r");
		    		  HAL_UART_Transmit(&huart2, (uint8_t*)SdCard.data, SdCard.bytes, 1000);

		  		  //
		  		  // Close file
		  		  //

		    		  SdCard.FatFsResult = f_close(&SdCard.SdCardFile);

		    		  SdCard.bytes = sprintf(SdCard.data, "File closed.\n\r");
		  		  HAL_UART_Transmit(&huart2, (uint8_t*)SdCard.data, SdCard.bytes, 1000);
	    	  }
	    }
}

