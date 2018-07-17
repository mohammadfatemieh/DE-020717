/**
 ******************************************************************************
 * File Name          : one_wire.c
 * Date               : 17/11/2016 00:59:00
 * Description        : one wire communication modul
 ******************************************************************************
 *
 *
 ******************************************************************************
 */
 
 
/* Includes ------------------------------------------------------------------*/
#include "one_wire.h"
#include "main.h"
#include "thermostat.h"
#include "common.h"
#include "display.h"


/* Imported Type  ------------------------------------------------------------*/
/* Imported Variable  --------------------------------------------------------*/
/* Imported Function  --------------------------------------------------------*/
/* Private Type --------------------------------------------------------------*/
OnewireStateTypeDef OnewireState;
TempSensorTypeDef ds18b20_1;
TempSensorTypeDef ds18b20_2;
TempSensorTypeDef ds18b20_3;
TempSensorTypeDef ds18b20_4;
TempSensorTypeDef ds18b20_5;
TempSensorTypeDef ds18b20_6;
TempSensorTypeDef ds18b20_7;
TempSensorTypeDef ds18b20_8;


/* Private Define ------------------------------------------------------------*/
#define ONEWIRE_TIMEOUT				800
#define ONEWIRE_SEARCH_ROM			((uint8_t)0xf0)
#define ONEWIRE_READ_ROM			((uint8_t)0x33)
#define ONEWIRE_MATCH_ROM			((uint8_t)0x55)
#define ONEWIRE_SKIP_ROM			((uint8_t)0xcc)
#define ONEWIRE_ALARM_SEARCH		((uint8_t)0xec)
#define ONEWIRE_CONVERT				((uint8_t)0x44)
#define ONEWIRE_WRITE_SCRATCHPAD	((uint8_t)0x4e)
#define ONEWIRE_READ_SCRATCHPAD		((uint8_t)0xbe)
#define ONEWIRE_COPY_SCRATCHPAD		((uint8_t)0x48)
#define ONEWIRE_RACALL_E2			((uint8_t)0xb8)
#define ONEWIRE_READ_POWER_SUPPLY	((uint8_t)0xb4)


/* Private Variable ----------------------------------------------------------*/
__IO uint32_t onewire_timer;
 __IO uint32_t onewire_timeout_timer;
uint32_t onewire_flags;
uint8_t onewire_buffer[ONEWIRE_BUF_SIZE];
uint8_t onewire_last_family_discrepancy;
uint8_t onewire_last_discrepancy;
uint8_t onewire_last_device_flag;
uint8_t onewire_sensor_number;
uint8_t onewire_crc;


/* Private Macro -------------------------------------------------------------*/
/* Private Function Prototype ------------------------------------------------*/
uint8_t ONEWIRE_Reset(void);
uint8_t ONEWIRE_ReadByte(void);
void ONEWIRE_SendByte(uint8_t data);
void ONEWIRE_SendBit(uint8_t send_bit);
uint8_t ONEWIRE_ReceiveBit(void);
void ONEWIRE_Send(uint8_t *command, uint8_t lenght);
void ONEWIRE_Receive(uint8_t *data, uint8_t lenght);
uint8_t ONEWIRE_ReadROM(uint8_t *ow_address);
uint8_t ONEWIRE_CrcCheck(uint8_t *ow_address, uint8_t lenght);
void ONEWIRE_Pack (uint8_t cmd, uint8_t buffer[8]);
uint8_t ONEWIRE_Unpack (uint8_t buffer[8]);
uint16_t ONEWIRE_ScratchpadToTemperature(uint16_t scratchpad);
void ONEWIRE_ResetSearch(void);
uint8_t ONEWIRE_Search(TempSensorTypeDef* ds18b20, uint8_t* sensor_cnt);
uint8_t ONEWIRE_GetROM(uint8_t index);


