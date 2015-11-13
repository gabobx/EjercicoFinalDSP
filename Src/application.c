/**
  ******************************************************************************
  * @file    audioFilter.c 
  * @author  Gustavo Muro
  * @version V0.0.1
  * @date    30/05/2015
  * @brief   Filtrado de audio.
  ******************************************************************************
  * @attention
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions are met:
  *
  * 1. Redistributions of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  *
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  *
  * 3. Neither the name of the copyright holder nor the names of its
  *    contributors may be used to endorse or promote products derived from this
  *    software without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  * POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "application.h"
#include "ff.h"
#include "waveplayer.h"
#include "waverecorder.h"
#include "ff.h"    
#include "ff_gen_drv.h"
#include "usbh_diskio.h"
#include "main.h"
#include "audioFilter.h"
#include "utils.h"



/* Private typedef -----------------------------------------------------------*/
typedef enum
{
  APPSTATE_IDLE = 0,
  APPSTATE_MOUNT_FS,
  APPSTATE_UMOUNT_FS,
	APPSTATE_WRITE,
  APPSTATE_PLAY,
}appState_enum;

/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static FATFS USBDISKFatFs;           /* File system object for USB disk logical drive */
static char USBDISKPath[4];          /* USB Host logical drive path */
static appState_enum appState = APPSTATE_IDLE;

uint16_t conteo=0;
uint16_t led3on=0;
uint16_t primpulso=1;


/* Variable used by FatFs*/
static FIL FileRead;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
int32_t getDataCB(int16_t *pBuff, int32_t length)
{
	
	q15_t rms;
	
  UINT bytesread = 0;
  
  f_read(&FileRead, pBuff, length*sizeof(int16_t), (void *)&bytesread); 
  
  audioFilter_filter(pBuff, pBuff, bytesread/sizeof(int16_t));
	
	arm_rms_q15(pBuff,bytesread/sizeof(int16_t),&rms);
	
	printf("valor rms: %u \n",rms);
	if(rms>1000){
		if((conteo<5)&(led3on==0)){
			if(TickTock_Get() > 700000){
				TickTock_Start();
				BSP_LED_On(LED6);
				conteo++;
				primpulso=0;
			}
		}else if((conteo==5) & (TickTock_Get() > 800000)){
			TickTock_Start();
			BSP_LED_On(LED3);
			primpulso=1;
			conteo=0;
			led3on=1;
		}
	}else{
		LEDsState = LEDS_OFF;
		led3on=0;
	}
  
  return bytesread;
}
/* Exported functions ------------------------------------------------------- */

extern void application_init(void)
{
	
  /*##-1- Link the USB Host disk I/O driver ##################################*/
  if(FATFS_LinkDriver(&USBH_Driver, USBDISKPath) != 0)
  {
    Error_Handler();
  }
	
  TickTock_Init();
  audioFilter_init();

}

extern void application_task(void)
{
  UINT bytesread = 0;
  WAVE_FormatTypeDef waveformat;
  
  switch (appState)
  {
    case APPSTATE_IDLE:
      break;
    
    case APPSTATE_MOUNT_FS:
      if (f_mount(&USBDISKFatFs, (TCHAR const*)USBDISKPath, 0 ) != FR_OK ) 
      {
        /* FatFs initialization fails */
        Error_Handler();
      }
      else
      {
        appState = APPSTATE_PLAY;
      }
      break;
    
    case APPSTATE_UMOUNT_FS:
      f_mount(NULL, (TCHAR const*)"", 0);
      appState = APPSTATE_IDLE;
      break;
    
		case APPSTATE_WRITE:
			if (f_open(&FileRead, WAVE_NAME_COMPLETO, FA_READ) != FR_OK)
      {
        Error_Handler();
      }
      else
      {
        /* Read sizeof(WaveFormat) from the selected file */
        f_read (&FileRead, &waveformat, sizeof(waveformat), &bytesread);

        WaveRecord(waveformat, getDataCB);

        f_close(&FileRead);
      }
			appState = APPSTATE_PLAY;
			break;

    case APPSTATE_PLAY:
      if (f_open(&FileRead, WAVE_NAME_COMPLETO, FA_READ) != FR_OK)
      {
        Error_Handler();
      }
      else
      {    
        /* Read sizeof(WaveFormat) from the selected file */
        f_read (&FileRead, &waveformat, sizeof(waveformat), &bytesread);
        
        WavePlayerStart(waveformat, getDataCB, 70);
        
        f_close(&FileRead);
        
      }
      break;
    
    default:
      appState = APPSTATE_IDLE;
      break;
  }
}

extern void application_conect(void)
{
  appState = APPSTATE_MOUNT_FS;
}
extern void application_disconect(void)
{
  appState = APPSTATE_UMOUNT_FS;
}

/* End of file ---------------------------------------------------------------*/


