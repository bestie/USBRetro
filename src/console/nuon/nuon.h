// nuon.h

#ifndef NUON_H
#define NUON_H

#include <stdint.h>
#include "hardware/pio.h"
#include "polyface_read.pio.h"
#include "polyface_send.pio.h"
#include "globals.h"
// #include "pico/util/queue.h"

// Define constants
#undef MAX_PLAYERS
#define MAX_PLAYERS       4

// Nuon GPIO pins
#define DATAIO_PIN        2
#define CLKIN_PIN         DATAIO_PIN + 1  // Note - in pins must be a consecutive 'in' group

// Nuon packet start bit type
#define PACKET_TYPE_READ  1
#define PACKET_TYPE_WRITE 0

// Nuon analog modes
#define ATOD_CHANNEL_NONE 0x00
#define ATOD_CHANNEL_MODE 0x01
#define ATOD_CHANNEL_X1 0x02
#define ATOD_CHANNEL_Y1 0x03
#define ATOD_CHANNEL_X2 0x04
#define ATOD_CHANNEL_Y2 0x05

// Nuon controller PROBE options
#define DEFCFG 1
#define VERSION 11
#define TYPE 3
#define MFG 0
#define CRC16 0x8005
#define MAGIC 0x4A554445 // HEX to ASCII == "JUDE" (The Polyface inventor)

// fun
#undef KONAMI_CODE
#define KONAMI_CODE {0x0200, 0x0200, 0x0800, 0x0800, 0x0400, 0x0100, 0x0400, 0x0100, 0x0008, 0x4000}

// Declaration of global variables
PIO pio;
uint sm1, sm2; // sm1 = send; sm2 = read

static int crc_lut[256]; // crc look up table
// queue_t packet_queue;

// Function declarations
void nuon_init(void);
uint32_t __rev(uint32_t);
uint8_t eparity(uint32_t);
int crc_calc(unsigned char data,int crc);
uint32_t crc_data_packet(int32_t value, int8_t size);

void __not_in_flash_func(core1_entry)(void);
void __not_in_flash_func(update_output)(void);
void __not_in_flash_func(post_globals)(uint8_t dev_addr, int8_t instance,
  uint32_t buttons, uint8_t analog_1x, uint8_t analog_1y,
  uint8_t analog_2x, uint8_t analog_2y, uint8_t analog_l,
  uint8_t analog_r, uint32_t keys, uint8_t quad_x);
void __not_in_flash_func(post_mouse_globals)(uint8_t dev_addr, int8_t instance,
  uint16_t buttons, uint8_t delta_x, uint8_t delta_y, uint8_t quad_x);

#endif // NUON_H