/* Program Code  -------------------------------------------------------------*/
void ONEWIRE_Init(void)
{
	ONEWIRE_ResetSearch();
	
	if(ONEWIRE_Search(&ds18b20_1, &onewire_sensor_number)) 
	{
		ONEWIRE_SensorConnected();
		if(ONEWIRE_Search(&ds18b20_2, &onewire_sensor_number) == 0) return;
		if(ONEWIRE_Search(&ds18b20_3, &onewire_sensor_number) == 0) return;
		if(ONEWIRE_Search(&ds18b20_4, &onewire_sensor_number) == 0) return;
		if(ONEWIRE_Search(&ds18b20_5, &onewire_sensor_number) == 0) return;
		if(ONEWIRE_Search(&ds18b20_6, &onewire_sensor_number) == 0) return;
		if(ONEWIRE_Search(&ds18b20_7, &onewire_sensor_number) == 0) return;
		if(ONEWIRE_Search(&ds18b20_8, &onewire_sensor_number) == 0) return;
	}
	else 
	{ 
		ONEWIRE_SensorNotConnected();
		ONEWIRE_SetUsart(ONEWIRE_9600);
		HAL_HalfDuplex_EnableReceiver(&huart2);
		__HAL_UART_FLUSH_DRREGISTER(&huart2);
		HAL_UART_Receive_IT(&huart2, onewire_buffer, ONEWIRE_PACKET_SIZE);
		OnewireState = ONEWIRE_PACKET_PENDING;
	}
}


