/* USER CODE BEGIN Header */
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
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "dict.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
// General Helper
struct node;
struct list;
void append(struct list *, char *);
int searchAndRemove(struct list *, char *);
struct list * listInit();
void emptiedList(struct list *);

char InputHandler();

void ChangeTimerPeriod();

char * getRandWord();
void setRandSeed();

char * stringGetLastKBit(char *, int);
// Terminal Controls
void Refresh();
void ClearLine();
void MoveCurTo(int, int);
void SetColorBrightRedAll();
void SetColorResetWithBold();
void SetColorBrightRedBG_BlackLetter();
void SetColorBrightCyanAll();
// UI printing
void print(const void *);
void PrintCentered(const void *, int, int);

void WelcomePagePrompt();

void StartPlayInit();
void PlayingPageInfo();
void WinningCat();

void DeadPageInit();

void SettingPageInit();

void SettingDifficultyInit();

void SettingSpeedInit();

void DrawGrid();
void SetFontBold();
// UX / Game flows Controls
void WelcomePageHandler();

void PlayingPageHandler();
void PlayingPageTimerCalback();

void DeadPageHandler();

void SettingPageHandler();

void SettingDifficultyHandler();

void SettingSpeedHandler();

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
struct node{
	char * data;
	struct node * next;
	struct node * back;
};

struct list{
	int stringSize;
	int size;
	struct node * head;
	struct node * tail;
};

char txBuffer[100];
char txQueue[100][100];
int currentTxQueueIndex = 0;
int endTxQueueIndex = 0;

typedef struct GameDataStruct {
	struct list * lines[10];
	int linesLength[10];
	int speed; /* 1 - 10 default 1 */
	int numberOfLines; /* 1 - 10 default 4 */
	int highScore; /* initial 0 */
	int gameState;
	/*
	 * 0: Welcome Page
	 * 1: Playing Page
	 * 11: Dead Page
	 * 2: Setting Page
	 * 21: Set Difficulty Page
	 * 22: Set Speed Page
	 */
	int currentScore; /* initial 0 */
	char currentScoreString[60];
} GameData;
GameData gameData;

char receive[1] = "-";
char line[100] = "";
char finishedLine[100] = "";

const char newLine[] = "\r\n";
const char clear[] = "\033[2J";
const char moveCurTopLeft[] = "\033[H";
const char clearAndMoveCurTopLeft[] = "\033[2J \033[H";
const char reset[] = "\033c";
const char debug[] = "NOT OK\r\n";
const char moveCurLeft[] = "\033[D";
const char clearLine[] = "\033[2K";

//const char moveCurLeftWelcome[] = "\033[4;0H";
const char inputPrompt[] = "\033[37;101mPu\033[30;42mRo\033[30;103mMu\033[37;104mTo\033[37;40m: ";

void WinningCat() {
	MoveCurTo(gameData.numberOfLines + 7, 34); print("　　　　∧,,∧");
	MoveCurTo(gameData.numberOfLines + 8, 34); print(" ☆二　⊂(・ω・｀)");
	MoveCurTo(gameData.numberOfLines + 9, 34); print("　　　　-ヽ　　と)");
	MoveCurTo(gameData.numberOfLines + 10, 34); print(" 　　　　　｀ｕ-ｕ'");
}

void init() {
	Refresh();
	SetFontBold();
	WelcomePagePrompt();
	gameData.gameState = 0;
	gameData.speed = 1;
	gameData.numberOfLines = 4;
	gameData.highScore = 0;
	gameData.currentScore = 0;
	for(int i = 0; i < 10; i++) {
		gameData.linesLength[i] = 0;
		gameData.lines[i] = listInit();
	}
}

