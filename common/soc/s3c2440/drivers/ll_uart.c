/**
 * @file ll_uart.c
 * @brief S3C2440 UART Low Level Driver (LLD)
 *
 * Pure register-level operations. No board-level logic.
 */

#include "ll_uart.h"
#include "ll_clock.h"
#include "s3c2440_soc.h"

/* ================= UTRSTATn (UART Tx/Rx Status Register) ================= */

// Bit [2]: Transmit buffer empty (including shift register) [2, 3].
// Used in non-FIFO mode to wait for transmission completion.
#define UTRSTAT_TX_EMPTY  (1 << 2)

// Bit [0]: Receive buffer data ready [3, 4].
// Poll this bit before reading URXH.
#define UTRSTAT_RX_READY  (1 << 0)


/* ================= UFCONn (UART FIFO Control Register) ================= */

// Bit [0]: FIFO Enable. 0 = disabled (1-byte hold register), 1 = enabled [5].
#define UFCON_FIFO_ENABLE (1 << 0)

// Bit [2]: Tx FIFO Reset. Write 1 to reset; hardware auto-clears [5].
#define UFCON_TX_RESET    (1 << 2)

// Bit [1]: Rx FIFO Reset. Write 1 to reset; hardware auto-clears [5].
#define UFCON_RX_RESET    (1 << 1)


/* ================= UFSTATn (UART FIFO Status Register) ================= */

// Bit [14]: Tx FIFO Full (64 bytes). Poll before writing in FIFO mode [8].
#define UFSTAT_TX_FULL    (1 << 14)

// Bit [6]: Rx FIFO Full (64 bytes) [8].
#define UFSTAT_RX_FULL    (1 << 6)

// Bits [5:0]: Rx FIFO byte count mask [10].
// Use as `ufstat & UFSTAT_RX_COUNT` to extract the number of received bytes.
#define UFSTAT_RX_COUNT   (0x3F)

void ll_uart_init(uint32_t baud_rate) {
    /* 1. Configure GPIO: GPH2 -> TXD0, GPH3 -> RXD0 */
    gpio_port_t *port_h = GPIO_PORT(PORT_H);
    port_h->CON &= ~((3 << 4) | (3 << 6));
    port_h->CON |= ((2 << 4) | (2 << 6));
    port_h->UP &= ~((1 << 2) | (1 << 3));

    /* 2. Format: 8 Bits, No Parity, 1 Stop Bit (8N1) */
    UART0->ULCON = 0x03;

    /* 3. Control: Polling Mode, Disable Interrupts, No DMA */
    UART0->UCON = 0x05;

    /* 4. Enable FIFO and reset it */
    UART0->UFCON = UFCON_FIFO_ENABLE | UFCON_TX_RESET | UFCON_RX_RESET;

    /* 5. Baud Rate */
    uint32_t pclk = ll_clock_get_pclk();
    UART0->UBRDIV = (pclk / (baud_rate * 16)) - 1;
}

void ll_uart_putc(char c) {
    while (UART0->UFSTAT & UFSTAT_TX_FULL)
        ;
    UART0->UTXH = (uint8_t)c;
}

char ll_uart_getc(void) {
    while ((UART0->UFSTAT & (UFSTAT_RX_COUNT | UFSTAT_RX_FULL)) == 0)
        ;
    return (char)UART0->URXH;
}

int ll_uart_tstc(void) {
    return (UART0->UFSTAT & (UFSTAT_RX_COUNT | UFSTAT_RX_FULL)) ? 1 : 0;
}

void ll_uart_flush(void) {
    UART0->UFCON |= UFCON_RX_RESET;
}

void ll_uart_wait_tx_done(void) {
    while (!(UART0->UTRSTAT & UTRSTAT_TX_EMPTY))
        ;
}
