/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#include "lwip/pbuf.h"
#include "lwip/udp.h"

#include "dhcpserver.h"

#define DEBUG_printf printf
#define DEBUG_fwrite writef

#define UDP_PORT 12345

#define PIN_X 10
#define PIN_Y 11
#define PIN_B 12

#define PWM_FREQ 50
#define PWM_RES 65536 // 2 ^ 16
#define PWM_FRAC 16   // 2 ^ 4

#define SSID_LEN (2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1)

void receive(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

struct State {
	float x;
	float y;
	float b;
};

struct State state = {0, 0, 0};
uint32_t pwm_wrap = 0;
dhcp_server_t dhcp_server;

int main() {
	#define CHECK(c, ...) if (c) { DEBUG_printf(__VA_ARGS__); return -1; }

    stdio_init_all();
	clocks_init();

	// delay to allow for serial terminal connection
	sleep_ms(2500);

	{ // PWM setup
		uint32_t sys_freq = clock_get_hz(clk_sys);
		DEBUG_printf("clk_sys: %d\n", sys_freq);

		// calculate clock division values
		uint32_t fp_divider = ((sys_freq - 1) / PWM_FREQ / (PWM_RES / PWM_FRAC)) + 1;
		fp_divider = fp_divider < 16 ? 16 : fp_divider; // clamp to minimum
		uint32_t int_divider = fp_divider / PWM_FRAC;
		uint32_t frac_divider = fp_divider % PWM_FRAC;

		DEBUG_printf("PWM division is %d and %d/16ths.\n", int_divider, frac_divider);

		// calculate the pwm wrap value, needed for duty calculation
		pwm_wrap = sys_freq * PWM_FRAC / fp_divider / PWM_FREQ - 1;

		DEBUG_printf("PWM wrap is %d.\n", pwm_wrap);

		// setup PWM output
		uint pins[] = {PIN_X, PIN_Y, PIN_B};
		for (size_t i = 0; i != sizeof(pins) / sizeof(*pins); ++i) {
			gpio_set_function(pins[i], GPIO_FUNC_PWM);
			uint slice = pwm_gpio_to_slice_num(pins[i]);
			uint channel = pwm_gpio_to_channel(pins[i]);
			pwm_set_clkdiv_int_frac(slice, int_divider, frac_divider);
			pwm_set_wrap(slice, pwm_wrap);
			pwm_set_chan_level(slice, channel, pwm_wrap / 2);
			pwm_set_enabled(slice, true);
			DEBUG_printf("Setup PWM pin %d for 50%% duty cycle at 50 Hz.\n", pins[i]);
		}

		DEBUG_printf("PWM setup complete.\n"); 
	}

	CHECK(cyw43_arch_init(), "Failed to initialise cyw43.\n");

	{ // wifi ap mode setup
		char ssid[SSID_LEN];
		char password[] = "password";
		pico_get_unique_board_id_string(ssid, SSID_LEN);
		cyw43_arch_enable_ap_mode(ssid, password, CYW43_AUTH_WPA2_AES_PSK);
		DEBUG_printf("WIFI AP setup at with SSID: %s\n", ssid);
	}

	{ // setup DHCP
		ip_addr_t mask;
		IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);
		dhcp_server_init(&dhcp_server, &netif_default->ip_addr, &mask);
	}

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

	pwm_set_gpio_level(PIN_X, (uint16_t)((state.x + 1) / 2 * pwm_wrap));
	pwm_set_gpio_level(PIN_Y, (uint16_t)((state.y + 1) / 2 * pwm_wrap));
	pwm_set_gpio_level(PIN_B, (uint16_t)(state.b * pwm_wrap));

	DEBUG_printf("X[%f] Y[%f] B[%f]\n", state.x, state.y, state.b);
}

