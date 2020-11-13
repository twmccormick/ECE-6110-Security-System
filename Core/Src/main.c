// ECE 6110 - Tyler McCormick - Midterm
//
// This project creates a proximity-based security system that includes live temperature and
// proximity readings via an HTTP server, which can be accessed by connecting to
// the IP address of the STM32 board while on the same network as the board. The live
// readings work by auto-refreshing the control panel web page every 5 seconds. Auto-refreshes
// faster than this make changing the established proximity fence almost impossible. As a result,
// the live values experience a maximum 5-second view lag from the "real" world values.
//
// This project also includes an adjustable alarm that is triggered when the on-board
// proximity sensor detects any object that is within the established proximity fence.
// When triggered, the live control panel web page alerts any viewers that an alarm has been
// triggered. This alarm also blinks an on-board LED using timer 16 to trigger an interrupt,
// for additional visibility. Additionally, the only way to silence the alarm is to press the
// alarm reset button (blue button on-board). This button triggers an interrupt which resets the alarm.
//
// Lastly, the established proximity fence can be changed through the control panel by
// using a text input to enter the desired distance at which any object at or within that boundary
// will trigger the alarm.
//
// For accessing the control panel web page, supported web browsers are:
//
// Google Chrome - Version 86.0.4240.193 (Official Build) (64-bit)
//
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
#include "main.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern I2C_HandleTypeDef hI2cHandler;
VL53L0X_Dev_t Dev =
{
  .I2cHandle = &hI2cHandler,
  .I2cDevAddr = PROXIMITY_I2C_ADDRESS
};

// WIFI SSID and Password
#define SSID     "SSID_GOES_HERE"
#define PASSWORD "SSID_PASSWORD_GOES_HERE"
#define PORT           80

#define WIFI_WRITE_TIMEOUT 10000
#define WIFI_READ_TIMEOUT  10000
#define SOCKET                 0

// The entire HTML webpage is stored in this char array,
// better make it big
static  uint8_t http[16384];
static  uint8_t  IP_Addr[4];

static uint8_t  currentTemp = 0;
static uint16_t currentDist = 0;

// Experimentally-determined calibration value (in mm)
// to be subtracted from the proximity readings
const uint16_t calibrationValue = 50;
static int alarmDist = 70;
static bool alarm = false;

// Handles for the UART and Timer 16
UART_HandleTypeDef huart1;
TIM_HandleTypeDef htim16;

void SystemClock_Config(void);

// Nice function for transmitting over serial
void serialPrint(char buffer[]);

// Updates the sensor values
void checkSensors();

static void MX_GPIO_Init(void);
static void MX_TIM16_Init(void);
static void MX_USART1_UART_Init(void);


// TOF pin configuration
static void VL53L0X_PROXIMITY_MspInit(void);

// Function declarations for VL53L0X init and range measurements
static void VL53L0X_PROXIMITY_Init(void);
static uint16_t VL53L0X_PROXIMITY_GetDistance(void);

// All wifi-related functions
static  WIFI_Status_t SendWebPage( uint8_t temperature, uint16_t proxData);
static  int wifi_server(void);
static  int wifi_start(void);
static  int wifi_connect(void);
static  bool WebServerProcess(void);

int main(void)
{

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  // Initialize the UART for serial communication 
  MX_USART1_UART_Init();

  // Initialize the ToF sensor
  VL53L0X_PROXIMITY_Init();

  // Use the BSP library to initialize LED2
  BSP_LED_Init(LED2);
  
  // Use the BSP library to initialize the temperature sensor
  BSP_TSENSOR_Init();

  // Initialize timer 16 (used for blinking the LED)
  // Currently set to blink once per 2s
  MX_TIM16_Init();
  
  // Starts the timer
  HAL_TIM_Base_Start(&htim16);
  
  // Enables the timer to trigger interrupts 
  __HAL_TIM_ENABLE_IT(&htim16, TIM_IT_UPDATE );
  
  // Just print a nice banner for starting the WIFI server 
  serialPrint("****** WIFI Web Server Start****** \n\r");

  wifi_server();

}

/**
* @brief System Clock Configuration
* @retval None
*/

