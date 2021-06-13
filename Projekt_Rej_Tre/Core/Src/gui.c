
#include "gui.h"
#include "main.h"
#include "fix.h"
#include "nofix.h"
#include "TFT_ILI9341.h"
#include "XPT2064.h"
#include "GFX_Color.h"
#include "GFX_EnhancedFonts.h"
#include "rtc.h"
#include <stdio.h>
#include <SD.h>
#include "usart.h"
#include "gps_neo6.h"

#define DRAW_WINDOW_X_START 0
#define DRAW_WINDOW_X_STOP ILI9341_TFTWIDTH
#define DRAW_WINDOW_Y_START 30
#define DRAW_WINDOW_Y_STOP (ILI9341_TFTHEIGHT-60)

#define TOGGLE_BUTTON_X ILI9341_TFTWIDTH-130
#define TOGGLE_BUTTON_Y 3
#define TOGGLE_BUTTON_WIDTH 120
#define TOGGLE_BUTTON_HEIGHT 26

#define TIME_X 0
#define TIME_Y 210
#define TIME_WIDTH 130
#define TIME_HEIGHT 26

#define DATE_X 200
#define DATE_Y 210
#define DATE_WIDTH 120
#define DATE_HEIGHT 26

#define USED_COLORS	5

typedef enum
{
	GUI_INIT, // Build GUI
	GUI_DRAW, // Read Touch and draw pixels
	GUI_TOGGLE // Clear drawing area
} GuiState;

GuiState State = GUI_INIT; // Initialization state for Paint State Machine

uint16_t UsedColors[USED_COLORS] = {ILI9341_BLACK, ILI9341_BLUE, ILI9341_GREEN, ILI9341_RED, ILI9341_ORANGE}; // Colors table

uint16_t CurrentColor = ILI9341_BLACK; // Default color

uint16_t CurrentButton = 0; //0 - start	1 - stop
uint8_t CurrentFix; //0 - nofix	1 - fix
uint8_t  CompareFix;


uint8_t CompareSeconds;

RTC_TimeTypeDef RtcTime;
RTC_DateTypeDef RtcDate;
uint8_t TrainingTimeBuffer[64];
double TrainingSpeed=0;
uint8_t counter=0;

extern uint32_t Timer;
extern SD_CARD SdCard;



void FixIndicator(void)
{
	if(CurrentFix == 0)	ILI9341_DrawImage(120, 0, nofix, 30, 30);
	else if(CurrentFix == 1) ILI9341_DrawImage(120, 0, fix, 30, 30);
}

//
// Draw Clearing Button above the drawing area
//
void StartButton(void)
{
	GFX_DrawFillRectangle(TOGGLE_BUTTON_X, TOGGLE_BUTTON_Y, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT, ILI9341_DARKGREEN); //
	  //ILI9341_ClearArea(TOGGLE_BUTTON_X, TOGGLE_BUTTON_Y, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT, ILI9341_DARKGREEN); //
	EF_PutString((const uint8_t*)"START", TOGGLE_BUTTON_X+5, TOGGLE_BUTTON_Y, ILI9341_WHITE, BG_TRANSPARENT, ILI9341_DARKGREEN); //

}

void StopButton(void)
{
	GFX_DrawFillRectangle(0, 40, 360, 120, ILI9341_WHITE); // Button Color

	GFX_DrawFillRectangle(TOGGLE_BUTTON_X, TOGGLE_BUTTON_Y, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT, ILI9341_DARKGREEN); //
	  //ILI9341_ClearArea(TOGGLE_BUTTON_X, TOGGLE_BUTTON_Y, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT, ILI9341_DARKGREEN); //
	EF_PutString((const uint8_t*)"STOP", TOGGLE_BUTTON_X+5, TOGGLE_BUTTON_Y, ILI9341_WHITE, BG_TRANSPARENT, ILI9341_DARKGREEN); //
}

