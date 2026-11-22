/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  -------------------------------Okragły wyświetlacz GC9A01--------------------*/
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "BME280_STM32.h"
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LCD_CS_LOW()   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET)
#define LCD_CS_HIGH()  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET)
#define LCD_DC_CMD()   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET)
#define LCD_DC_DATA()  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET)
#define LCD_RST_LOW()  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET)
#define LCD_RST_HIGH() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET)

#define LCD_W 240
#define LCD_H 240

/* kolory RGB565 */
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define WHITE   0xFFFF
#define BLACK   0x0000
#define YELLOW  0xFFE0


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void LCD_Cmd(uint8_t cmd)
{
    LCD_DC_CMD();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}

static void LCD_Data(const uint8_t *data, uint16_t len)
{
    LCD_DC_DATA();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi2, (uint8_t *)data, len, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}

/* komenda + dane z tablicy: {cmd, liczba_danych, dane...} */
static void LCD_CmdData(uint8_t cmd, const uint8_t *data, uint8_t len)
{
    LCD_Cmd(cmd);
    if (len) LCD_Data(data, len);
}

static const uint8_t init_seq[] = {
    0xEF,0,  0xEB,1,0x14,  0xFE,0,  0xEF,0,  0xEB,1,0x14,
    0x84,1,0x40, 0x85,1,0xFF, 0x86,1,0xFF, 0x87,1,0xFF,
    0x88,1,0x0A, 0x89,1,0x21, 0x8A,1,0x00, 0x8B,1,0x80,
    0x8C,1,0x01, 0x8D,1,0x01, 0x8E,1,0xFF, 0x8F,1,0xFF,
    0xB6,2,0x00,0x20,
	0x36,1,0xC8,            /* orientacja, BGR */
    0x3A,1,0x05,            /* 16 bit / piksel */
    0x90,4,0x08,0x08,0x08,0x08,
    0xBD,1,0x06, 0xBC,1,0x00,
    0xFF,3,0x60,0x01,0x04,
    0xC3,1,0x13, 0xC4,1,0x13, 0xC9,1,0x22, 0xBE,1,0x11,
    0xE1,2,0x10,0x0E,
    0xDF,3,0x21,0x0C,0x02,
    0xF0,6,0x45,0x09,0x08,0x08,0x26,0x2A,
    0xF1,6,0x43,0x70,0x72,0x36,0x37,0x6F,
    0xF2,6,0x45,0x09,0x08,0x08,0x26,0x2A,
    0xF3,6,0x43,0x70,0x72,0x36,0x37,0x6F,
    0xED,2,0x1B,0x0B, 0xAE,1,0x77, 0xCD,1,0x63,
    0x70,9,0x07,0x07,0x04,0x0E,0x0F,0x09,0x07,0x08,0x03,
    0xE8,1,0x34,
    0x62,12,0x18,0x0D,0x71,0xED,0x70,0x70,0x18,0x0F,0x71,0xEF,0x70,0x70,
    0x63,12,0x18,0x11,0x71,0xF1,0x70,0x70,0x18,0x13,0x71,0xF3,0x70,0x70,
    0x64,7,0x28,0x29,0xF1,0x01,0xF1,0x00,0x07,
    0x66,10,0x3C,0x00,0xCD,0x67,0x45,0x45,0x10,0x00,0x00,0x00,
    0x67,10,0x00,0x3C,0x00,0x00,0x00,0x01,0x54,0x10,0x32,0x98,
    0x74,7,0x10,0x85,0x80,0x00,0x00,0x4E,0x00,
    0x98,2,0x3E,0x07,
    0x35,0,                 /* tearing effect on */
    0x21,0,                 /* inwersja kolorów (wymagana w GC9A01) */
};