static int wifi_start(void)
{
uint8_t  MAC_Addr[6];

/*Initialize and use WIFI module */
if(WIFI_Init() ==  WIFI_STATUS_OK)
{
  serialPrint("ES-WIFI Initialized.\n\r");
  if(WIFI_GetMAC_Address(MAC_Addr) == WIFI_STATUS_OK)
  {

    char macAdd[60] = {0};
    sprintf(macAdd,"> es-wifi module MAC Address : %X:%X:%X:%X:%X:%X\n\r",
            MAC_Addr[0],
            MAC_Addr[1],
            MAC_Addr[2],
            MAC_Addr[3],
            MAC_Addr[4],
            MAC_Addr[5]);

    serialPrint(macAdd);

  }
  else
  {
    serialPrint("> ERROR : CANNOT get MAC address\n\r");
    return -1;
  }
}
else
{
  return -1;
}
return 0;
}


int wifi_connect(void)
{

wifi_start();

char connecting[100] = {0};
sprintf(connecting,"\nConnecting to %s , %s\n\r",SSID,PASSWORD);


serialPrint(connecting);
if( WIFI_Connect(SSID, PASSWORD, WIFI_ECN_WPA2_PSK) == WIFI_STATUS_OK)
{
  if(WIFI_GetIP_Address(IP_Addr) == WIFI_STATUS_OK)
  {

    char connectMes[100] = {0};
    sprintf(connectMes,"> es-wifi module connected: got IP Address : %d.%d.%d.%d\n\r",
            IP_Addr[0],
            IP_Addr[1],
            IP_Addr[2],
            IP_Addr[3]);

    serialPrint(connectMes);

  }
  else
  {
		  serialPrint(" ERROR : es-wifi module CANNOT get IP address\n\r");
    return -1;
  }
}
else
{
		 serialPrint("ERROR : es-wifi module NOT connected\n\r");
   return -1;
}
return 0;
}

int wifi_server(void)
{
bool StopServer = false;

serialPrint("\nRunning HTML Server test\n\r");
if (wifi_connect()!=0) return -1;


if (WIFI_STATUS_OK!=WIFI_StartServer(SOCKET, WIFI_TCP_PROTOCOL, 1, "", PORT))
{
  serialPrint("ERROR: Cannot start server.\n\r");
}

char logMes[100] = {0};
sprintf(logMes,"Server is running and waiting for an HTTP  Client connection to %d.%d.%d.%d\n\r",IP_Addr[0],IP_Addr[1],IP_Addr[2],IP_Addr[3]);
serialPrint(logMes);

do
{
  uint8_t RemoteIP[4];
  uint16_t RemotePort;


  while (WIFI_STATUS_OK != WIFI_WaitServerConnection(SOCKET,1000,RemoteIP,&RemotePort))
  {

  	char waitMes[100] = {0};
  	sprintf(waitMes,"Waiting for a connection to  %d.%d.%d.%d\n\r",IP_Addr[0],IP_Addr[1],IP_Addr[2],IP_Addr[3]);
  	serialPrint(waitMes);
  	checkSensors();

  }

  char conMes[100] = {0};
  sprintf(conMes,"Client connected %d.%d.%d.%d:%d\n\r",RemoteIP[0],RemoteIP[1],RemoteIP[2],RemoteIP[3],RemotePort);
  serialPrint(conMes);

  StopServer=WebServerProcess();

  if(WIFI_CloseServerConnection(SOCKET) != WIFI_STATUS_OK)
  {
    serialPrint("ERROR: failed to close current Server connection\n\r");
    return -1;
  }
}
while(StopServer == false);

if (WIFI_STATUS_OK!=WIFI_StopServer(SOCKET))
{
  serialPrint("ERROR: Cannot stop server.\n\r");
}

serialPrint("Server is stop\n\r");
return 0;
}

