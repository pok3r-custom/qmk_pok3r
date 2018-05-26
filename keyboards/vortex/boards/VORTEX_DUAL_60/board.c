/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio
                        (C) 2018 Charlie Waters

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.h"
#include "hal.h"

#include <string.h>

#define PBIT(PORT, LINE) ((PAL_PORT(LINE) == PORT) ? (1 << PAL_PAD(LINE)) : 0)
#define PAFIO_L(PORT, LINE, AF) (((PAL_PORT(LINE) == PORT) && (PAL_PAD(LINE) < 8)) ? (AF << (PAL_PAD(LINE) << 2)) : 0)
#define PAFIO_H(PORT, LINE, AF) (((PAL_PORT(LINE) == PORT) && (PAL_PAD(LINE) >= 8)) ? (AF << ((PAL_PAD(LINE) - 8) << 2)) : 0)
#define PAFIO(PORT, N, LINE, AF) ((N) ? PAFIO_H(PORT, LINE, AF) : PAFIO_L(PORT, LINE, AF))

#define OUT_BITS(PORT) (\
    PBIT(PORT, LINE_ROW1) | \
    PBIT(PORT, LINE_ROW2) | \
    PBIT(PORT, LINE_ROW3) | \
    PBIT(PORT, LINE_ROW4) | \
    PBIT(PORT, LINE_ROW5) | \
    PBIT(PORT, LINE_ROW6) | \
    PBIT(PORT, LINE_ROW7) | \
    PBIT(PORT, LINE_ROW8) | \
    PBIT(PORT, LINE_ROW9) | \
    PBIT(PORT, LINE_SPI_CS) | \
    PBIT(PORT, LINE_LED_SHIFT_DATA) | \
    PBIT(PORT, LINE_LED_SHIFT_CLK) | \
    PBIT(PORT, LINE_LED_SHIFT_CLR) | \
    PBIT(PORT, LINE_TROW1) | \
    PBIT(PORT, LINE_TCOL1) | \
    PBIT(PORT, LINE_TCOL2) | \
    PBIT(PORT, LINE_TCOL3) | \
    PBIT(PORT, LINE_TCOL4) | \
    PBIT(PORT, LINE_TCOL5) | \
    PBIT(PORT, LINE_TCOL6) | \
    PBIT(PORT, LINE_TCOL7) | \
    PBIT(PORT, LINE_TCOL8) | \
    PBIT(PORT, LINE_LED_VOLTAGE) | \
    PBIT(PORT, LINE_OD_RED1) | \
    PBIT(PORT, LINE_OD_RED2) | \
    PBIT(PORT, LINE_OD_RED3) | \
0)

#define IN_BITS(PORT) (\
    PBIT(PORT, LINE_COL1) | \
    PBIT(PORT, LINE_COL2) | \
    PBIT(PORT, LINE_COL3) | \
    PBIT(PORT, LINE_COL4) | \
    PBIT(PORT, LINE_COL5) | \
    PBIT(PORT, LINE_COL6) | \
    PBIT(PORT, LINE_COL7) | \
    PBIT(PORT, LINE_COL8) | \
0)

#define OD_BITS(PORT) (\
    PBIT(PORT, LINE_LED_VOLTAGE) | \
    PBIT(PORT, LINE_OD_RED1) | \
    PBIT(PORT, LINE_OD_RED2) | \
    PBIT(PORT, LINE_OD_RED3) | \
0)