void TimeReset(void)
{
	RTC_TimeTypeDef sTime = {0};

	  hrtc.Instance = RTC;
	  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
	  hrtc.Init.AsynchPrediv = 127;
	  hrtc.Init.SynchPrediv = 255;
	  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
	  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
	  if (HAL_RTC_Init(&hrtc) != HAL_OK)
	  {
	    Error_Handler();
	  }

	  /* USER CODE BEGIN Check_RTC_BKUP */

	  /* USER CODE END Check_RTC_BKUP */

	  /** Initialize RTC and set the Time and Date
	  */
	  sTime.Hours = 0x0;
	  sTime.Minutes = 0x0;
	  sTime.Seconds = 0x0;
	  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
	  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
	  {
	    Error_Handler();
	  }

}
void TimeAndDate(void)
{

		uint8_t Time[64];
		uint8_t Date[64];

		HAL_RTC_GetTime(&hrtc, &RtcTime, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&hrtc, &RtcDate, RTC_FORMAT_BIN);
		if(RtcTime.Seconds != CompareSeconds)
		{
			sprintf((char*)Date, "%02d.%02d.%02d",GpsState.Day, GpsState.Month, GpsState.Year);
			sprintf((char*)Time, "%02d:%02d:%02d",GpsState.Hour+2, GpsState.Minute,GpsState.Second);

			GFX_DrawFillRectangle(TIME_X, TIME_Y, TIME_WIDTH, TIME_HEIGHT, ILI9341_WHITE); // Button Color

			EF_PutString(Time, TIME_X+5, TIME_Y, ILI9341_BLACK, BG_TRANSPARENT, ILI9341_WHITE); // Button text

			GFX_DrawFillRectangle(DATE_X, DATE_Y, DATE_WIDTH, DATE_HEIGHT, ILI9341_WHITE); // Button Color

			EF_PutString(Date, DATE_X+5, DATE_Y, ILI9341_BLACK, BG_TRANSPARENT, ILI9341_WHITE); // Button text
			CompareSeconds = RtcTime.Seconds;
		}
}

void StatsDisplay(void)
{
	uint8_t Text[64]="Training stats:";
	uint8_t SpeedInfo[64];
	double AverageSpeed;

	AverageSpeed = TrainingSpeed/counter;
	sprintf((char*)SpeedInfo, "Avg speed:%.2lf km/h",AverageSpeed);

	GFX_DrawFillRectangle(TIME_X+70, TIME_Y-150, TIME_WIDTH+120, TIME_HEIGHT, ILI9341_WHITE); // Button Color

	GFX_DrawFillRectangle(TIME_X+10, TIME_Y-150, TIME_WIDTH+120, TIME_HEIGHT, ILI9341_WHITE);
	EF_PutString(Text, TIME_X+10, TIME_Y-170, ILI9341_BLACK, BG_TRANSPARENT, ILI9341_WHITE);

	GFX_DrawFillRectangle(TIME_X+10, TIME_Y-120, TIME_WIDTH+160, TIME_HEIGHT, ILI9341_WHITE); // Button Color
	EF_PutString(TrainingTimeBuffer, TIME_X+10, TIME_Y-120, ILI9341_BLACK, BG_TRANSPARENT, ILI9341_WHITE); // Button text

	GFX_DrawFillRectangle(TIME_X+10, TIME_Y-90, TIME_WIDTH+120, TIME_HEIGHT, ILI9341_WHITE); // Button Color
	EF_PutString(SpeedInfo, TIME_X+10, TIME_Y-90, ILI9341_BLACK, BG_TRANSPARENT, ILI9341_WHITE); // Button text

	;

}

uint8_t IsToggleButtonTouched(uint16_t x, uint16_t y)
{
	// Check if Touch point is higher than X begin of clear button
	if(x > TOGGLE_BUTTON_X)
	{
		// Check if Touch point is higher than X end of clear button
		if(x < (TOGGLE_BUTTON_X + TOGGLE_BUTTON_WIDTH))
		{
			// Check if Touch point is higher than Y begin of clear button
			if(y > TOGGLE_BUTTON_Y)
			{
				// Check if Touch point is higher than Y end of clear button
				if(y < (TOGGLE_BUTTON_Y+TOGGLE_BUTTON_HEIGHT))
				{
					// If we are sure that touched point was inside clear button - return 1
					return 1;
				}
			}
		}
	}
	// If clear button is not touched
	return 0;
}