static bool WebServerProcess(void)
{

uint16_t  respLen;

static   uint8_t resp[1024];
bool    stopserver=false;

if (WIFI_STATUS_OK == WIFI_ReceiveData(SOCKET, resp, 1000, &respLen, WIFI_READ_TIMEOUT))
{

 char getByte[40] = {0};
 sprintf(getByte, "get %d byte(s) from server\n\r",respLen);


 if( respLen > 0)
 {
    if(strstr((char *)resp, "GET")) /* GET: put web page */
    {

      if(SendWebPage( currentTemp, currentDist) != WIFI_STATUS_OK)
      {
        serialPrint("> ERROR : Cannot send web page\n\r");
      }
      else
      {
        serialPrint("Send page after  GET command\n\r");
      }
     }
     else if(strstr((char *)resp, "POST"))/* POST: received info */
     {
       serialPrint("Post request\n\r");

       int numEqs = 0;

       char fenceNum[3] = "000";

       // Everything inside this for loop is dedicated to
       // the traversal of the response from the end user
       // in order to find the desired fence number input
       // (The new proximity fence value)
       //
       // As the response is a char array of length 1024, the
       // for loop iterates a maximum of 1024 times to find what
       // we are looking for
       //
       // As it turns out, traversing a huge char array is challenging,
       // especially since the input could be a 2-digit or a 3-digit number
       // So, instead of looking for the number, we just look for the 7th '='.
       //
       // After studying the response after clicking the 'submit' button,
       // the desired input was alwas after the 7th '=' in the whole response.
       // So, it logically follows that the next 2 (or 3) characters in the
       // array following the 7th '=' will be the number we are looking for.
       //
       // After finding this number, some extra stuff is done in order to account for
       // 2 digit fence numbers as well as 3 digit ones. This was somewhat challenging,
       // as the webserver doesn't overwrite the entire response, just what gets sent each time,
       // so if a 2 digit fence value followed a 3 digit fence value, the loop would
       // slurp in the next digit, and the fence value would be corrupted
       //
       // To fix this, if the 3rd digit is a number, the loop stores it and then replaces that value with an
       // asterisk. If a 2 digit input follows a 3 digit input, the asterisk will not be overwritten,
       // and so if this happens, we know for sure that a 2-digit input was entered, and the loop accounts for this.

      for (int i=0; i<1024; i++) {

    	  // Searching and counting the '='
    	  if (resp[i] == '=') {

    		  numEqs++;

    	  }

    	  // If the 7th '=' was found, the next few entries are the new fence input
    	  if (numEqs == 7) {

    		  // Check the 3rd digit. If it's a number, then a 3-digit input has been
    		  // entered
    		  //
    		  // AS mentioned above, always write an asterisk after, if another
    		  // 3-digit input is entered this will get overwritten, and if a
    		  // 2-digit number gets entered, this if statement will not get executed,
    		  // as the condition simply checks that the 3rd digit is actually a number
    		  // (remember since the response is in char form, and so we check for numbers
    		  // based on their ASCII representation
    		  if ((resp[i+3] >= 48) && (resp[i+3] <= 57)) {

    			  // Store the 3rd digit, it's important
    		      fenceNum[2] = resp[i+3];

    		      resp[i+3] = '*';

    		      // Check if the first digit is a number and
    		      // store it if it is.
    		      if ((resp[i+1] >= 48) && (resp[i+1]<= 57)) {

    		          fenceNum[0] = resp[i+1];

    		      }

    		      // Same for the 2nd digit
    		      if ((resp[i+2] >= 48) && (resp[i+2]<= 57)) {

    		          fenceNum[1] = resp[i+2];

    		      }

    		  }

    		  else {

    			  // If the above condition fails, the input is clearly  a 2-digit number,
    			  // and so we adjust the placement of the digits to include a leading 0,
    			  // which won't affect the conversion we will run later
    		      fenceNum[0] = '0';
    		      fenceNum[1] = resp[i+1];
    		      fenceNum[2] = resp[i+2];

    		      break;

    		  }

    		  // Break out of the loop, we're done!
    		  break;

    	  }

      }

      // Sets the new alarm distance by converting a string to a number
      alarmDist = atoi(fenceNum);

       if(strstr((char *)resp, "stop_server"))
       {
         if(strstr((char *)resp, "stop_server=0"))
         {
           stopserver = false;
         }
         else if(strstr((char *)resp, "stop_server=1"))
         {
           stopserver = true;
         }
       }

       if(SendWebPage( currentTemp, currentDist) != WIFI_STATUS_OK)
       {
         serialPrint("> ERROR : Cannot send web page\n\r");
       }
       else
       {
         serialPrint("Send Page after POST command\n\r");
       }
     }
   }
}
else
{
  serialPrint("Client close connection\n\r");
}
return stopserver;

}