void driver() {
	switch(gameData.gameState){
	case 0:
		WelcomePageHandler();
		break;
	case 1:
		PlayingPageHandler();
		break;
	case 11:
		DeadPageHandler();
		break;
	case 2:
		SettingPageHandler();
		break;
	case 21:
		SettingDifficultyHandler();
		break;
	case 22:
		SettingSpeedHandler();
		break;
	default:
		WelcomePageHandler();
		break;
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	InputHandler();
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
//	return;
	currentTxQueueIndex++;
	if(currentTxQueueIndex == 100)
		currentTxQueueIndex -= 100;

	if(currentTxQueueIndex == endTxQueueIndex) return;
	strcpy(txBuffer, txQueue[currentTxQueueIndex]);
	HAL_UART_Transmit_IT(&huart2, (unsigned char *)txBuffer, strlen(txBuffer));
}

//void HAL_TIM_PeriodElaspedCallback(TIM_HandleTypeDef *htim) {
//}

void print(const void * s) {
	s = (char * ) s;
//	HAL_UART_Transmit(&huart2, (unsigned char *) s, strlen(s), HAL_MAX_DELAY);
//	return;
	if(strlen(s) == 0) return;
	strcpy(txQueue[endTxQueueIndex], s);
	endTxQueueIndex++;
	if(endTxQueueIndex == 100)
		endTxQueueIndex -= 100;

	if(endTxQueueIndex - 1 == currentTxQueueIndex || endTxQueueIndex + 99 == currentTxQueueIndex) {
		strcpy(txBuffer, txQueue[currentTxQueueIndex]);
		HAL_UART_Transmit_IT(&huart2, (unsigned char *) txBuffer, strlen(txBuffer));
	}
}

void append(struct list * lisPtr, char * newData) {
	struct node * np = malloc(sizeof(struct node));
	np->data = newData;
	np->back = lisPtr->tail;
	np->next = lisPtr->head;
	lisPtr->head->back = np;
	lisPtr->tail->next = np;
	lisPtr->tail = np;
	lisPtr->size++;
	lisPtr->stringSize += strlen(np->data);
}

int searchAndRemove(struct list * lisPtr, char * s) {
	struct node * np = lisPtr->head->next;
	while(np != lisPtr->head) {
		if(strcmp(np->data, s) == 0){
			np->next->back = np->back;
			np->back->next = np->next;
			lisPtr->size--;
			lisPtr->stringSize -= strlen(np->data);
			free(np);
			gameData.currentScore += strlen(s);
			return 1;
		}
		np = np->next;
	}
	return 0;
}

struct list * listInit() {
	struct node * headerNode = malloc(sizeof(struct node));
	headerNode->data = "";
	headerNode->back = headerNode;
	headerNode->next = headerNode;
	struct list * listPtr = malloc(sizeof(struct list));
	listPtr->stringSize = 0;
	listPtr->size = 0;
	listPtr->head = headerNode;
	listPtr->tail = headerNode;
	return listPtr;
}

void emptiedList(struct list * lisPtr) {
	struct node * tempNode = lisPtr->head->next;
	while(tempNode != lisPtr->head){
		struct node * delNode = tempNode;
		tempNode = tempNode->next;
		free(delNode);
	}
	lisPtr->head->next = lisPtr->head;
	lisPtr->head->back = lisPtr->head;
	lisPtr->tail = lisPtr->head;
	lisPtr->stringSize = 0;
	lisPtr->size = 0;
}

char * getRandWord() {
	return DICT[rand() % 9474];
}

void setRandSeed() {
	srand(SysTick->VAL);
}

char InputHandler() {
	if(receive[0] == '\r') { // Enter
		strcpy(finishedLine, line);
		strcpy(line, "");
		return receive[0];
	}
	if(receive[0] == '\e') { // Prolly Escape
		return receive[0];
	}
	if(receive[0] == '[' || receive[0] == 'A' || receive[0] == 'B' || receive[0] == 'C' || receive[0] == 'D') { // ALL Arrow BANNED / I Gave up.
		return receive[0];
	}
	if(receive[0] == '\177') { //Backspace
		if(strlen(line) == 0) return receive[0];
		print(receive);
		line[strlen(line) - 1] = '\0';
		return receive[0];
	}
	// Default
	strncat(line, receive, 1);
	print(receive);
	if(strlen(line) > 97) {
		strcpy(finishedLine, line);
		strcpy(line, "");
	}
	return receive[0];
}

void WelcomePagePrompt() {
	DrawGrid();
	PrintCentered("\033[48;5;248m\033[38;5;93mWel\033[38;5;17mcome \033[38;5;14mto \033[38;5;10mText \033[38;5;11mSur", 3, 25);
	print("\033[38;5;214mvi\033[38;5;196mval!");
	SetColorResetWithBold();
	MoveCurTo(4, 5); print("1) Play");
	MoveCurTo(5, 5); print("2) Setting");
	MoveCurTo(6, 5); print(inputPrompt); print(line);
}

void WelcomePageHandler() {
	HAL_UART_Receive_IT(&huart2, (unsigned char *)receive, 1);
	if(receive[0] != '\r') return;
	receive[0] = 'X';
	if(strcmp(finishedLine, "1") == 0) {
		StartPlayInit();
		gameData.gameState = 1;
		return;
	}
	if(strcmp(finishedLine, "2") == 0) {
		SettingPageInit();
		gameData.gameState = 2;
		return;
	}
	ClearLine();
	DrawGrid();
	MoveCurTo(6, 5); print(inputPrompt);
}

void PlayingPageInfo() {
	Refresh();

	SetColorBrightRedBG_BlackLetter();
	for(int i = 0; i < gameData.numberOfLines; i++) {
		MoveCurTo(i + 5, 77);
		print("XX");
	}
	SetColorResetWithBold();

	sprintf(gameData.currentScoreString, "%d", gameData.currentScore);
	PrintCentered("Carnibaru Score: ", 3, 20); // score on 3 digit average (actual len = 17)
	print(gameData.currentScoreString);

	for(int i = 0; i < gameData.numberOfLines; i++) {
		char tempString[100] = "";
		struct node * tempNode = gameData.lines[i]->tail;
		while(tempNode != gameData.lines[i]->head) {
			strcat(tempString, tempNode->data);
			strcat(tempString, " ");
			tempNode = tempNode->back;
		}
		char * outString = stringGetLastKBit(tempString, gameData.linesLength[i]);
		MoveCurTo(5 + i, 3); print(">"); print(outString);
	}
	DrawGrid();
	MoveCurTo(5 + gameData.numberOfLines, 5);
}

void PlayingPageHandler() {
	HAL_UART_Receive_IT(&huart2, (unsigned char *)receive, 1);
	if(receive[0] != '\r') return;
	receive[0] = 'X';
	int checkIfRemoved = 0;
	for(int i = 0; i < gameData.numberOfLines; i++) {
		checkIfRemoved = searchAndRemove(gameData.lines[i], finishedLine);
		if(checkIfRemoved == 1){
			gameData.linesLength[i] -= strlen(finishedLine);
			if(gameData.linesLength[i] < 0) gameData.linesLength[i] = 0;
		}
	}

	PlayingPageInfo();
	print(inputPrompt); print(line);

}

void PlayingPageTimerCallback() {
	if(gameData.gameState != 1) return;
	for(int i = 0; i < gameData.numberOfLines; i++) {
		gameData.linesLength[i] += 1;
		if(gameData.linesLength[i] > 74){
			gameData.gameState = 11; // DEAD
			DeadPageInit();
			return;
		}
		if(gameData.linesLength[i] > gameData.lines[i]->stringSize){
			append(gameData.lines[i], getRandWord());
		}
	}
	PlayingPageInfo();
	print(inputPrompt); print(line);
}

void StartPlayInit() {
	setRandSeed();
	gameData.currentScore = 0;
	for(int i = 0; i < gameData.numberOfLines; i++) {
		emptiedList(gameData.lines[i]);
		gameData.linesLength[i] = 0;
	}
}

void DeadPageInit() {
	int flag = 0;
	print(clearLine);
	PrintCentered("You Are Dead (ó﹏ò｡) \r\n\n", gameData.numberOfLines + 6, 19);

	if(gameData.currentScore > gameData.highScore) {
		gameData.highScore = gameData.currentScore;
		print("\tYou got a new HighScore!!\r\n");
		flag = 1;
	} else {
		char currentHighScore[40];
		sprintf(currentHighScore, "\tYour Current HighScore: %d\r\n", gameData.highScore);
		print(currentHighScore);
	}
	char deadPrompt2[60];
	sprintf(deadPrompt2, "\tYour Score: %d\r\n\tEnter to go back to menu\r\n", gameData.currentScore);
	print(deadPrompt2);
	if(flag == 1){
		WinningCat();
	}
	DrawGrid();
	MoveCurTo(gameData.numberOfLines + 11, 3);
}

void DeadPageHandler() {
	HAL_UART_Receive_IT(&huart2, (unsigned char *)receive, 1);
	if(receive[0] != '\r') return;
	receive[0] = 'X';
	gameData.gameState = 0;
	Refresh();
	WelcomePagePrompt();
}

void SettingPageInit() {
	Refresh();
	DrawGrid();
	char settingDiffPrompt[30], settingSpeedPrompt[30];
	PrintCentered("Setting Page", 3, -1);
	sprintf(settingDiffPrompt, "1) Difficulty Setting (%d)", gameData.numberOfLines);
	sprintf(settingSpeedPrompt, "2) Speed Setting (%d)", gameData.speed);
	MoveCurTo(4, 5); print(settingDiffPrompt);
	MoveCurTo(5, 5); print(settingSpeedPrompt);
	MoveCurTo(6, 5); print("0) Menu");
	MoveCurTo(7, 5); print(inputPrompt);
}

void SettingPageHandler() {
	HAL_UART_Receive_IT(&huart2, (unsigned char *)receive, 1);
	if(receive[0] != '\r') return;
	receive[0] = 'X';
	if(strcmp(finishedLine, "1") == 0) {
		SettingDifficultyInit();
		gameData.gameState = 21;
		return;
	}
	if(strcmp(finishedLine, "2") == 0) {
		SettingSpeedInit();
		gameData.gameState = 22;
		return;
	}
	if(strcmp(finishedLine, "0") == 0) {
		Refresh();
		WelcomePagePrompt();
		gameData.gameState = 0;
		return;
	}
	ClearLine();
	DrawGrid();
	MoveCurTo(7, 5); print(inputPrompt);
}

void SettingDifficultyInit() {
	Refresh();
	DrawGrid();
	PrintCentered("Difficulty Setting", 3, -1);
	char difficultyPrompt[50];
	sprintf(difficultyPrompt, "Current Difficulty: %d (1 - 10 Lines)", gameData.numberOfLines);
	MoveCurTo(4, 5); print(difficultyPrompt);
	MoveCurTo(5, 5); print("0) Back");
	MoveCurTo(6, 5); print(inputPrompt);
}

void SettingDifficultyHandler() {
	HAL_UART_Receive_IT(&huart2, (unsigned char *)receive, 1);
	if(receive[0] != '\r') return;
	receive[0] = 'X';
	if(strcmp(finishedLine, "10") == 0){
		gameData.numberOfLines = 10;
		SettingDifficultyInit();
		return;
	}
	if('1' <= finishedLine[0] && finishedLine[0] <= '9' && strlen(finishedLine) == 1) {
		gameData.numberOfLines = finishedLine[0] - '0';
		SettingDifficultyInit();
		return;
	}
	if(strcmp(finishedLine, "0") == 0){
		gameData.gameState = 2;
		SettingPageInit();
		return;
	}
	ClearLine();
	DrawGrid();
	MoveCurTo(6, 5); print(inputPrompt);
}

void SettingSpeedInit() {
	Refresh();
	DrawGrid();
	PrintCentered("Speed Setting", 3, -1);
	char speedPrompt[70];
	sprintf(speedPrompt, "Current Speed: %d (1 - 10 Letter per Second per Line)", gameData.speed);
	MoveCurTo(4, 5); print(speedPrompt);
	MoveCurTo(5, 5); print("0) Back");
	MoveCurTo(6, 5); print(inputPrompt);
}

void SettingSpeedHandler() {
	HAL_UART_Receive_IT(&huart2, (unsigned char *)receive, 1);
	if(receive[0] != '\r') return;
	receive[0] = 'X';
	if(strcmp(finishedLine, "10") == 0){
		gameData.speed = 10;
		ChangeTimerPeriod();
		SettingSpeedInit();
		return;
	}
	if(49 <= finishedLine[0] && finishedLine[0] <= 57 && strlen(finishedLine) == 1) {
		gameData.speed = finishedLine[0] - '0';
		ChangeTimerPeriod();
		SettingSpeedInit();
		return;
	}
	if(strcmp(finishedLine, "0") == 0) {
		gameData.gameState = 2;
		SettingPageInit();
		return;
	}
	ClearLine();
	DrawGrid();
	MoveCurTo(6, 5); print(inputPrompt);
}

void ChangeTimerPeriod(){
	HAL_TIM_Base_Stop_IT(&htim2);
    TIM2->ARR = 11000 - (gameData.speed * 1000) - 1;
    TIM2->CNT = 0;
	HAL_TIM_Base_Start_IT(&htim2);
}

char * stringGetLastKBit(char * s, int k){
	int len = strlen(s);
	if(len <= k) return s;
	return s + len - k;
}

void Refresh() {
	print(clearAndMoveCurTopLeft);
}

void ClearLine() {
	print(clearLine);
}

void MoveCurTo(int row, int col) {
	char position[20];
	sprintf(position, "\033[%d;%dH", row, col);
	print(position);
}

void SetColorBrightRedAll() {
	print("\033[91;101m");
}

void SetColorBrightRedBG_BlackLetter() {
	print("\033[30;101m");
}

void SetColorResetWithBold() {
	print("\033[0m\033[1m"); //WithBold
}

void SetColorBrightCyanAll() {
	print("\033[96;106m");
}

void SetFontBold() {
	print("\033[1m");
}

void DrawGrid() {
	MoveCurTo(1, 1);
	SetColorBrightCyanAll();
	print("12345678901234567890123456789012345678901234567890123456789012345678901234567890\r\n"); // 80
	for(int i = 2; i < 25; i++) {
		char toPrint[20];
		sprintf(toPrint, "OO\033[%d;79HOO\r\n", i);
		print(toPrint);
	}
	print("12345678901234567890123456789012345678901234567890123456789012345678901234567890"); // 80
	MoveCurTo(2, 3);
	SetColorResetWithBold();
}

void PrintCentered(const void * s, int row, int len) {
	if(len == -1)
		len = strlen(s);
	int col = 40 - (len / 2);
	MoveCurTo(row, col);
	print(s);
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  init();
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  driver();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 9999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 9999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 921600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void TIM2_IRQHandler(void)
{
  /* USER CODE BEGIN TIM2_IRQn 0 */
	PlayingPageTimerCallback();
  /* USER CODE END TIM2_IRQn 0 */
  HAL_TIM_IRQHandler(&htim2);
  /* USER CODE BEGIN TIM2_IRQn 1 */

  /* USER CODE END TIM2_IRQn 1 */
}
/* USER CODE END 4 */

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