void ONEWIRE_Service(void)
{
	static uint8_t ow_pcnt = 0;
	static uint8_t ds18b20_cnt = 0;
	uint16_t sensor_temp;
	uint8_t e;
	
	if(!IsONEWIRE_TimerExpired())	// process timing
	{
		return;
	}
	else if(IsONEWIRE_SensorConnected()) // if ds18b20 digital temperature sensor used
	{
		/** ============================================================================*/
		/**			R E A D 	D S 1 8 B 2 0 	  S E N S O R 		V A L U E			*/
		/** ============================================================================*/
		if(ow_pcnt == 0)
		{
			ONEWIRE_Reset();
			ONEWIRE_SendByte(ONEWIRE_MATCH_ROM);
			
			for(e = 0; e < 8; e++)
			{
				if(ds18b20_cnt == 0) ONEWIRE_SendByte(ds18b20_1.rom_code[e]);
				else if(ds18b20_cnt == 1) ONEWIRE_SendByte(ds18b20_2.rom_code[e]);
				else if(ds18b20_cnt == 2) ONEWIRE_SendByte(ds18b20_3.rom_code[e]);
				else if(ds18b20_cnt == 3) ONEWIRE_SendByte(ds18b20_4.rom_code[e]);
				else if(ds18b20_cnt == 4) ONEWIRE_SendByte(ds18b20_5.rom_code[e]);
				else if(ds18b20_cnt == 5) ONEWIRE_SendByte(ds18b20_6.rom_code[e]);
				else if(ds18b20_cnt == 6) ONEWIRE_SendByte(ds18b20_7.rom_code[e]);
				else if(ds18b20_cnt == 7) ONEWIRE_SendByte(ds18b20_8.rom_code[e]);
			}
			
			ONEWIRE_SendByte(ONEWIRE_CONVERT);
			ONEWIRE_StartTimer(750);
			ow_pcnt = 1;
		}
		else if(ow_pcnt == 1)
		{
			ONEWIRE_Reset();
			ONEWIRE_SendByte(ONEWIRE_MATCH_ROM);
			
			for(e = 0; e < 8; e++)
			{
				if(ds18b20_cnt == 0) ONEWIRE_SendByte(ds18b20_1.rom_code[e]);
				else if(ds18b20_cnt == 1) ONEWIRE_SendByte(ds18b20_2.rom_code[e]);
				else if(ds18b20_cnt == 2) ONEWIRE_SendByte(ds18b20_3.rom_code[e]);
				else if(ds18b20_cnt == 3) ONEWIRE_SendByte(ds18b20_4.rom_code[e]);
				else if(ds18b20_cnt == 4) ONEWIRE_SendByte(ds18b20_5.rom_code[e]);
				else if(ds18b20_cnt == 5) ONEWIRE_SendByte(ds18b20_6.rom_code[e]);
				else if(ds18b20_cnt == 6) ONEWIRE_SendByte(ds18b20_7.rom_code[e]);
				else if(ds18b20_cnt == 7) ONEWIRE_SendByte(ds18b20_8.rom_code[e]);
			}
			
			ONEWIRE_SendByte(ONEWIRE_READ_SCRATCHPAD);
			ONEWIRE_Receive(onewire_buffer, 9);
			onewire_crc = ONEWIRE_CrcCheck(onewire_buffer, 8);
			
			if(onewire_crc == onewire_buffer[8])
			{
				sensor_temp = (onewire_buffer[1] << 8) + onewire_buffer[0];
				if(ds18b20_cnt == 0) ds18b20_1.temperature = ONEWIRE_ScratchpadToTemperature(sensor_temp);
				else if(ds18b20_cnt == 1) ds18b20_2.temperature = ONEWIRE_ScratchpadToTemperature(sensor_temp);
				else if(ds18b20_cnt == 2) ds18b20_3.temperature = ONEWIRE_ScratchpadToTemperature(sensor_temp);
				else if(ds18b20_cnt == 3) ds18b20_4.temperature = ONEWIRE_ScratchpadToTemperature(sensor_temp);
				else if(ds18b20_cnt == 4) ds18b20_5.temperature = ONEWIRE_ScratchpadToTemperature(sensor_temp);
				else if(ds18b20_cnt == 5) ds18b20_6.temperature = ONEWIRE_ScratchpadToTemperature(sensor_temp);
				else if(ds18b20_cnt == 6) ds18b20_7.temperature = ONEWIRE_ScratchpadToTemperature(sensor_temp);
				else if(ds18b20_cnt == 7) ds18b20_8.temperature = ONEWIRE_ScratchpadToTemperature(sensor_temp);
			}
			
			if(++ds18b20_cnt >= onewire_sensor_number) ds18b20_cnt = 0;
			ow_pcnt = 0;
		}		
	}
	else // use onewire bus as usart half duplex
	{
		if (OnewireState == ONEWIRE_PACKET_RECEIVED)
		{
			/** ==========================================================================*/
			/**			S E T		N E W		B U T T O N 		S T A T E			  */
			/** ==========================================================================*/
			if(onewire_buffer[2] == ACK)
			{
				BUTTON_SetNewState(GUI_ID_BUTTON_Dnd, (BUTTON_StateTypeDef)onewire_buffer[3]);
				BUTTON_SetNewState(GUI_ID_BUTTON_Sos, (BUTTON_StateTypeDef)onewire_buffer[4]);
				BUTTON_SetNewState(GUI_ID_BUTTON_Maid,(BUTTON_StateTypeDef)onewire_buffer[5]);
			}
			/** ==========================================================================*/
			/**			S E T		N E W		D A T E 	& 		T I M E				  */
			/** ==========================================================================*/
			if(onewire_buffer[6] == ACK)	// set new rtc date & time from controller clock
			{
				date.Date = onewire_buffer[7];
				date.WeekDay = onewire_buffer[8];
				date.Month = onewire_buffer[9];
				date.Year = onewire_buffer[10];
				time.Hours = onewire_buffer[11];
				time.Minutes = onewire_buffer[12];
				time.Seconds = onewire_buffer[13];
				HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BCD);
				HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BCD);
				HAL_RTC_WaitForSynchro(&hrtc);
			}
			/** ==========================================================================*/
			/**			S E T		T H E R M O S T A T 	P A R A M E T E R S			  */
			/** ==========================================================================*/
			if(onewire_buffer[14] == ACK)
			{
				Thermostat_1.valve = onewire_buffer[15];
				Thermostat_1.fan_mode = onewire_buffer[16];
				Thermostat_1.fan_speed = onewire_buffer[17];
				Thermostat_1.ctrl_mode = onewire_buffer[18];
				Thermostat_1.actual_temperature = ((onewire_buffer[19] << 8) + onewire_buffer[20]);
				Thermostat_1.set_temperature = ((onewire_buffer[21] << 8) + onewire_buffer[22]);
				Thermostat_1.set_temperature_diff = onewire_buffer[23];
				Thermostat_1.fan_low_speed_band = onewire_buffer[24];
				Thermostat_1.fan_middle_speed_band = onewire_buffer[25];
				Thermostat_1.fan_speed_diff = onewire_buffer[26];
                DISPLAY_SetpointUpdateSet();
			}
            /** ==========================================================================*/
			/**			    S E T	    	N T C        C O N S T A N T S		          */
			/** ==========================================================================*/
            if(onewire_buffer[27] == ACK)                           //(ACK = SET; NAK = SKEEP)
            {
                ambient_ntc_b_value = ((onewire_buffer[28] << 8) + onewire_buffer[29]);
                fancoil_ntc_b_value = ((onewire_buffer[30] << 8) + onewire_buffer[31]);
                
                if (onewire_buffer[32] == 1) 
                {
                    FANCOIL_RelayTypeReset();
                    FANCOIL_TriacTypeSet();
                }
                else if (onewire_buffer[32] == 2) 
                {
                    FANCOIL_TriacTypeReset();
                    FANCOIL_RelayTypeSet();
                }
            }
			/** ==========================================================================*/
			/**		    S E T		N E W		D I S P L A Y    M E S S A G E            */
			/** ==========================================================================*/
			if(onewire_buffer[33] == ACK)                           //(ACK = SET; NAK = SKEEP)
			{
                display_message_id = onewire_buffer[34];
                buzzer_mode = onewire_buffer[35];
                buzzer_repeat_time = onewire_buffer[36];
                display_message_time = onewire_buffer[37];
                DISPLAY_UpdateSet();
			}
            /** ==========================================================================*/
			/**             S E T       S E N S O R         S T A T U S                   */
			/** ==========================================================================*/
            if(onewire_buffer[38] == ACK)                           //(ACK = SET; NAK = SKEEP)
			{
                if (onewire_buffer[39] == 0) FANCOIL_NTC_SensorNotConnected();
                else if (onewire_buffer[39] == 1) FANCOIL_NTC_SensorConnected();
                
                if (onewire_buffer[40] == 0) AMBIENT_NTC_SensorNotConnected();
                else if (onewire_buffer[40] == 1) AMBIENT_NTC_SensorConnected();
                
                if (onewire_buffer[41] == 0) AMBIENT_LIGHT_SensorNotConnected();
                else if (onewire_buffer[41] == 1) AMBIENT_LIGHT_SensorConnected();
                
                if (onewire_buffer[42] == 0) ONEWIRE_SensorNotConnected();
                else if (onewire_buffer[42] == 1) ONEWIRE_SensorConnected();
                
                if (onewire_buffer[43] == 0) FAN_RPM_SensorNotConnected();
                else if (onewire_buffer[43] == 1) FAN_RPM_SensorConnected();
            }
            /** ==========================================================================*/
			/**                     S E T		    C O M M A N  D		                  */
			/** ==========================================================================*/
            // onewire_buffer[44] == ACK)                           //(ACK = SET; NAK = SKEEP)
            // onewire_buffer[45] = IsBUTTON_OpenDoorActiv()
            //
			OnewireState = ONEWIRE_PACKET_SEND;
		}
		else if(OnewireState == ONEWIRE_PACKET_SEND)
		{
			ClearBuffer(onewire_buffer,  ONEWIRE_BUF_SIZE);
			/** ==========================================================================*/
			/**			   S E T			P A C K E T			H E A D E R			  	  */
			/** ==========================================================================*/
			onewire_buffer[0] = ONEWIRE_CONTROLLER_ADDRESS;
			onewire_buffer[1] = ONEWIRE_THERMOSTAT_ADDRESS;
			/** ==========================================================================*/
			/**			S E T		 B U T T O N 		S T A T E			              */
			/** ==========================================================================*/
			onewire_buffer[2] = NAK;	// 						                            (ACK = SET; NAK = SKEEP)
			onewire_buffer[3] = BUTTON_GetState(GUI_ID_BUTTON_Dnd);	// DND BUTTON STATE 	(0 = RELEASED; 1 = PRESSED)
			onewire_buffer[4] = BUTTON_GetState(GUI_ID_BUTTON_Sos);	// SOS BUTTON STATE		(0 = RELEASED; 1 = PRESSED)
			onewire_buffer[5] = BUTTON_GetState(GUI_ID_BUTTON_Maid);// HM CALL BUTTON STATE	(0 = RELEASED; 1 = PRESSED)
            if ((onewire_buffer[3] + 
                 onewire_buffer[4] + 
                 onewire_buffer[5]) != 0)       onewire_buffer[2] = ACK;
			/** ==========================================================================*/
			/**			S E T		D A T E 	& 		T I M E				              */
			/** ==========================================================================*/
			onewire_buffer[6] = NAK;			// (ACK = SET; NAK = SKEEP)
			onewire_buffer[7] = date.Date;		// DATE DAY		(0x01 - 0x31)
			onewire_buffer[8] = date.WeekDay;	// DATE WEEKDAY	(0x01 - 0x07)
			onewire_buffer[9] = date.Month;		// DATE MONTH	(0x01 - 0x12)
			onewire_buffer[10] = date.Year;		// DATE YEAR	(0x00 - 0x99)
			onewire_buffer[11] = time.Hours;	// TIME HOURS	(0x00 - 0x23)
			onewire_buffer[12] = time.Minutes;	// TIME MINUTE	(0x00 - 0x59)
			onewire_buffer[13] = time.Seconds;	// TIME SECONDS	(0x00 - 0x59)
			/** ==========================================================================*/
			/**			S E T		T H E R M O S T A T 	P A R A M E T E R S			  */
			/** ==========================================================================*/
			onewire_buffer[14] = NAK;						//				(ACK = SET; NAK = SKEEP)
			onewire_buffer[15] = Thermostat_1.valve;		// VALVE		(0 = OFF; 1 = COOLING VALVE ON)
			onewire_buffer[16] = Thermostat_1.fan_mode;		// FAN_MODE		(0 = AUTO; 1 = LOW; 2 = MIDDLE; 3 = HIGH)
			onewire_buffer[17] = Thermostat_1.fan_speed;	// FAN_SPEED	(0 = OFF; 1 = LOW; 2 = MIDDLE; 3 = HIGH)
			onewire_buffer[18] = Thermostat_1.ctrl_mode;	// CONTROL MODE	(0 = OFF; 1 = COOLING; 2 = HEATING)
			onewire_buffer[19] = (Thermostat_1.actual_temperature >> 8);	// ACTUAL TEMPERATURE MSB		
			onewire_buffer[20] = (Thermostat_1.actual_temperature & 0xff);	// ACTUAL TEMPERATURE LSB		
			onewire_buffer[21] = (Thermostat_1.set_temperature >> 8);		// SETPOINT TEMPERATURE MSB			
			onewire_buffer[22] = (Thermostat_1.set_temperature & 0xff);		// SETPOINT TEMPERATURE LSB			
			onewire_buffer[23] = Thermostat_1.set_temperature_diff;			// SETPOINT TEMPERATURE DIFFERENCE			
			onewire_buffer[24] = Thermostat_1.fan_low_speed_band;			// FAN LOW SPEED BAND DIFFERENCE			
			onewire_buffer[25] = Thermostat_1.fan_middle_speed_band;		// FAN MIDDLE SPEED BAND DIFFERENCE		
			onewire_buffer[26] = Thermostat_1.fan_speed_diff;				// FAN SPEED TRESHOLD DIFFERENCE
            /** ==========================================================================*/
			/**			    S E T	    	N T C        C O N S T A N T S		          */
			/** ==========================================================================*/
            onewire_buffer[27] = NAK;                       //				(ACK = SET; NAK = SKEEP)
            onewire_buffer[28] = (ambient_ntc_b_value >> 8);
            onewire_buffer[29] = (ambient_ntc_b_value & 0xff);
            onewire_buffer[30] = (fancoil_ntc_b_value >> 8);
            onewire_buffer[31] = (fancoil_ntc_b_value & 0xff);
            if (IsFANCOIL_TriacTypeActiv())         onewire_buffer[32] = 1;
            else if (IsFANCOIL_RelayTypeActiv())    onewire_buffer[32] = 2;
			/** ==========================================================================*/
			/**		S E T		A C T I V		D I S P L A Y    M E S S A G E		      */
			/** ==========================================================================*/
			onewire_buffer[33] = NAK; 								//(ACK = SET; NAK = SKEEP)
            onewire_buffer[34] = display_message_id;
            onewire_buffer[35] = buzzer_mode;
            onewire_buffer[36] = buzzer_repeat_time;
            onewire_buffer[37] = display_message_time;
            /** ==========================================================================*/
			/**             S E T       S E N S O R         S T A T U S                   */
			/** ==========================================================================*/
            onewire_buffer[38] = NAK; 								//(ACK = SET; NAK = SKEEP)
            if (IsFANCOIL_NTC_SensorConnected())    onewire_buffer[39] = 1;
            if (IsAMBIENT_NTC_SensorConnected())    onewire_buffer[40] = 1;
            if (IsAMBIENT_LIGHT_SensorConnected())  onewire_buffer[41] = 1;
            if (IsONEWIRE_SensorConnected())        onewire_buffer[42] = 1;
            if (IsFAN_RPM_SensorConnected())        onewire_buffer[43] = 1;
            if ((onewire_buffer[39] + 
                 onewire_buffer[40] + 
                 onewire_buffer[41] + 
                 onewire_buffer[42] +
                 onewire_buffer[43]) != 0)          onewire_buffer[38] = ACK;
            /** ==========================================================================*/
			/**                     S E T		    C O M M A N  D		                  */
			/** ==========================================================================*/
            onewire_buffer[44] = NAK; 								//(ACK = SET; NAK = SKEEP)
            
            if(IsBUTTON_OpenDoorActiv())
            {
                BUTTON_OpenDoorReset();
                onewire_buffer[44] = ACK;
                onewire_buffer[45] = 1;
            }
			/** ==========================================================================*/
			/**		G E T		P A C K E T		C R C		A N D		S E N D			  */
			/** ==========================================================================*/
			onewire_buffer[63] = CalcCRC(onewire_buffer, 63);
			HAL_Delay(10);
			HAL_UART_Transmit(&huart2, onewire_buffer, ONEWIRE_PACKET_SIZE, ONEWIRE_TRANSFER_TIMEOUT);
			while (huart2.gState != HAL_UART_STATE_READY) continue;
            __HAL_UART_FLUSH_DRREGISTER(&huart2);
			HAL_UART_Receive_IT(&huart2, onewire_buffer, ONEWIRE_PACKET_SIZE);
			OnewireState = ONEWIRE_PACKET_PENDING;
		}
	}
}