static WIFI_Status_t SendWebPage( uint8_t temperature, uint16_t proxData)
{
uint8_t  temp[50];
char proxMes[50] = {0};
char fenceMes[50] = {0};

uint16_t SentDataLength;
WIFI_Status_t ret;

// construct web page content
strcpy((char *)http, (char *)"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n");
strcat((char *)http, (char *)"<html>\r\n<body>\r\n");

// Set up automatic refresh. This is so the newest values for temperature and
// proximity are (at least somewhat) recent
strcat((char *)http, (char *)"<meta http-equiv=\"refresh\" content=\"5\">");

// Center the text for heading 2 (h2) and 3 (h3)
strcat((char *)http, (char *)"<style>");
strcat((char *)http, (char *)"h2 {text-align: center;}");
strcat((char *)http, (char *)"h3 {text-align: center;}");
strcat((char *)http, (char *)"</style>");

// The title of the webpage and a nice strong header
strcat((char *)http, (char *)"<title>Proximity Security System</title>\r\n");
strcat((char *)http, (char *)"<h2>STM32 Proximity Security System Control Panel</h2>\r\n");

// Checks the alarm has been triggered and if it has, adds a scary-looking warning
// that alerts the user about the triggered alarm
//
// Also changes the background to a scary-looking red
//
// If this check fails, set the  background to a professional-gray
if (alarm == true) {

	strcat((char *)http, (char *)"<h3>WARNING!!! ALARM HAS BEEN TRIGGERED!</h3>\r\n");
	strcat((char *)http, (char *)"<body style=\"background-color:red;\">");
}

else {

	strcat((char *)http, (char *)"<body style=\"background-color:grey;\">");

}


strcat((char *)http, (char *)"<br /><hr>\r\n");

// Nice header for the live readings
strcat((char *)http, (char *)"<p style=\"text-decoration: underline;\"><strong>Live Readings</strong></p>");

/*
if (alarm == true) {

	strcat((char *)http, (char *)"<body style=\"background-color:red;\">");
}

else {

	strcat((char *)http, (char *)"<body style=\"background-color:grey;\">");

}

*/

// Printing the temperature, use an input form for the text display as it ensures a white background for the
// readings, which won't be affected by background color changes
strcat((char *)http, (char *)"<p><form method=\"POST\"><strong>Current Temperature: <input type=\"text\" value=\"");
sprintf((char *)temp, "%d", temperature);
strcat((char *)http, (char *)temp);
strcat((char *)http, (char *)"\"> <sup>O</sup>F");

// Object too far to measure accurately, say that there is no object
if (proxData > 2000) {

	  strcat((char *)http, (char *)"<p><form method=\"POST\"><strong>Object Distance: <input type=\"text\" value=\"No Object Detected!\"");

}

else {

	  // Otherwise, display the current proximity data
	  strcat((char *)http, (char *)"<p><form method=\"POST\"><strong>Object Distance: <input type=\"text\" value=\"");
	  sprintf((char *)proxMes, "%d", proxData);
	  strcat((char *)http, (char *)proxMes);
	  strcat((char *)http, (char *)"\"> mm");

}

// Nice header for the security settings
strcat((char *)http, (char *)"<p> </p>");
strcat((char *)http, (char *)"<p style=\"text-decoration: underline;\"><strong>Security Settings</strong></p>");
strcat((char *)http, (char *)"<p> </p>");

// Input form for the new proximity distance
strcat((char *)http, (char *)"<p><form method=\"POST\"><strong>Current Proximity Fence: <input type=\"text\" value=\"");
sprintf((char *)fenceMes, "%d", alarmDist);
strcat((char *)http, (char *)fenceMes);
strcat((char *)http, (char *)"\"> mm");

// Just a nice separator
strcat((char *)http, (char *)"<p> </p>");

// Input for the new fence number
strcat((char *)http, (char *)"<label for=\"fenceNum\"><strong>New Proximity Fence: </strong></label>");
strcat((char *)http, (char *)"<input type=\"text\" id=\"fenceNum\" name=\"fenceNum\"><br><br>");

// Submit button
strcat((char *)http, (char *)"</strong><p><input type=\"submit\"></form></span>");

// End the page
strcat((char *)http, (char *)"</body>\r\n</html>\r\n");

// Send the page off
ret = WIFI_SendData(0, (uint8_t *)http, strlen((char *)http), &SentDataLength, WIFI_WRITE_TIMEOUT);

if((ret == WIFI_STATUS_OK) && (SentDataLength != strlen((char *)http)))
{
  ret = WIFI_STATUS_ERROR;
}

return ret;
}

