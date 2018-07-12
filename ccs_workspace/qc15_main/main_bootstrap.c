/*
 * main_bootstrap.c
 *
 *  Created on: Jul 12, 2018
 *      Author: george
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <msp430fr5972.h>
#include <driverlib.h>
#include <s25flash.h>

//#include "qc15.h"

#include "util.h"

#include "lcd111.h"
#include "ht16d35b.h"
#include "s25flash.h"
#include "ipc.h"
#include "leds.h"

#define POST_MCU 0
#define POST_LCD 1
#define POST_LED 2
#define POST_NOR 3
#define POST_IPC 4
#define POST_UPNEXT 5
#define POST_UP 6
#define POST_DOWN 7
#define POST_LEFT 8
#define POST_RIGHT 9
#define POST_SW1 10
#define POST_SW2 11
#define POST_OK 12

extern volatile uint8_t f_time_loop;
extern uint8_t s_buttons;
void poll_buttons();

void bootstrap(uint8_t fastboot) {
    uint8_t rx_from_radio[IPC_MSG_LEN_MAX] = {0};
    uint8_t bootstrap_status = 0;
    uint16_t time_32nd_secs = 0;

    // 0->1   MCU POST
    // 1->2   LCD POST
    // 2->3   LED POST
    // 3->4   NOR POST
    // 4->5   IPC POST w/ RADIOMCU POST STATUS
    // 5->6   UP
    // 6->7   DOWN
    // 7->8   LEFT
    // 8->9   RIGHT
    // 9->10  SWITCH TOGGLE
    // 10->11 SWITCH TOGGLE
    // 11->12 OK w/ any feedback

    // Tell the radio module to reboot.
    ipc_tx_byte(IPC_MSG_REBOOT);

    if (bootstrap_status == POST_MCU) {
        bootstrap_status++;
    }

    if (bootstrap_status == POST_LCD) {
        bootstrap_status++;
        if (!fastboot) {
            lcd111_text(1, "QC15 BOOTSTRAP>");
            lcd111_text(0, "LCD POST: OK");
            delay_millis(200);
        }
    }

    if (bootstrap_status == POST_LED) {
        if (ht16d_post()) {
            bootstrap_status++;
            if (!fastboot) {
                lcd111_text(0, "LED driver POST: OK");
                led_all_one_color(100, 100, 100);
                delay_millis(200);
            }
        } else {
            // POST appears to have failed.
            lcd111_text(1, "QC15 BOOTSTRAP> FAIL");
            lcd111_text(0, "LED driver POST: FAIL");
            delay_millis(2000);
            bootstrap_status++;
        }
    }

    if (bootstrap_status == POST_NOR) {
        if (s25flash_post()) {
            bootstrap_status++;
            if (!fastboot) {
                lcd111_text(0, "SPI NOR flash POST: OK");
                delay_millis(200);
            }
        } else {
            // TODO: This is FATAL, probably.
            //  We should do an animation or something so the badge does
            //  _something_.
            lcd111_text(1, "QC15 BOOTSTRAP> FAIL");
            lcd111_text(0, "SPI NOR flash POST FAIL!");
            led_all_one_color(200, 0, 0);
            delay_millis(2000);
            bootstrap_status++;
        }
    }

    while (1) {
        if (f_time_loop) {
            f_time_loop = 0;
            time_32nd_secs++;
            poll_buttons();
        }

        // Received an IPC message
        if (f_ipc_rx) {
            f_ipc_rx = 0;
            // If it's valid...
            if (ipc_get_rx(rx_from_radio)) {
                if ((rx_from_radio[0] & 0xF0) == IPC_MSG_POST) {
                    // Give the correct response.
                    ipc_tx_byte(IPC_MSG_POST); // TODO: this will change
                    if (bootstrap_status == POST_IPC) {
                        // got a POST message.
                        if (rx_from_radio[0] & 0x0F) {
                            lcd111_text(1, "QC15 BOOTSTRAP> FAIL");
                            led_all_one_color(200, 50, 0);
                            // it indicates a FAILURE on the radio side
                            if (rx_from_radio[0] & BIT0) {
                                // MCU fail
                                lcd111_text(0, "RADIOMCU: MCU FAIL");
                                delay_millis(2000);
                            }
                            if (rx_from_radio[0] & BIT1) {
                                // XT1 fail
                                lcd111_text(0, "RADIOMCU: XT1 FAIL");
                                delay_millis(2000);
                            }
                            if (rx_from_radio[0] & BIT2) {
                                // RFM75 fail
                                lcd111_text(0, "RADIOMCU: RFM75 FAIL");
                                delay_millis(2000);
                            }
                            bootstrap_status++;
                        } else {
                            // all good.
                            bootstrap_status++;
                            if (!fastboot) {
                                lcd111_text(0, "IPC POST: OK");
                                delay_millis(200);
                            }
                        }
                    }
                }

                if (bootstrap_status == POST_SW1 &&
                        (rx_from_radio[0] & 0xF0) == IPC_MSG_SWITCH) {
                    lcd111_text(0, "POST: Toggle switch back");
                    bootstrap_status++;
                } else if (bootstrap_status == POST_SW2 &&
                        (rx_from_radio[0] & 0xF0) == IPC_MSG_SWITCH) {
                    lcd111_text(0, "POST: Buttons OK");
                    delay_millis(1000);
                    lcd111_text(0, "Click UP to leave POST");
                    bootstrap_status++;
                    break;
                }

            } else {
                // CRC fail. It will resend.
            }
        }

        if (time_32nd_secs == 128 && bootstrap_status == POST_IPC) {
            time_32nd_secs = 0;
            lcd111_text(1, "QC15 BOOTSTRAP> FAIL");
            lcd111_text(0, "IPC POST> general fail");
            led_all_one_color(200, 0, 0);
            delay_millis(2000);
            bootstrap_status++;
        }

        if (bootstrap_status == POST_UPNEXT) {
            if (fastboot) {
                bootstrap_status = POST_OK;
                break;
            }
            lcd111_text(0, "POST: Click UP.");
            bootstrap_status++;
        }

        if (s_buttons) {
            if (s_buttons & BIT0) { // DOWN
                if (s_buttons & BIT4) {
                    if (bootstrap_status == POST_DOWN) {
                        lcd111_text(0, "POST: Click LEFT.");
                        bootstrap_status++;
                    }
                } else {
                    // press
                }
            }

            if (s_buttons & BIT1) {
                if (s_buttons & BIT5) { // RIGHT
                    if (bootstrap_status == POST_RIGHT) {
                        lcd111_text(0, "POST: Toggle switch.");
                        bootstrap_status++;
                    }
                } else {
                    // press
                }
            }
            if (s_buttons & BIT2) {
                if (s_buttons & BIT6) {
                    if (bootstrap_status == POST_LEFT) {
                        lcd111_text(0, "POST: Click RIGHT.");
                        bootstrap_status++;
                    }
                } else {
                    // press
                }
            }
            if (s_buttons & BIT3) {
                if (s_buttons & BIT7) {
                    if (bootstrap_status == POST_UP) {
                        lcd111_text(0, "POST: Click DOWN.");
                        bootstrap_status++;
                    }
                } else {
                    // press
                }
            }
            s_buttons = 0;
        }
    }

}