uint8_t ONEWIRE_Reset(void)
{
	ONEWIRE_SetUsart(ONEWIRE_9600);
	onewire_buffer[0] = 0xf0;
	HAL_UART_Transmit(&huart2, onewire_buffer, 1, ONEWIRE_TIMEOUT);
	HAL_UART_Receive(&huart2, onewire_buffer, 1, ONEWIRE_TIMEOUT);
	ONEWIRE_SetUsart(ONEWIRE_115200);
	if((onewire_buffer[0] != 0xf0) && (onewire_buffer[0] != 0x00) && (onewire_buffer[0] != 0xff)) return (1);
	else return(0);	
}


void ONEWIRE_SetUsart(uint32_t setup)
{
	huart2.Instance        		= USART2;
	huart2.Init.BaudRate		= setup;
	huart2.Init.WordLength 		= UART_WORDLENGTH_8B;
	huart2.Init.StopBits   		= UART_STOPBITS_1;
	huart2.Init.Parity     		= UART_PARITY_NONE;
	huart2.Init.HwFlowCtl  		= UART_HWCONTROL_NONE;
	huart2.Init.Mode       		= UART_MODE_TX_RX;
	huart2.Init.OverSampling	= UART_OVERSAMPLING_16;
	huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    
	if (HAL_UART_DeInit(&huart2) != HAL_OK) Error_Handler();
	if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();
}