static void VL53L0X_PROXIMITY_Init(void)
{

// Initialize I2C interface
SENSOR_IO_Init();

	// Initialize pins for TOF
VL53L0X_PROXIMITY_MspInit();

// One-time TOF device initialization
VL53L0X_DataInit(&Dev);

}


static uint16_t VL53L0X_PROXIMITY_GetDistance(void)
{
VL53L0X_RangingMeasurementData_t RangingMeasurementData;

VL53L0X_PerformSingleRangingMeasurement(&Dev, &RangingMeasurementData);

return RangingMeasurementData.RangeMilliMeter;
}

/**
* @brief  VL53L0X proximity sensor Msp Initialization.
*/
static void VL53L0X_PROXIMITY_MspInit(void)
{
GPIO_InitTypeDef GPIO_InitStruct;

// Configure GPIO pin : VL53L0X_XSHUT_Pin
GPIO_InitStruct.Pin = VL53L0X_XSHUT_Pin;
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Pull = GPIO_PULLUP;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
HAL_GPIO_Init(VL53L0X_XSHUT_GPIO_Port, &GPIO_InitStruct);

HAL_GPIO_WritePin(VL53L0X_XSHUT_GPIO_Port, VL53L0X_XSHUT_Pin, GPIO_PIN_SET);

HAL_Delay(1000);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
switch (GPIO_Pin)
{
  case (GPIO_PIN_1):
  {
    SPI_WIFI_ISR();
    break;
  }

  // This triggers if the blue-button
  // interrupt is triggered. This is the only
  // way to reset the alarm
  case (GPIO_PIN_13):
  	{

      	alarm = false;

  	}
  default:
  {
    break;
  }
}
}

