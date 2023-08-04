/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#define STR_SIZE(s) (sizeof(s) - 1)

#define DEBUG_printf (void)
#define DEBUG_fwrite (void)

#define TCP_PORT 8080
#define POLL_TIME_S 5

#include "index_html.h"
#include "index_js.h"
#include "index_css.h"

struct State {
	double x;
	double y;
	int    b;
};

// request
const char s_get[]  = "GET /";
const char s_root[] = " ";
const char s_html[] = "index.html ";
const char s_js[]   = "index.js ";
const char s_css[]  = "index.css ";
const char s_state[] = "state/";

// response
const char s_ok[]      = "HTTP/1.1 200 OK\r\n\r\n";
const char s_ok_html[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n";
const char s_ok_js[]   = "HTTP/1.1 200 OK\r\nContent-Type: text/javascript; charset=utf-8\r\n\r\n";
const char s_ok_css[]  = "HTTP/1.1 200 OK\r\nContent-Type: text/css; charset=utf-8\r\n\r\n";

err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err);

bool connected = false;
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

	{ // Open TCP server
		DEBUG_printf("Starting server on %s:%d.\n", ip4addr_ntoa(netif_ip4_addr(netif_default)), TCP_PORT);

		struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
		CHECK(pcb == NULL, "Failed to create TCP PCB.\n");

		err_t err = tcp_bind(pcb, IP_ANY_TYPE, TCP_PORT);
		CHECK(err, "Failed to bind to port %d\n", TCP_PORT);

		struct tcp_pcb *server_pcb = tcp_listen_with_backlog(pcb, 1);
		CHECK(server_pcb == NULL, "Failed to listen.\n");

		tcp_accept(server_pcb, tcp_server_accept);
	}

    while (true) {
		cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    }
}

err_t tcp_close_client_connection(struct tcp_pcb *client_pcb, err_t close_err) {
	connected = false;
	tcp_poll(client_pcb, NULL, 0);
	tcp_recv(client_pcb, NULL);
	tcp_err(client_pcb, NULL);
	err_t err = tcp_close(client_pcb);
	if (err != ERR_OK) {
		DEBUG_printf("Close failed with %d, calling abort.\n", err);
		tcp_abort(client_pcb);
		close_err = ERR_ABRT;
	}
    return close_err;
}

static err_t tcp_server_poll(void *arg, struct tcp_pcb *pcb) {
	(void)arg;
    DEBUG_printf("TCP server poll.\n");
    return tcp_close_client_connection(pcb, ERR_OK); // Just disconnect clent?
}

static void tcp_server_err(void *arg, err_t err) {
	(void)arg;
	DEBUG_printf("TCP server error: %d\n", err);
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
	(void) arg;

    if (p == NULL) {
        DEBUG_printf("Connection closed.\n");
        return tcp_close_client_connection(pcb, ERR_OK);
    }

	// Don't handle packets that are split over multiple pbufs
	if (p->len != p->tot_len) {
		DEBUG_printf("Packet too large (%d), err %d.\n", p->tot_len, err);
		return tcp_close_client_connection(pcb, ERR_MEM);
	} else {
		DEBUG_printf("TCP server received %d bytes, err %d.\n", p->tot_len, err);
	}

	// Debug print request
	// DEBUG_fwrite(p->payload, 1, p->len, stdout);

	char *curr = p->payload;
	size_t curr_len = p->len;

	// Check if we have a GET request
	if (curr_len < STR_SIZE(s_get) || strncmp(curr, s_get, STR_SIZE(s_get)) != 0) {
		DEBUG_printf("Cannot handle request: not a GET request.\n");
		return tcp_close_client_connection(pcb, ERR_VAL);
	}
	curr += STR_SIZE(s_get);
	curr_len -= STR_SIZE(s_get);

	// Check if we can handle the request
	const char *header = NULL;
	size_t header_len = 0;
	const char *file = NULL;
	size_t file_len = 0;
	if (curr_len >= STR_SIZE(s_root) && strncmp(curr, s_root, STR_SIZE(s_root)) == 0) {
		header = s_ok_html;
		header_len = STR_SIZE(s_ok_html);
		file = index_html;
		file_len = index_html_len;
	} else if (curr_len >= STR_SIZE(s_html) && strncmp(curr, s_html, STR_SIZE(s_html)) == 0) {
		header = s_ok_html;
		header_len = STR_SIZE(s_ok_html);
		file = index_html;
		file_len = index_html_len;
	} else if (curr_len >= STR_SIZE(s_js) && strncmp(curr, s_js, STR_SIZE(s_js)) == 0) {
		header = s_ok_js;
		header_len = STR_SIZE(s_ok_js);
		file = index_js;
		file_len = index_js_len;
	} else if (curr_len >= STR_SIZE(s_css) && strncmp(curr, s_css, STR_SIZE(s_css)) == 0) {
		header = s_ok_css;
		header_len = STR_SIZE(s_ok_css);
		file = index_css;
		file_len = index_css_len;
	} else if (curr_len >= STR_SIZE(s_state) && strncmp(curr, s_state, STR_SIZE(s_state)) == 0) {
		curr += STR_SIZE(s_state);
		curr_len -= STR_SIZE(s_state);
		header = s_ok;
		header_len = STR_SIZE(s_ok);
		file = "";
		int ret = sscanf(curr, "%lf/%lf/%i ", &state.x, &state.y, &state.b);
		if (ret == EOF || ret != 3) {
			DEBUG_printf("Could not parse state values.\n");
			return tcp_close_client_connection(pcb, ERR_VAL);
		} else {
			printf("x[%lf] y[%lf] b[%i]\n", state.x, state.y, state.b);
		}
	} else {
		DEBUG_printf("TCP cannot handle request.\n");
		return tcp_close_client_connection(pcb, ERR_VAL);
	}

	// let network stack know that we've processed the request
	tcp_recved(pcb, p->tot_len);
    pbuf_free(p);

	// write header
	err = tcp_write(pcb, header, header_len, 0);
	if (err != ERR_OK) {
		DEBUG_printf("failed to write response header, %d\n", err);
		return tcp_close_client_connection(pcb, err);
	}

	// write file
	err = tcp_write(pcb, file, file_len, 0);
	if (err != ERR_OK) {
		DEBUG_printf("failed to write response body, %d\n", err);
		return tcp_close_client_connection(pcb, err);
	}

    return tcp_close_client_connection(pcb, ERR_OK);
}

err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
	(void)arg;

    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("Failure in accept.\n");
        return ERR_VAL;
    }
    DEBUG_printf("Client connected.\n");

	if (connected) {
		DEBUG_printf("Cannot handle more than 1 connection currently.\n");
		return ERR_MEM;
	}
	connected = true;

    // setup connection to client
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcp_server_err);

    return ERR_OK;
}