#define AF_BITS(PORT, N) (\
    PAFIO(PORT, N, LINE_ROW1,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_ROW2,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_ROW3,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_ROW4,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_ROW5,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_ROW6,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_ROW7,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_ROW8,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_ROW9,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_COL1,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_COL2,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_COL3,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_COL4,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_COL5,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_COL6,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_COL7,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_COL8,     AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_SPI_SCK,  AFIO_SPI)  | \
    PAFIO(PORT, N, LINE_SPI_MOSI, AFIO_SPI)  | \
    PAFIO(PORT, N, LINE_SPI_MISO, AFIO_SPI)  | \
    PAFIO(PORT, N, LINE_SPI_CS,   AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_LED_SHIFT_DATA, AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_LED_SHIFT_CLK, AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_LED_SHIFT_CLR, AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_TROW1, AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_TCOL1, AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_TCOL2, AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_TCOL3, AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_TCOL4, AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_TCOL5, AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_TCOL6, AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_TCOL7, AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_TCOL8, AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_LED_VOLTAGE, AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_OD_RED1, AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_OD_RED2, AFIO_GPIO) | \
    PAFIO(PORT, N, LINE_OD_RED3, AFIO_GPIO) | \
0)

#define PESSR_L(LINE) ((PAL_PAD(LINE) < 8) ? (HT32_PAL_IDX(LINE) << (PAL_PAD(LINE) * 4uL)) : 0)
#define PESSR_H(LINE) ((PAL_PAD(LINE) >=8) ? (HT32_PAL_IDX(LINE) << ((PAL_PAD(LINE) - 8uL) * 4uL)) : 0)

/* doesn't work due to https://gcc.gnu.org/bugzilla/show_bug.cgi?id=4210 */
/*
#define PESSR(N, LINE) ((N) ? (PESSR_H(LINE)) : (PESSR_L(LINE)))
#define ESSR_BITS(N) (\
    PESSR(N, LINE_COL1) | \
    PESSR(N, LINE_COL2) | \
    PESSR(N, LINE_COL3) | \
    PESSR(N, LINE_COL4) | \
    PESSR(N, LINE_COL5) | \
    PESSR(N, LINE_COL6) | \
    PESSR(N, LINE_COL7) | \
    PESSR(N, LINE_COL8) | \
0)
*/

#define ESSR_BITS_0 (\
    PESSR_L(LINE_COL1) | \
    PESSR_L(LINE_COL2) | \
    PESSR_L(LINE_COL3) | \
    PESSR_L(LINE_COL4) | \
0)

#define ESSR_BITS_1 (\
    PESSR_H(LINE_COL5) | \
    PESSR_H(LINE_COL6) | \
    PESSR_H(LINE_COL7) | \
    PESSR_H(LINE_COL8) | \
0)

/**
 * @brief   PAL setup.
 * @details Digital I/O ports static configuration as defined in @p board.h.
 *          This variable is used by the HAL when initializing the PAL driver.
 */
