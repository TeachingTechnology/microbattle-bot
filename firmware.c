/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/pbuf.h"
#include "lwip/udp.h"

#define DEBUG_printf printf
#define DEBUG_fwrite writef

#define UDP_PORT 12345

void receive(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

struct State {
	float x;
	float y;
	float b;
};

struct State state = {0, 0, 0};

int main() {
	#define CHECK(c, ...) if (c) { DEBUG_printf(__VA_ARGS__); return -1; }

    stdio_init_all();

	CHECK(cyw43_arch_init(), "Failed to initialise cyw43.\n");

	cyw43_arch_enable_sta_mode();

	DEBUG_printf("Connecting to wifi... ");

	CHECK(
		cyw43_arch_wifi_connect_timeout_ms(
			WIFI_SSID,
			WIFI_PASSWORD,
			CYW43_AUTH_WPA2_AES_PSK,
			30000
		),
		"FAILED\n"
	);
	DEBUG_printf("connected\n");

	{ // Setup udp connection and receive callback
		DEBUG_printf("Starting server on %s:%d.\n", ip4addr_ntoa(netif_ip4_addr(netif_default)), UDP_PORT);

		struct udp_pcb *pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
		CHECK(pcb == NULL, "Failed to create UDP PCB.\n");

		err_t err = udp_bind(pcb, IP_ANY_TYPE, UDP_PORT);
		CHECK(err, "Failed to bind to port %d\n", UDP_PORT);

		udp_recv(pcb, receive, NULL);
	}

    while (true) {
		cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    }
}

void receive(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
	if (p->len != sizeof(struct State)) {
		DEBUG_printf("ERROR: received packet of invalid length (%d)\n", p->len);
		return;
	}

	memcpy(&state, p->payload, sizeof(struct State));

	pbuf_free(p);

	DEBUG_printf("X[%f] Y[%f] B[%f]\n", state.x, state.y, state.b);
}

