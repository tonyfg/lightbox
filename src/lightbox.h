/**
 *    Copyright (c) 2011, 2013 - Tony Gonçalves.
 *    All rights reserved.
 *
 *    This file is part of Lightbox.
 *
 *    Lightbox is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Lightbox is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with Lightbox.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __LIGHTBOX_H__
#define __LIGHTBOX_H__

#include "uipopt.h"

#define STATE_OFFLINE 0
#define STATE_INIT 1
#define STATE_ONLINE 2

// Function that uIP calls back when new data is received
#define UIP_APPCALL net_callback

// LED and pin masks
#define led   0x00800000 // (1 << 23), corresponds to PB23
#define pinsA 0x000f0000 // (1 << 16 | 1 << 17 | 1 << 18 | 1 << 19)
#define pinsB 0x78000000 // (1 << 27 | 1 << 28 | 1 << 29 | 1 << 30)

/**
 * Data buffer = 1024 leds(max) on each channel, 12 bits per led (*8
 * for all channels), 2 buffers
 */
#define SDMX_MAX_BUFFER_SIZE (1024 * 12 * 2)



struct lightbox_state {
  int state;
};
typedef struct lightbox_state uip_tcp_appstate_t;



void lightbox_init();
void net_callback();

#endif