const PALConfig pal_default_config = {
    // GPIO A
    .setup[0] = {
        .DIR = OUT_BITS(IOPORTA),
        .INE = IN_BITS(IOPORTA),
        .PU = IN_BITS(IOPORTA),
        .PD = 0x0000,
        .OD = OD_BITS(IOPORTA),
        .DRV = 0x0000,
        .LOCK = 0x0000,
        .OUT = 0x0000,
        .CFG[0] = AF_BITS(IOPORTA, 0),
        .CFG[1] = AF_BITS(IOPORTA, 1),
    },
    // GPIO B
    .setup[1] = {
        .DIR = OUT_BITS(IOPORTB),
        .INE = IN_BITS(IOPORTB),
        .PU = IN_BITS(IOPORTB),
        .PD = 0x0000,
        .OD = OD_BITS(IOPORTB),
        .DRV = 0x0000,
        .LOCK = 0x0000,
        .OUT = 0x0000,
        .CFG[0] = AF_BITS(IOPORTB, 0),
        .CFG[1] = AF_BITS(IOPORTB, 1),
    },
    // GPIO C
    .setup[2] = {
        .DIR = OUT_BITS(IOPORTC),
        .INE = IN_BITS(IOPORTC),
        .PU = IN_BITS(IOPORTC),
        .PD = 0x0000,
        .OD = OD_BITS(IOPORTC),
        .DRV = 0x0000,
        .LOCK = 0x0000,
        .OUT = 0x0000,
        .CFG[0] = AF_BITS(IOPORTC, 0),
        .CFG[1] = AF_BITS(IOPORTC, 1),
    },
    // GPIO D
    .setup[3] = {
        .DIR = OUT_BITS(IOPORTD),
        .INE = IN_BITS(IOPORTD),
        .PU = IN_BITS(IOPORTD),
        .PD = 0x0000,
        .OD = OD_BITS(IOPORTD),
        .DRV = 0x0000,
        .LOCK = 0x0000,
        .OUT = 0x0000,
        .CFG[0] = AF_BITS(IOPORTD, 0),
        .CFG[1] = AF_BITS(IOPORTD, 1),
    },
    // GPIO E
    .setup[4] = {
        .DIR = OUT_BITS(IOPORTE),
        .INE = IN_BITS(IOPORTE),
        .PU = IN_BITS(IOPORTE),
        .PD = 0x0000,
        .OD = OD_BITS(IOPORTE),
        .DRV = 0x0000,
        .LOCK = 0x0000,
        .OUT = 0x0000,
        .CFG[0] = AF_BITS(IOPORTE, 0),
        .CFG[1] = AF_BITS(IOPORTE, 1),
    },
    // Enable Column Pins for EXTI
    .ESSR[0] = ESSR_BITS_0,
    .ESSR[1] = ESSR_BITS_1,
};

const ioline_t row_list[MATRIX_ROWS] = {
    LINE_ROW1,
    LINE_ROW2,
    LINE_ROW3,
    LINE_ROW4,
    LINE_ROW5,
    LINE_ROW6,
    LINE_ROW7,
    LINE_ROW8,
    LINE_ROW9,
};

const ioline_t col_list[MATRIX_COLS] = {
    LINE_COL1,
    LINE_COL2,
    LINE_COL3,
    LINE_COL4,
    LINE_COL5,
    LINE_COL6,
    LINE_COL7,
    LINE_COL8,
};

void __early_init(void) {
    ht32_clock_init();
}

#if HAL_USE_GPT == TRUE
// GPT Initialization

static const GPTConfig bftm0_config = {
    .frequency = 1000000,
    .callback = NULL,
};

void gpt_init(void) {
    gptStart(&GPTD_BFTM0, &bftm0_config);
}
#endif

// SPI Initialization

static const SPIConfig spi1_config = {
    .end_cb = NULL,
    .cr0 = SPI_CR0_SELOEN,
    .cr1 = 8 | SPI_CR1_FORMAT_MODE0 | SPI_CR1_MODE,
    .cpr = 1,
    .fcr = 0,
};

void spi_init(void) {
    spiStart(&SPID1, &spi1_config);
    palSetLine(LINE_SPI_CS);
}

#if HAL_USE_UART == TRUE
// UART Initialization

static const UARTConfig usart0_config = {
    .mdr = USART_MDR_MODE_NORMAL,
    .lcr = USART_LCR_WLS_8BIT,
    .fcr = USART_FCR_URTXEN,
    .baud = 115200,
};

void uart_init(void) {
    uartStart(&USARTD0, &usart0_config);
}

void uart_send(const char *str) {
    uartStartSend(&USARTD0, strlen(str), str);
}

// Override the sendchar_pf from usb_main.c
// So printf() will print to serial instead
/*
void sendchar_pf(void *p, char c) {
    uartStartSend(&USARTD0, 1, &c);
}
*/
#endif

/**
 * @brief   Board-specific initialization code.
 * @todo    Add your board-specific code, if any.
 */
void boardInit(void) {
#if HAL_USE_GPT == TRUE
    gpt_init();
#endif
    spi_init();
#if HAL_USE_UART == TRUE
    uart_init();
#endif
    palSetLine(LINE_LED_VOLTAGE); //led voltage off after boot
}