void ONEWIRE_SendByte(uint8_t data)
{
	uint8_t i;
	
	for(i = 0; i < 8; i++, (data = data >> 1))
	{
		ONEWIRE_SendBit(data & 0x01);
	}
}


uint8_t ONEWIRE_ReadByte(void)
{
	uint8_t rd_byte, i;
	
	for(i = 0; i < 8; i++)
	{
		rd_byte = (rd_byte >>1)+ 0x80* ONEWIRE_ReceiveBit();
	}
	
	return rd_byte;
}


uint8_t ONEWIRE_ReceiveBit(void)
{
	uint8_t txd, rxd;
	txd = 0xff;	
	HAL_UART_Transmit(&huart2, &txd, 1, ONEWIRE_TIMEOUT);	
	HAL_UART_Receive(&huart2, &rxd, 1, ONEWIRE_TIMEOUT);	
	if(rxd == txd) return(1U);
	else return(0);
}


void ONEWIRE_SendBit(uint8_t send_bit)
{
	uint8_t txb, rxb;	
	if(send_bit == 0)  txb = 0x00;
	else txb = 0xff;	
	HAL_UART_Transmit(&huart2, &txb, 1, ONEWIRE_TIMEOUT);
	HAL_UART_Receive(&huart2, &rxb, 1, ONEWIRE_TIMEOUT);
}