static void LCD_Init(void)
{
    LCD_CS_HIGH();
    LCD_RST_HIGH(); HAL_Delay(10);
    LCD_RST_LOW();  HAL_Delay(10);
    LCD_RST_HIGH(); HAL_Delay(120);

    uint16_t i = 0;
    while (i < sizeof(init_seq))
    {
        uint8_t cmd = init_seq[i++];
        uint8_t len = init_seq[i++];
        LCD_CmdData(cmd, &init_seq[i], len);
        i += len;
    }

    LCD_Cmd(0x11);  HAL_Delay(120);   /* wyjście ze sleep */
    LCD_Cmd(0x29);  HAL_Delay(20);    /* włącz wyświetlanie */
}

static void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t d[4];
    d[0] = x0 >> 8; d[1] = x0; d[2] = x1 >> 8; d[3] = x1;
    LCD_CmdData(0x2A, d, 4);          /* kolumny */
    d[0] = y0 >> 8; d[1] = y0; d[2] = y1 >> 8; d[3] = y1;
    LCD_CmdData(0x2B, d, 4);          /* wiersze */
    LCD_Cmd(0x2C);                    /* zapis pamięci */
}

static void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    static uint8_t line[LCD_W * 2];
    for (uint16_t i = 0; i < w; i++)
    {
        line[2*i]   = color >> 8;     /* najpierw starszy bajt */
        line[2*i+1] = color & 0xFF;
    }

    LCD_SetWindow(x, y, x + w - 1, y + h - 1);
    LCD_DC_DATA();
    LCD_CS_LOW();
    for (uint16_t row = 0; row < h; row++)
        HAL_SPI_Transmit(&hspi2, line, w * 2, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}
/* Jeden piksel */
static void LCD_DrawPixel(int16_t x, int16_t y, uint16_t color)
{
    if (x < 0 || y < 0 || x >= LCD_W || y >= LCD_H) return;
    uint8_t d[2] = { color >> 8, color & 0xFF };
    LCD_SetWindow(x, y, x, y);
    LCD_Data(d, 2);
}

/* Okrąg (sam obwód): środek (x0, y0), promień r */
static void LCD_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t x = r, y = 0;
    int16_t err = 1 - r;

    while (x >= y)
    {
        /* 8 symetrycznych punktów */
        LCD_DrawPixel(x0 + x, y0 + y, color);
        LCD_DrawPixel(x0 + y, y0 + x, color);
        LCD_DrawPixel(x0 - y, y0 + x, color);
        LCD_DrawPixel(x0 - x, y0 + y, color);
        LCD_DrawPixel(x0 - x, y0 - y, color);
        LCD_DrawPixel(x0 - y, y0 - x, color);
        LCD_DrawPixel(x0 + y, y0 - x, color);
        LCD_DrawPixel(x0 + x, y0 - y, color);

        y++;
        if (err < 0)
            err += 2 * y + 1;
        else
        {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

/* Linia pozioma - pomocnicza do wypełnionego koła */
static void LCD_HLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    if (y < 0 || y >= LCD_H) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > LCD_W) w = LCD_W - x;
    if (w > 0) LCD_FillRect(x, y, w, 1, color);
}

