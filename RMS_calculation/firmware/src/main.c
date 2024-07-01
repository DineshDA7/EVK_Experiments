/*******************************************************************************
* Copyright (C) 2019 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/

/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes
#include <math.h>                
#define ADC_VREF                (3.3f)

//static void TransmitCompleteCallback(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle);
//static void ReceiveCompleteCallback(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle);
static void adc_sram_dma_callback(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle);

#define RX_BUFFER_SIZE 10
#define LED_ON    LED_Clear
#define LED_OFF   LED_Set

extern volatile int TimerFlag;
unsigned int ADC_result;
uint16_t ADC_voltage;
unsigned int ADC_status;
unsigned int TempValue=0xf0f;
unsigned int AvgRmsCount=0,Mvg_Avg_cnt,circular_pointer;
unsigned long int Avarage,RmsVoltage,AvgRmsVoltage;

volatile uint16_t     adc_res1[1000]      = {0};
volatile uint16_t     adc_res2[1000]      = {0};
volatile uint16_t RMS_volt_buf[100]      = {0};


volatile uint16_t     ArraySwitchFlag=0;
volatile uint16_t     Dac_res[10]      = {0xffff};
float input_voltage;
uint8_t con;
char messageStart[] = "**** DMAC USART echo demo ****\r\n\
**** Type a buffer of 10 characters and observe it echo back using DMA ****\r\n\
**** LED toggles each time the buffer is echoed ****\r\n";
char receiveBuffer[RX_BUFFER_SIZE] = {};
char echoBuffer[RX_BUFFER_SIZE+2] = {};

        
static volatile bool writeStatus = false;
static volatile bool readStatus = false;
static volatile bool adc_dma_done= false;

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

int main ( void )
{
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    
    NVIC_INT_Enable();
    ADC0_Enable();
    ADC1_Enable();
    SERCOM5_USART_TransmitterEnable();
    printf("Started....1\r\n");
    DMAC_ChannelCallbackRegister(DMAC_CHANNEL_0, adc_sram_dma_callback,0);
    DMAC_ChannelTransfer(DMAC_CHANNEL_0,(const void *)&ADC0_REGS->ADC_RESULT,(const void *)adc_res1,200);
    TC0_TimerStart();
    
    ADC0_ConversionStart();
    SW0_InputEnable();
    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
        
         if(TimerFlag==1)
        {
            TimerFlag=0;
            
            //ADC0_ConversionStart();
           // ADC_status=ADC0_ConversionStatusGet();
  
        }
                  
//            if(SW0_Get()==0)
//            {
//                if(con==0)
//                {   
//                    printf("Switch pressed\r\n");
//                    con=1;
//                    LED_Y_Toggle();
//                    DMAC_ChannelTransfer(DMAC_CHANNEL_0,(const void *)&ADC0_REGS->ADC_RESULT,(const void *)adc_res,20000);
//                }
//            }else
//            {
//                con=0;
//            }
        if(adc_dma_done == true)
        {
            adc_dma_done=false;
            //printf("ADC value is : \r\n");
//            for(int jj=0;jj<20000;jj++)
//            {
//                printf("%d,",adc_res[jj]);
//            }
            if(ArraySwitchFlag)
            {
                for(int jj=0;jj<100;jj++)
                {  
                    ADC_voltage=(uint16_t)(adc_res2[jj]*0.8056);
                    Avarage+=ADC_voltage*ADC_voltage;
                }
                RmsVoltage=sqrt(Avarage/100);
                AvgRmsCount=AvgRmsCount+1;
                Avarage=0;
            }else
            {
                for(int jj=0;jj<100;jj++)
                {  
                    ADC_voltage=(uint16_t)(adc_res1[jj]*0.8056);
                    Avarage+=ADC_voltage*ADC_voltage;
                }
                RmsVoltage=sqrt(Avarage/100);
                AvgRmsCount=AvgRmsCount+1;
                Avarage=0;
            }
           // RmsVoltage=sqrt(Avarage/100);
           // for(int jj=0;jj<10;jj++)
            if(AvgRmsCount<=10)
            {
                RMS_volt_buf[AvgRmsCount]=RmsVoltage;
                AvgRmsVoltage+=(uint16_t)RMS_volt_buf[RmsVoltage];
                AvgRmsVoltage = AvgRmsVoltage/AvgRmsCount;
            }
             if(AvgRmsCount>10)
            {
                 
                RMS_volt_buf[circular_pointer]=RmsVoltage;
                for(Mvg_Avg_cnt=0;Mvg_Avg_cnt<10;Mvg_Avg_cnt++)
                {
                AvgRmsVoltage+=(uint16_t)RMS_volt_buf[Mvg_Avg_cnt];
                }
                AvgRmsVoltage = AvgRmsVoltage/AvgRmsCount;
                AvgRmsCount=10;
                if(++circular_pointer > 9)
                {
                    circular_pointer=0;
                }
            }
            printf("ADCVolt=%d mV",ADC_voltage);
            printf("       ");
             //printf("vol_sqr_sum=%lu\r\n",Avarage);
            printf("RMSVolt=%lu mV",RmsVoltage);
            printf("       ");
            printf("AvgRmsVolt=%lu mV\r\n",AvgRmsVoltage);
        }
        
       /* if(readStatus == true)
        {
            
            readStatus = false;

            memcpy(echoBuffer, receiveBuffer,sizeof(receiveBuffer));
            echoBuffer[sizeof(receiveBuffer)]='\r';
            echoBuffer[sizeof(receiveBuffer)+1]='\n';


            DMAC_ChannelTransfer(DMAC_CHANNEL_0, &echoBuffer, (const void *)&SERCOM5_REGS->USART_INT.SERCOM_DATA, sizeof(echoBuffer));
            LED_Y_Toggle();
        }
        else if(writeStatus == true)
        {
            writeStatus = false;
            DMAC_ChannelTransfer(DMAC_CHANNEL_1, (const void *)&SERCOM5_REGS->USART_INT.SERCOM_DATA, &receiveBuffer, sizeof(receiveBuffer));            
        }*/
       // else
        //{
            /* Repeat the loop */
          //  ;
        //}        
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}

/*static void TransmitCompleteCallback(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle)
{
    writeStatus = true;    
}

static void ReceiveCompleteCallback(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle)
{
    readStatus = true;    
}*/
static void adc_sram_dma_callback(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle)
{
	if (event == DMAC_TRANSFER_EVENT_COMPLETE)
    {
       adc_dma_done = true;
       if(ArraySwitchFlag)
       {
           ArraySwitchFlag=0;
           DMAC_ChannelTransfer(DMAC_CHANNEL_0,(const void *)&ADC0_REGS->ADC_RESULT,(const void *)adc_res1,200);
       }else
       {
            ArraySwitchFlag=1;
            DMAC_ChannelTransfer(DMAC_CHANNEL_0,(const void *)&ADC0_REGS->ADC_RESULT,(const void *)adc_res2,200);
       }
       
      //  DAC_DataWrite (DAC_CHANNEL_0, adc_res[0]);
    }
}
/*******************************************************************************
 End of File
*/