//
// PAINT_INIT state function
//

void FixCheck(void)
{
		CurrentFix = NEO6_IsFix(&GpsState);
		if(CurrentFix!=CompareFix){
		FixIndicator();
		CompareFix = NEO6_IsFix(&GpsState);
		}

}

void InitScreen(void)
{
	// Clear whole display
	ILI9341_ClearDisplay(ILI9341_WHITE);
	// Title
	EF_PutString((const uint8_t*)"GPS", 5, 2, ILI9341_BLACK, BG_TRANSPARENT, ILI9341_GREEN);
	// Drawing area
	GFX_DrawRectangle(DRAW_WINDOW_X_START, DRAW_WINDOW_Y_START, DRAW_WINDOW_X_STOP, DRAW_WINDOW_Y_STOP, ILI9341_BLACK);
	// Current color indicator
	FixIndicator();
	// Clear button
	StartButton();
	FixCheck();
	// Color buttons



//	ColorButtons();

	State = GUI_DRAW;
}

//
// PAINT_DRAW state function
//
void DrawScreen(void)
{
	TimeAndDate();
	FixCheck();
	// Check if screen was touched
	if(XPT2046_IsTouched())
	{
		uint16_t x, y; // Touch points

		XPT2046_GetTouchPoint(&x, &y); // Get the current couched point


		// Check if Toggle button was touched
		if(IsToggleButtonTouched(x, y))
		{
			// Jump to Clearing state
			State = GUI_TOGGLE;
		}
	}
}

void DrawToggle(void)
{
	if(CurrentButton == 0){
		CurrentButton = 1;
		StopButton();
		TimeReset();
		//	zapis na SD
	}
	else if(CurrentButton == 1){
		CurrentButton = 0;
		StartButton();
		StatsDisplay();
	}



	State = GUI_DRAW;
}

void Gps(void)
{
	uint8_t Message[64];
	uint8_t MessageLength;

	NEO6_Task(&GpsState);

		  if((HAL_GetTick() - Timer) > 1000)
		  {
			  MessageLength = sprintf((char*)Message, "\033[2J\033[;H"); // Clear terminal and home cursor
			  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);

			  if(NEO6_IsFix(&GpsState))
			  {
				  if(CurrentButton)
				  {
									  SdCard.FatFsResult = f_open(&SdCard.SdCardFile, "dane_gps.txt", FA_WRITE|FA_OPEN_APPEND);

									  MessageLength = sprintf((char*)Message, "Time:%02d:%02d:%02d\n\r", GpsState.Hour+2, GpsState.Minute, GpsState.Second);
									  f_printf(&SdCard.SdCardFile, (char*)Message);
					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);

					  				  MessageLength = sprintf((char*)Message, "Date:%02d.%02d.20%02d\n\r", GpsState.Day, GpsState.Month, GpsState.Year);
					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);
					  				  f_printf(&SdCard.SdCardFile, (char*)Message);

					  				  MessageLength = sprintf((char*)Message, "Latitude: %.2f %c\n\r", GpsState.Latitude, GpsState.LatitudeDirection);
					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);
					  				  f_printf(&SdCard.SdCardFile, (char*)Message);

					  				  MessageLength = sprintf((char*)Message, "Longitude: %.2f %c\n\r", GpsState.Longitude, GpsState.LongitudeDirection);
					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);
					  				  f_printf(&SdCard.SdCardFile, (char*)Message);

//					  				  MessageLength = sprintf((char*)Message, "Altitude: %.2f m above sea level\n\r", GpsState.Altitude);
//					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);
//					  				  f_printf(&SdCard.SdCardFile, (char*)Message);

					  				  MessageLength = sprintf((char*)Message, "Speed:%.2f km/h\n\r", GpsState.SpeedKilometers);
					  				  TrainingSpeed+=GpsState.SpeedKilometers;

					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);

					  				  GFX_DrawFillRectangle(50, 90, 240, TIME_HEIGHT, ILI9341_WHITE); // Button Color

					  				  EF_PutString(Message, 50, 90, ILI9341_BLACK, BG_TRANSPARENT, ILI9341_WHITE); // Button text

					  				  f_printf(&SdCard.SdCardFile, (char*)Message);