/* Koło wypełnione */
static void LCD_FillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t x = r, y = 0;
    int16_t err = 1 - r;

    while (x >= y)
    {
        LCD_HLine(x0 - x, y0 + y, 2 * x + 1, color);
        LCD_HLine(x0 - x, y0 - y, 2 * x + 1, color);
        LCD_HLine(x0 - y, y0 + x, 2 * y + 1, color);
        LCD_HLine(x0 - y, y0 - x, 2 * y + 1, color);

        y++;
        if (err < 0)
            err += 2 * y + 1;
        else
        {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}
static void LCD_Fill(uint16_t color)

{
    LCD_FillRect(0, 0, LCD_W, LCD_H, color);
}

/* =====================================================================
 *  Wyświetlanie napisów na GC9A01
 *  Wklej CAŁOŚĆ do sekcji  USER CODE BEGIN 0  w main.c,
 *  POD funkcją LCD_Fill() (korzysta z LCD_SetWindow i LCD_Data).
 *
 *  Czcionka 5x7 (ASCII 32..126), każdy znak zajmuje 6x8 pikseli.
 *  Parametr "scale" powiększa tekst: 1 = 6x8, 2 = 12x16, 3 = 18x24 ...
 *  Polskie znaki (ą, ę, ł...) nie są obsługiwane - pojawi się "?".
 * ===================================================================== */

#define FONT_W        5
#define FONT_H        7
#define CHAR_W        6      /* 5 kolumn + 1 odstępu */
#define CHAR_H        8      /* 7 wierszy + 1 odstępu */
#define TEXT_MAX_SCALE 6

static const uint8_t font5x7[] = {
    0x00,0x00,0x00,0x00,0x00, /* ' ' */
    0x00,0x00,0x5F,0x00,0x00, /* !  */
    0x00,0x07,0x00,0x07,0x00, /* "  */
    0x14,0x7F,0x14,0x7F,0x14, /* #  */
    0x24,0x2A,0x7F,0x2A,0x12, /* $  */
    0x23,0x13,0x08,0x64,0x62, /* %  */
    0x36,0x49,0x55,0x22,0x50, /* &  */
    0x00,0x05,0x03,0x00,0x00, /* '  */
    0x00,0x1C,0x22,0x41,0x00, /* (  */
    0x00,0x41,0x22,0x1C,0x00, /* )  */
    0x08,0x2A,0x1C,0x2A,0x08, /* *  */
    0x08,0x08,0x3E,0x08,0x08, /* +  */
    0x00,0x50,0x30,0x00,0x00, /* ,  */
    0x08,0x08,0x08,0x08,0x08, /* -  */
    0x00,0x60,0x60,0x00,0x00, /* .  */
    0x20,0x10,0x08,0x04,0x02, /* /  */
    0x3E,0x51,0x49,0x45,0x3E, /* 0  */
    0x00,0x42,0x7F,0x40,0x00, /* 1  */
    0x42,0x61,0x51,0x49,0x46, /* 2  */
    0x21,0x41,0x45,0x4B,0x31, /* 3  */
    0x18,0x14,0x12,0x7F,0x10, /* 4  */
    0x27,0x45,0x45,0x45,0x39, /* 5  */
    0x3C,0x4A,0x49,0x49,0x30, /* 6  */
    0x01,0x71,0x09,0x05,0x03, /* 7  */
    0x36,0x49,0x49,0x49,0x36, /* 8  */
    0x06,0x49,0x49,0x29,0x1E, /* 9  */
    0x00,0x36,0x36,0x00,0x00, /* :  */
    0x00,0x56,0x36,0x00,0x00, /* ;  */
    0x08,0x14,0x22,0x41,0x00, /* <  */
    0x14,0x14,0x14,0x14,0x14, /* =  */
    0x00,0x41,0x22,0x14,0x08, /* >  */
    0x02,0x01,0x51,0x09,0x06, /* ?  */
    0x32,0x49,0x79,0x41,0x3E, /* @  */
    0x7E,0x11,0x11,0x11,0x7E, /* A  */
    0x7F,0x49,0x49,0x49,0x36, /* B  */
    0x3E,0x41,0x41,0x41,0x22, /* C  */
    0x7F,0x41,0x41,0x22,0x1C, /* D  */
    0x7F,0x49,0x49,0x49,0x41, /* E  */
    0x7F,0x09,0x09,0x01,0x01, /* F  */
    0x3E,0x41,0x41,0x51,0x32, /* G  */
    0x7F,0x08,0x08,0x08,0x7F, /* H  */
    0x00,0x41,0x7F,0x41,0x00, /* I  */
    0x20,0x40,0x41,0x3F,0x01, /* J  */
    0x7F,0x08,0x14,0x22,0x41, /* K  */
    0x7F,0x40,0x40,0x40,0x40, /* L  */
    0x7F,0x02,0x04,0x02,0x7F, /* M  */
    0x7F,0x04,0x08,0x10,0x7F, /* N  */
    0x3E,0x41,0x41,0x41,0x3E, /* O  */
    0x7F,0x09,0x09,0x09,0x06, /* P  */
    0x3E,0x41,0x51,0x21,0x5E, /* Q  */
    0x7F,0x09,0x19,0x29,0x46, /* R  */
    0x46,0x49,0x49,0x49,0x31, /* S  */
    0x01,0x01,0x7F,0x01,0x01, /* T  */
    0x3F,0x40,0x40,0x40,0x3F, /* U  */
    0x1F,0x20,0x40,0x20,0x1F, /* V  */
    0x7F,0x20,0x18,0x20,0x7F, /* W  */
    0x63,0x14,0x08,0x14,0x63, /* X  */
    0x03,0x04,0x78,0x04,0x03, /* Y  */
    0x61,0x51,0x49,0x45,0x43, /* Z  */
    0x00,0x7F,0x41,0x41,0x00, /* [  */
    0x02,0x04,0x08,0x10,0x20, /* \  */
    0x00,0x41,0x41,0x7F,0x00, /* ]  */
    0x04,0x02,0x01,0x02,0x04, /* ^  */
    0x40,0x40,0x40,0x40,0x40, /* _  */
    0x00,0x01,0x02,0x04,0x00, /* `  */
    0x20,0x54,0x54,0x54,0x78, /* a  */
    0x7F,0x48,0x44,0x44,0x38, /* b  */
    0x38,0x44,0x44,0x44,0x20, /* c  */
    0x38,0x44,0x44,0x48,0x7F, /* d  */
    0x38,0x54,0x54,0x54,0x18, /* e  */
    0x08,0x7E,0x09,0x01,0x02, /* f  */
    0x08,0x14,0x54,0x54,0x3C, /* g  */
    0x7F,0x08,0x04,0x04,0x78, /* h  */
    0x00,0x44,0x7D,0x40,0x00, /* i  */
    0x20,0x40,0x44,0x3D,0x00, /* j  */
    0x00,0x7F,0x10,0x28,0x44, /* k  */
    0x00,0x41,0x7F,0x40,0x00, /* l  */
    0x7C,0x04,0x18,0x04,0x78, /* m  */
    0x7C,0x08,0x04,0x04,0x78, /* n  */
    0x38,0x44,0x44,0x44,0x38, /* o  */
    0x7C,0x14,0x14,0x14,0x08, /* p  */
    0x08,0x14,0x14,0x18,0x7C, /* q  */
    0x7C,0x08,0x04,0x04,0x08, /* r  */
    0x48,0x54,0x54,0x54,0x20, /* s  */
    0x04,0x3F,0x44,0x40,0x20, /* t  */
    0x3C,0x40,0x40,0x20,0x7C, /* u  */
    0x1C,0x20,0x40,0x20,0x1C, /* v  */
    0x3C,0x40,0x30,0x40,0x3C, /* w  */
    0x44,0x28,0x10,0x28,0x44, /* x  */
    0x0C,0x50,0x50,0x50,0x3C, /* y  */
    0x44,0x64,0x54,0x4C,0x44, /* z  */
    0x00,0x08,0x36,0x41,0x00, /* {  */
    0x00,0x00,0x7F,0x00,0x00, /* |  */
    0x00,0x41,0x36,0x08,0x00, /* }  */
    0x08,0x04,0x08,0x10,0x08, /* ~  */
};

/* Rysuje jeden znak. (x, y) = lewy górny róg. */
static void LCD_DrawChar(uint16_t x, uint16_t y, char c,
                         uint16_t fg, uint16_t bg, uint8_t scale)
{
    static uint8_t buf[CHAR_W * TEXT_MAX_SCALE * CHAR_H * TEXT_MAX_SCALE * 2];

    if (scale < 1) scale = 1;
    if (scale > TEXT_MAX_SCALE) scale = TEXT_MAX_SCALE;
    if (c < 32 || c > 126) c = '?';

    uint16_t w = CHAR_W * scale;
    uint16_t h = CHAR_H * scale;
    if (x + w > LCD_W || y + h > LCD_H) return;   /* poza ekranem */

    const uint8_t *glyph = &font5x7[(c - 32) * FONT_W];
    uint32_t n = 0;

    for (uint16_t py = 0; py < h; py++)
    {
        uint8_t row = py / scale;
        for (uint16_t px = 0; px < w; px++)
        {
            uint8_t col = px / scale;
            uint16_t color = bg;
            if (col < FONT_W && row < FONT_H && (glyph[col] & (1 << row)))
                color = fg;
            buf[n++] = color >> 8;
            buf[n++] = color & 0xFF;
        }
    }

    LCD_SetWindow(x, y, x + w - 1, y + h - 1);
    LCD_Data(buf, (uint16_t)n);
}

/* Rysuje napis zaczynając od (x, y). '\n' przechodzi do nowej linii. */
static void LCD_DrawString(uint16_t x, uint16_t y, const char *s,
                           uint16_t fg, uint16_t bg, uint8_t scale)
{
    uint16_t x0 = x;
    while (*s)
    {
        if (*s == '\n')
        {
            x = x0;
            y += CHAR_H * scale;
        }
        else
        {
            LCD_DrawChar(x, y, *s, fg, bg, scale);
            x += CHAR_W * scale;
        }
        s++;
    }
}

/* Rysuje napis wyśrodkowany poziomo na wysokości y (wygodne na okrągłym ekranie). */
static void LCD_DrawStringCentered(uint16_t y, const char *s,
                                   uint16_t fg, uint16_t bg, uint8_t scale)
{
    uint16_t len = 0;
    for (const char *p = s; *p; p++) len++;

    uint16_t w = len * CHAR_W * scale;
    uint16_t x = (w < LCD_W) ? (LCD_W - w) / 2 : 0;
    LCD_DrawString(x, y, s, fg, bg, scale);
}

/* ------------------------------------------------------------------
 *  PRZYKŁAD UŻYCIA (np. w USER CODE BEGIN 2, po LCD_Init()):
 *
 *    LCD_Fill(BLACK);
 *    LCD_DrawStringCentered(70,  "Hello",     WHITE,  BLACK, 4);
 *    LCD_DrawStringCentered(115, "STM32G474", YELLOW, BLACK, 2);
 *    LCD_DrawStringCentered(145, "GC9A01",    GREEN,  BLACK, 2);
 *
 *  Licznik w pętli while(1):
 *
 *    static uint32_t licznik = 0;
 *    char txt[16];
 *    sprintf(txt, "%5lu", licznik++);          // #include <stdio.h>
 *    LCD_DrawStringCentered(180, txt, RED, BLACK, 3);
 *    HAL_Delay(200);
 * ------------------------------------------------------------------ */


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
  MX_SPI2_Init();
  MX_SPI3_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
  LCD_Init();

   LCD_Fill(BLACK);
	  LCD_DrawCircle(120, 120, 115, WHITE);
LCD_DrawStringCentered(100, "1. Test String", 0x5566,0x0000, 2);
int  bmeStatus = BME280_Config(OSRS_2, OSRS_16, OSRS_1, MODE_NORMAL, T_SB_0p5, IIR_16);
LCD_DrawStringCentered(150, (char)bmeStatus, 0x5566,0x0000, 2);
/* USER CODE END 2 */

  /* Initialize led */
  BSP_LED_Init(LED_GREEN);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  float temperature = 0.0f;
  float pressure = 0.0f;
  float humidity = 0.0f;

  while (1)
  {
      HAL_Delay(1000);

	    //LCD_FillRect(0,   0,   120, 120, RED);
	    //LCD_FillRect(120, 0,   120, 120, GREEN);
	    //LCD_FillRect(0,   120, 120, 120, BLUE);
	    //LCD_FillRect(120, 120, 120, 120, YELLOW);
	    //LCD_FillRect(90,  90,  60,  60,  WHITE);
	    //HAL_Delay(1000);
	    //LCD_Fill(BLACK);
      BME280_Measure(&temperature, &pressure, &humidity);
      printf("T=%d.%02d C  P=%ld Pa  ID=0x%02X\r\n", (int)temperature, abs((int)(temperature * 100) % 100),   (long)pressure, chipID);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
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
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
