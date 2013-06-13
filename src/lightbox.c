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


#include <board.h>
#include <trace.h>

#include <string.h>

#include <uip.h>

#include "lightbox.h"



/**
 * Buffers and pointers to store and manipulate led data
 */
// Main memory pool for storing pixel data, we'll have to manage it by
// hand since we don't have malloc.
unsigned char big_buf[SDMX_MAX_BUFFER_SIZE];

// These pointers are for flipping front and back buffers.
unsigned char *front_buf;
unsigned char *back_buf;

// Iterator for the pin toggling
unsigned char *curpx;
unsigned char *cur_back_px;
unsigned int leds_per_chan = 0;
unsigned int buf_size = 0; // This should always be leds_per_chan * 12

int ledstate = 0;

static struct lightbox_state s;

unsigned int *TC1_SVR = AT91C_AIC_SVR + AT91C_ID_TC1;

/**
 * These are used as local variables, but are declared here for added
 * performance
 */
int len;
int i;
int bithalf = 0;
int gapctr = 0;
int recv_frames = 0;
unsigned int discard_frames = 0;


/**
 * Interrupt handlers for flipping the LED pins.
 */
__attribute__ ((section (".ramfunc"))) void second_half();
__attribute__ ((section (".ramfunc")))
void first_half() {
	// Aknowledge interrupt by accessing status register
	/* *AT91C_TC1_SR; */
	--curpx;
	// Set current pixel value to output pins
	/* *AT91C_PIOA_ODSR = ((~(*curpx)) << 16); //Set 4 lowest bits starting at pin A16 */
	/* *AT91C_PIOB_ODSR = ((~(*curpx)) << 23); //Set 4 highest bits starting at pin B27 */
	*AT91C_PIOB_ODSR = ((~(*curpx)) << 27); // Remove this line and uncomment above when we have 8 channels ;)

	*TC1_SVR = (unsigned int) second_half;
}
__attribute__ ((section (".ramfunc")))
void second_half() {
	// Aknowledge interrupt by accessing status register
	/* *AT91C_TC1_SR; */
	*TC1_SVR = (unsigned int) first_half;
	/* *AT91C_PIOA_ODSR = ((*curpx) << 16); */
	/* *AT91C_PIOB_ODSR = ((*curpx) << 23); */
	*AT91C_PIOB_ODSR = ((*curpx) << 27);

	/* if (curpx == front_buf)
	 * 	*AT91C_AIC_IDCR = (1 << AT91C_ID_TC1); */
	//Disable interrupt if we are finished sending the frame
	*AT91C_AIC_IDCR = ((curpx == front_buf) << AT91C_ID_TC1);
}



/**
 * Interrupt handlers for sending the 16 bit pre-gap
 */
__attribute__ ((section (".ramfunc"))) void gap2();
__attribute__ ((section (".ramfunc")))
void gap1() {
	/* *AT91C_TC1_SR; */
	if (gapctr % 2) {
		/* *AT91C_PIOA_CODR = pinsA; */
		*AT91C_PIOB_CODR = pinsB;
	}
	else {
		/* *AT91C_PIOA_SODR = pinsA; */
		*AT91C_PIOB_SODR = pinsB;
	}

	*TC1_SVR = (unsigned int) gap2;
}
__attribute__ ((section (".ramfunc")))
void gap2() {
	/* *AT91C_TC1_SR; */
	if (gapctr % 2) {
		/* *AT91C_PIOA_SODR = pinsA; */
		*AT91C_PIOB_SODR = pinsB;
	}
	else {
		/* *AT91C_PIOA_CODR = pinsA; */
		*AT91C_PIOB_CODR = pinsB;
	}

	--gapctr;
	*TC1_SVR = (unsigned int) (gapctr ? gap1 : first_half);
	/* if (gapctr)
	 * 	*TC1_SVR = (unsigned int) gap1;
	 * else
	 * 	*TC1_SVR = (unsigned int) first_half; */
}