//					  				  MessageLength = sprintf((char*)Message, "Satelites: %d\n\r", GpsState.SatelitesNumber);
//					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);
//					  				  f_printf(&SdCard.SdCardFile, (char*)Message);

//					  				  MessageLength = sprintf((char*)Message, "Dilution of precision: %.2f\n\r", GpsState.Dop);
//					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);
//					  				  f_printf(&SdCard.SdCardFile, (char*)Message);

//					  				  MessageLength = sprintf((char*)Message, "Horizontal dilution of precision: %.2f\n\r", GpsState.Hdop);
//					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);
//					  				  f_printf(&SdCard.SdCardFile, (char*)Message);

//					  				  MessageLength = sprintf((char*)Message, "Vertical dilution of precision: %.2f\n\r", GpsState.Vdop);
//					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);
//					  				  f_printf(&SdCard.SdCardFile, (char*)Message);

					  				SdCard.FatFsResult = f_close(&SdCard.SdCardFile);

					  				HAL_RTC_GetTime(&hrtc, &RtcTime, RTC_FORMAT_BIN);
									HAL_RTC_GetDate(&hrtc, &RtcDate, RTC_FORMAT_BIN);


									sprintf((char*)TrainingTimeBuffer, "Time:%02d:%02d:%02d\n\r", RtcTime.Hours, RtcTime.Minutes, RtcTime.Seconds);

									GFX_DrawFillRectangle(TIME_X+70, TIME_Y-150, TIME_WIDTH+80, TIME_HEIGHT, ILI9341_WHITE); // Button Color

									EF_PutString(TrainingTimeBuffer, TIME_X+70, TIME_Y-150, ILI9341_BLACK, BG_TRANSPARENT, ILI9341_WHITE); // Button text

									counter++;



				  }
				  else
				  {
					  counter =0;
					  MessageLength = sprintf((char*)Message, "Time:%02d:%02d:%02d\n\r", GpsState.Hour+2, GpsState.Minute, GpsState.Second);

					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);

					  				  MessageLength = sprintf((char*)Message, "Date:%02d.%02d.20%02d\n\r", GpsState.Day, GpsState.Month, GpsState.Year);
					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);


					  				  MessageLength = sprintf((char*)Message, "Latitude: %.2f %c\n\r", GpsState.Latitude, GpsState.LatitudeDirection);
					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);

					  				  MessageLength = sprintf((char*)Message, "Longitude: %.2f %c\n\r", GpsState.Longitude, GpsState.LongitudeDirection);
					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);

//					  				  MessageLength = sprintf((char*)Message, "Altitude: %.2f m above sea level\n\r", GpsState.Altitude);
//					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);

					  				  MessageLength = sprintf((char*)Message, "Speed:%f km/h\n\r",GpsState.SpeedKilometers);
					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);

//					  				  MessageLength = sprintf((char*)Message, "Satelites: %d\n\r", GpsState.SatelitesNumber);
//					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);

//					  				  MessageLength = sprintf((char*)Message, "Dilution of precision: %.2f\n\r", GpsState.Dop);
//					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);

//					  				  MessageLength = sprintf((char*)Message, "Horizontal dilution of precision: %.2f\n\r", GpsState.Hdop);
//					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);

//					  				  MessageLength = sprintf((char*)Message, "Vertical dilution of precision: %.2f\n\r", GpsState.Vdop);
//					  				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);

				  }

			  }
			  else
			  {
				  MessageLength = sprintf((char*)Message, "No Fix\n\r");
				  HAL_UART_Transmit(&huart2, Message, MessageLength, 1000);
			  }

			  Timer = HAL_GetTick();

		  }

}
void Gui(void)
{
	switch(State)
	{
	case GUI_INIT:
		InitScreen();
		break;
	case GUI_DRAW:
		DrawScreen();
		  break;
	case GUI_TOGGLE:
		DrawToggle();
		break;
	}
}
