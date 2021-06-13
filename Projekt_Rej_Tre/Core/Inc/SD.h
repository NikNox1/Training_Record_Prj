#ifndef INC_SD_H_
#define INC_SD_H_
#include "fatfs.h"

void MountSD(void);

typedef struct{
	FRESULT FatFsResult;
	FATFS SdFatFs;
	FIL SdCardFile;
	uint8_t bytes;
	char data[128];
}SD_CARD;
#endif /* INC_SD_H_ */