/**
 * Set up the AIC registers for Timer 1 (Timer 0 is used by the uIP stuff)
 */
void irq_setup() {
	// Disable TC1 interrupt while we set things up
	*AT91C_AIC_IDCR = (1 << AT91C_ID_TC1);
	// Set interrupt handler function
	//AT91C_AIC_SVR[AT91C_ID_TC1] = (unsigned int) first_half;
	/* *TC1_SVR = (unsigned int) first_half; */
	// Configure to be rising edge sensitive and highest prio
	AT91C_AIC_SMR[AT91C_ID_TC1] = (AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL |
								   AT91C_AIC_PRIOR_HIGHEST);
	/* AT91C_AIC_SMR[AT91C_ID_TC1] = (AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE |
	 * 							   AT91C_AIC_PRIOR_HIGHEST); */
	//Enable fast forcing (FIQ) for TC1 interrupt
	//*AT91C_AIC_FFER = (1 << AT91C_ID_TC1);
}

__attribute__ ((section (".ramfunc")))
inline void disable_sendbit() {
	*AT91C_AIC_IDCR = (1 << AT91C_ID_TC1);
}

__attribute__ ((section (".ramfunc")))
inline void enable_sendbit() {
	// Clear the interrupt
	*AT91C_AIC_ICCR = (1 << AT91C_ID_TC1);
	// Enable TC1 interrupt
	/* *AT91C_AIC_IDCR = (0 << AT91C_ID_TC1); */
	*AT91C_AIC_IECR = (1 << AT91C_ID_TC1);
}



/**
 * Set up timer 1 interrupt generation interval
 * (using TC1 because Timer 0 is used by the uIP stuff)
 */
void timer_setup() {
	// Enable timer1 clock
	*AT91C_PMC_PCER = (1 << AT91C_ID_TC1);

	/* Don't mess with these, since external code might need it. */
	/* SYNC trigger not used (AT91C_TCB_SYNC) */
	*AT91C_TCB_BCR = 0;
	/* external clocks not used */
	*AT91C_TCB_BMR = (AT91C_TCB_TC0XC0S_NONE | AT91C_TCB_TC1XC1S_NONE |
					  AT91C_TCB_TC2XC2S_NONE);

	// disable all timer1 interrupts
	*AT91C_TC1_IDR = 0xFF;
	// enable the clock	and start it
	*AT91C_TC1_CCR = AT91C_TC_CLKEN | AT91C_TC_SWTRG;
	// Select timer divisor 1 (MCLK / 2)
	*AT91C_TC1_CMR = AT91C_TC_CPCTRG | AT91C_TC_CLKS_TIMER_DIV1_CLOCK;
	// Set value for counter comparison, this should give us about 2us
	*AT91C_TC1_RC = 48;
	// enable RC compare interrupt
	*AT91C_TC1_IER = AT91C_TC_CPCS;
}


/**
 * Example on sending data with uIP / Emac:
 * uip_send(uip_appdata, buflen);
 */



void lightbox_init()
{
	s.state = STATE_OFFLINE;

	// Initialize IO pins
	*AT91C_PIOA_OER = pinsA;
	*AT91C_PIOA_PER = pinsA;
	*AT91C_PIOA_PPUDR = pinsA;
	*AT91C_PIOA_OWER = pinsA;

	*AT91C_PIOB_OER = led | pinsB;
	*AT91C_PIOB_PER = led | pinsB;
	*AT91C_PIOB_PPUDR = led | pinsB;
	*AT91C_PIOB_OWER = led | pinsB;

	/* Turn off LED */
	*AT91C_PIOB_CODR = led;

	irq_setup();
	timer_setup();

	// Start listening
	uip_listen(HTONS(23));
}