void ONEWIRE_Send(uint8_t *command, uint8_t lenght)
{
	uint32_t i;
	
	uint32_t one_wire_lenght = lenght * 8;
	
	for (i = 0;  i < lenght; i++) 
	{
		ONEWIRE_Pack(command[i], &(onewire_buffer[i * 8]));
	}
	
	HAL_UART_Transmit(&huart2, onewire_buffer, one_wire_lenght, ONEWIRE_TIMEOUT);
}


void ONEWIRE_Receive(uint8_t *data, uint8_t lenght)
{
	uint32_t i;
	uint32_t onewire_lenght = lenght * 8;
	uint8_t tx_byte = 0xff;
	
	for(i = 0; i < onewire_lenght; i++)
	{
		HAL_UART_Transmit(&huart2, &tx_byte, 1, ONEWIRE_TIMEOUT);
		HAL_UART_Receive(&huart2, &onewire_buffer[i], 1, ONEWIRE_TIMEOUT);
	}
	
	for(i = 0; i < lenght; i++)
	{
		data[i] = ONEWIRE_Unpack(&(onewire_buffer[i * 8]));
	}
}


uint8_t ONEWIRE_ReadROM(uint8_t *ow_address)
{
	uint8_t crc;
	
	if(ONEWIRE_Reset() != 0)
	{
		ONEWIRE_SendByte(ONEWIRE_READ_ROM);
		ONEWIRE_Receive(ow_address, 8);
		crc = ONEWIRE_CrcCheck(ow_address, 7);
		if((crc != ow_address[7]) || (crc == 0))return (1);
		else return(0);
	}
	else return (2);
}


