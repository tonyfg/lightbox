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


#include "uip.h"
#include "uip_arp.h"
#include "tapdev.h"
#include "timer.h"

//-----------------------------------------------------------------------------
//         Local Define
//-----------------------------------------------------------------------------

/// uIP buffer : The ETH header
#define BUF ((struct uip_eth_hdr *)&uip_buf[0])



struct timer periodic_timer, arp_timer;



//-----------------------------------------------------------------------------
//         Global functions
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/// Global function for uIP to use
/// \param m Pointer to string that logged
//-----------------------------------------------------------------------------
inline void uip_log(char *m)
{
    TRACE_DEBUG("-uIP- %s\n\r", m);
}

#ifdef __DHCPC_H__
//-----------------------------------------------------------------------------
/// Global function for uIP DHCPC to use, notification of DHCP configuration
/// \param s Pointer to DHCP state instance
//-----------------------------------------------------------------------------
void dhcpc_configured(const struct dhcpc_state *s)
{
    u8_t * pAddr;

    printf("\n\r");
    printf("=== DHCP Configurations ===\n\r");
    pAddr = (u8_t *)s->ipaddr;
    printf("- IP   : %d.%d.%d.%d\n\r",
        pAddr[0], pAddr[1], pAddr[2], pAddr[3]);
    pAddr = (u8_t *)s->netmask;
    printf("- Mask : %d.%d.%d.%d\n\r",
        pAddr[0], pAddr[1], pAddr[2], pAddr[3]);
    pAddr = (u8_t *)s->default_router;
    printf("- GW   : %d.%d.%d.%d\n\r",
        pAddr[0], pAddr[1], pAddr[2], pAddr[3]);
    pAddr = (u8_t *)s->dnsaddr;
    printf("- DNS  : %d.%d.%d.%d\n\r",
        pAddr[0], pAddr[1], pAddr[2], pAddr[3]);
    printf("===========================\n\r\n");
    uip_sethostaddr(s->ipaddr);
    uip_setnetmask(s->netmask);
    uip_setdraddr(s->default_router);

  #ifdef __RESOLV_H__
    resolv_conf(s->dnsaddr);
  #else
    printf("DNS NOT enabled in the demo\n\r");
  #endif
}
#endif /* __DHCPC_H__ */

//-----------------------------------------------------------------------------
/// Initialize demo application
//-----------------------------------------------------------------------------
static void app_init(void)
{
	lightbox_init();

#ifdef __DHCPC_H__
    printf("P: DHCPC Init\n\r");
    dhcpc_init(MacAddress.addr, 6);
#endif

	TRACE_INFO("Lightbox initialized!");
}



__attribute__ ((section (".ramfunc")))
void netloop() {
	unsigned int i;

    while(1) {
        uip_len = tapdev_read();
        if(uip_len > 0) {
            if(BUF->type == htons(UIP_ETHTYPE_IP)) {
                uip_arp_ipin();
                uip_input();
                /* If the above function invocation resulted in data that
                should be sent out on the network, the global variable
                uip_len is set to a value > 0. */
                if(uip_len > 0) {
                    uip_arp_out();
                    tapdev_send();
                }
            } else if(BUF->type == htons(UIP_ETHTYPE_ARP)) {
                uip_arp_arpin();
                /* If the above function invocation resulted in data that
                should be sent out on the network, the global variable
                uip_len is set to a value > 0. */
                if(uip_len > 0) {
                    tapdev_send();
                }
            }
        } else if(timer_expired(&periodic_timer)) {
            timer_reset(&periodic_timer);
            for(i = 0; i < UIP_CONNS; i++) {
                uip_periodic(i);
                /* If the above function invocation resulted in data that
                   should be sent out on the network, the global variable
                   uip_len is set to a value > 0. */
                if(uip_len > 0) {
                  uip_arp_out();
                  tapdev_send();
                }
            }
#if UIP_UDP
            for(i = 0; i < UIP_UDP_CONNS; i++) {
                uip_udp_periodic(i);
                /* If the above function invocation resulted in data that
                   should be sent out on the network, the global variable
                   uip_len is set to a value > 0. */
                if(uip_len > 0) {
                    uip_arp_out();
                    tapdev_send();
                }
            }
#endif /* UIP_UDP */

            /* Call the ARP timer function every 10 seconds. */
            if(timer_expired(&arp_timer)) {
                timer_reset(&arp_timer);
                uip_arp_timer();
            }
        }
    }
}



__attribute__ ((section (".ramfunc")))
int main()
{
    uip_ipaddr_t ipaddr;

	//init_pio();

    // System devices initialize
    tapdev_init();
    clock_init();
    timer_set(&periodic_timer, CLOCK_SECOND / 2);
    timer_set(&arp_timer, CLOCK_SECOND * 10);

    // Init uIP
    uip_init();

#ifndef __DHCPC_H__
    // Set the IP address of this host
    uip_ipaddr(ipaddr, HostIpAddress[0], HostIpAddress[1],
                       HostIpAddress[2], HostIpAddress[3]);
    uip_sethostaddr(ipaddr);

    uip_ipaddr(ipaddr, RoutIpAddress[0], RoutIpAddress[1],
                       RoutIpAddress[2], RoutIpAddress[3]);
    uip_setdraddr(ipaddr);

    uip_ipaddr(ipaddr, NetMask[0], NetMask[1], NetMask[2], NetMask[3]);
    uip_setnetmask(ipaddr);
#else
    printf("P: DHCP Supported\n\r");
    uip_ipaddr(ipaddr, 0, 0, 0, 0);
    uip_sethostaddr(ipaddr);
    uip_ipaddr(ipaddr, 0, 0, 0, 0);
    uip_setdraddr(ipaddr);
    uip_ipaddr(ipaddr, 0, 0, 0, 0);
    uip_setnetmask(ipaddr);
#endif

    uip_setethaddr(MacAddress);

    app_init();

	netloop();

    return 0;
}