/**
* @brief  SPI3 line detection callback.
* @param  None
* @retval None
*/
void SPI3_IRQHandler(void)
{
HAL_SPI_IRQHandler(&hspi);
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM16 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM16_Init(void)
{

  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 7999;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 9999;
  htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim16.Init.RepetitionCounter = 0;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, M24SR64_Y_RF_DISABLE_Pin|M24SR64_Y_GPO_Pin|ISM43362_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, ARD_D10_Pin|SPBTLE_RF_RST_Pin|ARD_D9_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, ARD_D8_Pin|ISM43362_BOOT0_Pin|ISM43362_WAKEUP_Pin|LED2_Pin
                          |SPSGRF_915_SDN_Pin|ARD_D5_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, USB_OTG_FS_PWR_EN_Pin|PMOD_RESET_Pin|STSAFE_A100_RESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPBTLE_RF_SPI3_CSN_GPIO_Port, SPBTLE_RF_SPI3_CSN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, VL53L0X_XSHUT_Pin|LED3_WIFI__LED4_BLE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPSGRF_915_SPI3_CSN_GPIO_Port, SPSGRF_915_SPI3_CSN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ISM43362_SPI3_CSN_GPIO_Port, ISM43362_SPI3_CSN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : M24SR64_Y_RF_DISABLE_Pin M24SR64_Y_GPO_Pin ISM43362_RST_Pin ISM43362_SPI3_CSN_Pin */
  GPIO_InitStruct.Pin = M24SR64_Y_RF_DISABLE_Pin|M24SR64_Y_GPO_Pin|ISM43362_RST_Pin|ISM43362_SPI3_CSN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : USB_OTG_FS_OVRCR_EXTI3_Pin SPSGRF_915_GPIO3_EXTI5_Pin SPBTLE_RF_IRQ_EXTI6_Pin ISM43362_DRDY_EXTI1_Pin */
  GPIO_InitStruct.Pin = USB_OTG_FS_OVRCR_EXTI3_Pin|SPSGRF_915_GPIO3_EXTI5_Pin|SPBTLE_RF_IRQ_EXTI6_Pin|ISM43362_DRDY_EXTI1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : BUTTON_EXTI13_Pin */
  GPIO_InitStruct.Pin = BUTTON_EXTI13_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BUTTON_EXTI13_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ARD_A5_Pin ARD_A4_Pin ARD_A3_Pin ARD_A2_Pin
                           ARD_A1_Pin ARD_A0_Pin */
  GPIO_InitStruct.Pin = ARD_A5_Pin|ARD_A4_Pin|ARD_A3_Pin|ARD_A2_Pin
                          |ARD_A1_Pin|ARD_A0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : ARD_D1_Pin ARD_D0_Pin */
  GPIO_InitStruct.Pin = ARD_D1_Pin|ARD_D0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : ARD_D10_Pin SPBTLE_RF_RST_Pin ARD_D9_Pin */
  GPIO_InitStruct.Pin = ARD_D10_Pin|SPBTLE_RF_RST_Pin|ARD_D9_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ARD_D4_Pin */
  GPIO_InitStruct.Pin = ARD_D4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(ARD_D4_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ARD_D7_Pin */
  GPIO_InitStruct.Pin = ARD_D7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ARD_D7_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ARD_D13_Pin ARD_D12_Pin ARD_D11_Pin */
  GPIO_InitStruct.Pin = ARD_D13_Pin|ARD_D12_Pin|ARD_D11_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ARD_D3_Pin */
  GPIO_InitStruct.Pin = ARD_D3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ARD_D3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ARD_D6_Pin */
  GPIO_InitStruct.Pin = ARD_D6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ARD_D6_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ARD_D8_Pin ISM43362_BOOT0_Pin ISM43362_WAKEUP_Pin LED2_Pin
                           SPSGRF_915_SDN_Pin ARD_D5_Pin SPSGRF_915_SPI3_CSN_Pin */
  GPIO_InitStruct.Pin = ARD_D8_Pin|ISM43362_BOOT0_Pin|ISM43362_WAKEUP_Pin|LED2_Pin
                          |SPSGRF_915_SDN_Pin|ARD_D5_Pin|SPSGRF_915_SPI3_CSN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : DFSDM1_DATIN2_Pin DFSDM1_CKOUT_Pin */
  GPIO_InitStruct.Pin = DFSDM1_DATIN2_Pin|DFSDM1_CKOUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF6_DFSDM1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : QUADSPI_CLK_Pin QUADSPI_NCS_Pin OQUADSPI_BK1_IO0_Pin QUADSPI_BK1_IO1_Pin
                           QUAD_SPI_BK1_IO2_Pin QUAD_SPI_BK1_IO3_Pin */
  GPIO_InitStruct.Pin = QUADSPI_CLK_Pin|QUADSPI_NCS_Pin|OQUADSPI_BK1_IO0_Pin|QUADSPI_BK1_IO1_Pin
                          |QUAD_SPI_BK1_IO2_Pin|QUAD_SPI_BK1_IO3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : INTERNAL_I2C2_SCL_Pin INTERNAL_I2C2_SDA_Pin */
  GPIO_InitStruct.Pin = INTERNAL_I2C2_SCL_Pin|INTERNAL_I2C2_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : INTERNAL_UART3_TX_Pin INTERNAL_UART3_RX_Pin */
  GPIO_InitStruct.Pin = INTERNAL_UART3_TX_Pin|INTERNAL_UART3_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : LPS22HB_INT_DRDY_EXTI0_Pin LSM6DSL_INT1_EXTI11_Pin ARD_D2_Pin HTS221_DRDY_EXTI15_Pin
                           PMOD_IRQ_EXTI12_Pin */
  GPIO_InitStruct.Pin = LPS22HB_INT_DRDY_EXTI0_Pin|LSM6DSL_INT1_EXTI11_Pin|ARD_D2_Pin|HTS221_DRDY_EXTI15_Pin
                          |PMOD_IRQ_EXTI12_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : USB_OTG_FS_PWR_EN_Pin SPBTLE_RF_SPI3_CSN_Pin PMOD_RESET_Pin STSAFE_A100_RESET_Pin */
  GPIO_InitStruct.Pin = USB_OTG_FS_PWR_EN_Pin|SPBTLE_RF_SPI3_CSN_Pin|PMOD_RESET_Pin|STSAFE_A100_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : VL53L0X_XSHUT_Pin LED3_WIFI__LED4_BLE_Pin */
  GPIO_InitStruct.Pin = VL53L0X_XSHUT_Pin|LED3_WIFI__LED4_BLE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : VL53L0X_GPIO1_EXTI7_Pin LSM3MDL_DRDY_EXTI8_Pin */
  GPIO_InitStruct.Pin = VL53L0X_GPIO1_EXTI7_Pin|LSM3MDL_DRDY_EXTI8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OTG_FS_VBUS_Pin */
  GPIO_InitStruct.Pin = USB_OTG_FS_VBUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OTG_FS_VBUS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : USB_OTG_FS_ID_Pin USB_OTG_FS_DM_Pin USB_OTG_FS_DP_Pin */
  GPIO_InitStruct.Pin = USB_OTG_FS_ID_Pin|USB_OTG_FS_DM_Pin|USB_OTG_FS_DP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : INTERNAL_SPI3_SCK_Pin INTERNAL_SPI3_MISO_Pin INTERNAL_SPI3_MOSI_Pin */
  GPIO_InitStruct.Pin = INTERNAL_SPI3_SCK_Pin|INTERNAL_SPI3_MISO_Pin|INTERNAL_SPI3_MOSI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PMOD_SPI2_SCK_Pin */
  GPIO_InitStruct.Pin = PMOD_SPI2_SCK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(PMOD_SPI2_SCK_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PMOD_UART2_CTS_Pin PMOD_UART2_RTS_Pin PMOD_UART2_TX_Pin PMOD_UART2_RX_Pin */
  GPIO_InitStruct.Pin = PMOD_UART2_CTS_Pin|PMOD_UART2_RTS_Pin|PMOD_UART2_TX_Pin|PMOD_UART2_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : ARD_D15_Pin ARD_D14_Pin */
  GPIO_InitStruct.Pin = ARD_D15_Pin|ARD_D14_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */

// This whole functin is the interrupt handler for
// the alarm system, this only chekcs the alarm and toggle the LED is
// alarm is true and turns it off when not true. This is becasue
// there is no way to garuntee that the last state of the LED is off
//
// Right now the LED blink is set to once per 2 seconds
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  // Check to make sure the correct timer called this interrupt
  if (htim == &htim16 )
  {

	  //
	  if (alarm == true) {

		  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);

	  }

	  else {

		  BSP_LED_Off(LED2);

	  }

  }

}

// Made a function for printing over the USART, as I was tired of copy-pasting
// the obscenely long HAL transmit function
void serialPrint(char buffer[]) {

	HAL_UART_Transmit(&huart1, (uint8_t*)buffer , strlen(buffer), HAL_MAX_DELAY);

}

// This function reads values from the temperature sensor and the
// proximity sensor, converting and calibrating the values as necessary
void checkSensors() {

	// Read and then convert the temperature to farenheit
	currentTemp = BSP_TSENSOR_ReadTemp();
	currentTemp = (currentTemp*1.8)+32;


	// Calibrate the ToF sensor with an experimentally-determined value
	currentDist = VL53L0X_PROXIMITY_GetDistance();
	currentDist = currentDist-calibrationValue;

	// Also checks whether the new proximity value violates the established proximity fence
	//
	// If it does, activate the alarm
	if (currentDist <= alarmDist) {

		alarm = true;

	}

	else {

	}

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