uint8_t ONEWIRE_CrcCheck(uint8_t *ow_address, uint8_t lenght)
{
	uint8_t crc=0;
	uint8_t i, j;

	for (i = 0; i < lenght; i++) 
	{
		uint8_t inbyte = ow_address[i];
		
		for (j = 0; j < 8; j++) 
		{
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) 
			crc ^= 0x8C;
			inbyte >>= 1;
		}
	}
	
	return crc;
}


void ONEWIRE_Pack(uint8_t command, uint8_t buffer[8])
{
	uint32_t i;
	
	for (i = 0;  i < 8; i++)
	{
		buffer[i] = (command & (1 << i)) ? 0xff : 0x00;
	}
}


uint8_t ONEWIRE_Unpack (uint8_t buffer[8])
{
	uint32_t i;
	uint8_t res = 0;

	for (i = 0; i < 8; i++) 
	{
		if (buffer[i] == 0xff)
		{
			res |=  (1 << i);
		}
	}

	return res;
}


uint16_t ONEWIRE_ScratchpadToTemperature(uint16_t scratchpad) 
{
    uint16_t result;
	
	if((scratchpad & 0xfc00) == 0xfc00)
	{
		scratchpad = ~scratchpad + 1;
		result = scratchpad >> 4; 							// cijelobrojni dio temperature
		result *= 10; 										// 22 -> 220
		result += (((scratchpad & 0x000f) *625) / 1000);
		result |= 0x8000; 									// add minus sign
	}
	else
	{
		result = scratchpad >> 4; 							// cijelobrojni dio temperature
		result *= 10; 										// 22 -> 220
		result += (((scratchpad & 0x000f) *625) / 1000);	// add decimal part
	}
    
    return result;
}