__attribute__ ((section (".ramfunc")))
void newdata()
{
	len = uip_datalen();
	unsigned char *data_end = cur_back_px + len;
	unsigned char *backbuf_end = back_buf + buf_size;

	switch (s.state) {
	case STATE_ONLINE:
		if (data_end < backbuf_end) {
			// Data fits in backbuffer, so just copy it
			memcpy(cur_back_px, uip_appdata, len);
			cur_back_px += len;
		}
		else {
			/* we have to fill the backbuffer and handle any extra
			 * data. If this extra data fits on the front buffer, then
			 * we just copy it there, otherwise we have to drop every
			 * frame but the last */
			unsigned int diff = data_end - backbuf_end;
			memcpy(cur_back_px, uip_appdata, len - diff);

			/* Back buffer is full, lets wait for front buffer to be
			 * sent to leds before writing any extra data to it */
			while (*AT91C_AIC_IECR && (1 << AT91C_ID_TC1))
				;

			/* If we received over 2 frames we don't have anywhere to
			 * store them, so drop the earlier frames and copy data
			 * from the most recent frame to the front buffer
			 * (soon-to-be back buffer) */
			discard_frames = 0;
			if (diff > buf_size)
				discard_frames = diff - (diff % buf_size);
			memcpy(front_buf,
				   uip_appdata + (len - diff) + discard_frames,
				   diff - discard_frames);

			TRACE_INFO("backbuf = %d, backbuf_end = %d\n",
					   back_buf, backbuf_end);
			TRACE_INFO("data_end = %d, copied to = %d\n",
					   data_end, cur_back_px + len - diff);
			TRACE_INFO("cur_back_px = %d, diff = %d, leftover = %d\n",
					   cur_back_px, diff, len - diff);
			TRACE_INFO("Discarded %d bytes\n", discard_frames);

			recv_frames++;
			TRACE_INFO("F=%d, %db\n\n\n", recv_frames,
					   cur_back_px + len - diff - back_buf);

			ledstate = !ledstate;
			*AT91C_PIOB_ODSR |= (ledstate << 23);

			/* It's time to switch buffers and send the next frame ;) */
			cur_back_px = front_buf;
			curpx = back_buf + buf_size;
			front_buf = back_buf;
			back_buf = cur_back_px;
			cur_back_px += (diff - discard_frames);
			gapctr = 16;
			*TC1_SVR = (unsigned int) gap1;
			enable_sendbit();
		}
		break;
	case STATE_INIT:
		leds_per_chan = *((int *)uip_appdata);
		buf_size = leds_per_chan * 12;
		front_buf = big_buf;
		back_buf = big_buf + buf_size;
		curpx = back_buf; // front buffer is read backwards ;)
		cur_back_px = back_buf;
		s.state = STATE_ONLINE;
		TRACE_INFO("Initialized new connection with %d LEDs per channel.\n", leds_per_chan);
		break;
	default:
		TRACE_WARNING("Received data packets when no one was connected... \
WTF?!?!?\n");
	}
}



/**
 * uIP event callback. This is called by uIP everytime something
 * happens on the network.
 */
__attribute__ ((section (".ramfunc")))
void net_callback()
{
	if (uip_newdata()) {
		/* We enter this when new data packets are received on an
		 * existing connection. */
		newdata();
	}
	else if (uip_connected()) {
		/*	  tcp_markconn(uip_conn, &s);*/
		/* Switch state to initializing (STATE_INIT) */
		if (s.state == STATE_OFFLINE) {
			s.state = STATE_INIT;
			TRACE_INFO("New connection initiated!\n\n");
		}
		else {
			TRACE_WARNING("Got a new connection, while an older one was still \
open... Maybe we should close the old one? :S\n");
		}
	}
	else if (uip_closed() || uip_aborted() || uip_timedout()) {
		/* TODO: Cleanup and close stuff on our side. */
		recv_frames = 0;
		TRACE_INFO("Connection closed...\n");
		switch (s.state) {
		case STATE_INIT:
		case STATE_ONLINE:
			disable_sendbit();
			s.state = STATE_OFFLINE;
			break;
		}
	}
}
