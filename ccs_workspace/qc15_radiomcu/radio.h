/*
 * radio.h
 *
 *  Created on: Jun 27, 2018
 *      Author: george
 */

#ifndef RADIO_H_
#define RADIO_H_

#include <stdint.h>
#include "rfm75.h"

typedef struct {
    uint16_t badge_id;
    uint8_t proto_version;
    uint8_t msg_type;
    uint8_t msg_payload[14];
    uint16_t crc16;
} radio_proto;

typedef struct { // Beacon payload (broadcast)
    uint32_t time;
    uint8_t name[10];
} radio_beacon_payload;

typedef struct { // Connect payload (unicast)
    uint8_t name[10];
    uint8_t connect_flags;
} radio_connect_payload;

typedef struct { // Progress report payload (unicast to base)
    uint8_t part_id;
    uint8_t part_data[10]; // 80 bits per segment
} radio_progress_payload;

rfm75_rx_callback_fn radio_rx_done;
rfm75_tx_callback_fn radio_tx_done;
void radio_init();

#endif /* RADIO_H_ */