void ONEWIRE_ResetSearch(void) 
{
	onewire_last_discrepancy = 0;
	onewire_last_family_discrepancy = 0;
	onewire_last_device_flag = 0;
	onewire_sensor_number = 0;
}


uint8_t ONEWIRE_Search(TempSensorTypeDef* ds18b20, uint8_t* sensor_cnt) 
{
	static uint8_t init_cnt = 0;
	uint8_t last_zero, rom_byte_number, search_result;
	uint8_t id_bit, cmp_id_bit, id_bit_number;
	uint8_t rom_byte_mask, search_direction;

	id_bit_number = 1;
	last_zero = 0;
	rom_byte_number = 0;
	rom_byte_mask = 1;

	if (onewire_last_device_flag == 0)
	{
		if (ONEWIRE_Reset() == 0)
		{
			onewire_last_discrepancy = 0;
			onewire_last_device_flag = 0;
			onewire_last_family_discrepancy = 0;
			return (0);
		}

		ONEWIRE_SendByte(ONEWIRE_SEARCH_ROM); 

		do{
			id_bit = ONEWIRE_ReceiveBit();
			cmp_id_bit = ONEWIRE_ReceiveBit();
			
			if ((id_bit == 1) && (cmp_id_bit == 1)) break;
			else
			{
				if (id_bit != cmp_id_bit) search_direction = id_bit;  // bit write value for search
				else
				{
					if (id_bit_number < onewire_last_discrepancy)
					{
						search_direction = ((ds18b20->rom_code[rom_byte_number] & rom_byte_mask) > 0);
					}
					else search_direction = (id_bit_number == onewire_last_discrepancy);
					
					if (search_direction == 0)
					{
						last_zero = id_bit_number;
						if (last_zero < 9)  onewire_last_family_discrepancy = last_zero;
					}
				}
				
				if (search_direction == 1) ds18b20->rom_code[rom_byte_number] |= rom_byte_mask;
				else ds18b20->rom_code[rom_byte_number] &= ~rom_byte_mask;
				
				ONEWIRE_SendBit(search_direction);
				id_bit_number++;
				rom_byte_mask <<= 1;
				
				if (rom_byte_mask == 0)
				{
					rom_byte_number++;
					rom_byte_mask = 1;
				}
			}
		} while(rom_byte_number < 8);
		
		if (!(id_bit_number < 65))
		{
			search_result = 1;
			onewire_last_discrepancy = last_zero;
			if (onewire_last_discrepancy == 0) onewire_last_device_flag = 1;
		}
	}
	
	if ((search_result == 0) || (ds18b20->rom_code[0] == 0))
	{
		onewire_last_discrepancy = 0;
		onewire_last_device_flag = 0;
		onewire_last_family_discrepancy = 0;
		return (0);
	} 
	else 
	{
		init_cnt++;
		*sensor_cnt = init_cnt;
		ds18b20->sensor_id = init_cnt;
		return (init_cnt);
	}	
}


void ONEWIRE_Select(uint8_t* addr) 
{
	uint8_t i;
	
	ONEWIRE_SendByte(ONEWIRE_MATCH_ROM);
	
	for (i = 0; i < 8; i++) 
	{
		ONEWIRE_SendByte(*(addr + i));
	}
}


/******************************   END OF FILE  **********************************/
