/*
 * main.c
 *
 *  Created on: Jun 15, 2018
 *      Author: @duplico
 */


/*
 * P1.0 radio CS - 1.0
 * P1.1 radio CLK - 1.5
 * P1.2 radio SIMO - 1.4
 * P1.3 radio SOMI - 1.6
 * P1.6 radio EN   - 2.5
 * P1.7 radio IRQ  - 2.6
 * P2.0 XTAL
 * P2.1 XTAL
 * P2.2 switch (right is high)
 *   |----/\/\/\/\/-----VCC
 */

#include <stdint.h>

#include "driverlib.h"
#include <msp430fr2433.h>

#include "rfm75.h"

uint16_t status;
volatile uint8_t f_time_loop;
volatile uint16_t csecs=0;

rfm75_rx_callback_fn radio_rx_done;
rfm75_tx_callback_fn radio_tx_done;

void delay_millis(unsigned long mils) {
    while (mils) {
        __delay_cycles(1000);
        mils--;
    }
}

void init_io() {
    // The magic FRAM make-it-work command:
    PMM_unlockLPM5();

    P2DIR |= BIT2; // Green LED
    P2OUT &= ~BIT2;
}

void radio_rx_done(uint8_t* data, uint8_t len, uint8_t pipe) {
    // it was an rx:
    // light some shit up!
    P2OUT |= BIT2;
    csecs = 100;
}

void radio_tx_done(uint8_t ack) {

}

void main (void)
 {
    //Stop watchdog timer
    WDT_A_hold(WDT_A_BASE);

    init_io();

    // On boot, the clock system is as follows:
    // * MCLK and SMCLK -> DCOCLKDIV (divided DCO) (1 MHz)
    // * ACLK           -> REFO (32k internal oscillator)

    // We need timer A3 for our loop below.
    Timer_A_initUpModeParam timer_param = {0};
    timer_param.clockSource = TIMER_A_CLOCKSOURCE_SMCLK; // 1 MHz
    // We want this to go every 10 ms, so at 100 Hz (every 10,000 ticks @ 1MHz)
    //  (a centisecond clock!)
    timer_param.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1; // /1
    timer_param.timerPeriod = 10000;
    timer_param.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    timer_param.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE;
    timer_param.timerClear = TIMER_A_SKIP_CLEAR;
    timer_param.startTimer = false;

    rfm75_init(35, &radio_rx_done, &radio_tx_done);
    rfm75_post();

    Timer_A_initUpMode(TIMER_A1_BASE, &timer_param);
    Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);

    __bis_SR_register(GIE);

    while (1) {
        if (f_rfm75_interrupt) {
            f_rfm75_interrupt = 0;
            rfm75_deferred_interrupt();
        }

        if (f_time_loop) {
            // centisecond.
            if (!csecs) {
                // turn some shit off
                P2OUT &= ~BIT2;
            }
        }
        __bis_SR_register(LPM0_bits);
    }
}

// 0xFFF4 Timer1_A3 CC0
#pragma vector=TIMER1_A0_VECTOR
__interrupt
void TIMER_ISR() {
    // All we have here is TA0CCR0 CCIFG0
    f_time_loop = 1;
    if (csecs) {
        csecs--;
    }
    LPM0_EXIT;
}
