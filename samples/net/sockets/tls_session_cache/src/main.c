/*
 * Copyright (c) 2026 Rithic Chellaram Hariharan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * TLS Persistent Session Cache Example
 *
 * Demonstrates TLS session resumption across reboots using NVS.
 * On first boot, a full TLS handshake is performed. On subsequent
 * boots, the cached session is restored from flash and the handshake
 * is abbreviated (session resumption).
 *
 * To test:
 *   1. Build and flash with CONFIG_NET_SOCKETS_TLS_SESSION_CACHE_PERSISTENT=y
 *   2. First boot: observe "Full TLS handshake" in logs
 *   3. Reboot the device
 *   4. Second boot: observe "TLS session resumed" in logs
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>

LOG_MODULE_REGISTER(tls_session_cache_sample, LOG_LEVEL_DBG);

#define SERVER_ADDR  "192.0.2.1"
#define SERVER_PORT  4433
#define CA_TAG       1

/* A sample CA certificate - replace with your own */
static const unsigned char ca_certificate[] =
	"-----BEGIN CERTIFICATE-----\n"
	"Replace with your CA certificate\n"
	"-----END CERTIFICATE-----\n";

static int setup_tls_credential(void)
{
	int err;

	err = tls_credential_add(CA_TAG,
				 TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca_certificate,
				 sizeof(ca_certificate));
	if (err < 0) {
		LOG_ERR("Failed to add CA certificate: %d", err);
		return err;
	}

	return 0;
}

static int connect_to_server(void)
{
	int sock;
	int ret;
	struct sockaddr_in addr;
	sec_tag_t sec_tag_list[] = { CA_TAG };
	int session_cache = ZSOCK_TLS_SESSION_CACHE_ENABLED;

	sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);
	if (sock < 0) {
		LOG_ERR("Failed to create socket: %d", errno);
		return -errno;
	}

	/* Set TLS security tag */
	ret = zsock_setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST,
			       sec_tag_list, sizeof(sec_tag_list));
	if (ret < 0) {
		LOG_ERR("Failed to set TLS_SEC_TAG_LIST: %d", errno);
		goto cleanup;
	}

	/* Enable session caching - this is what triggers save/restore */
	ret = zsock_setsockopt(sock, SOL_TLS, TLS_SESSION_CACHE,
			       &session_cache, sizeof(session_cache));
	if (ret < 0) {
		LOG_ERR("Failed to set TLS_SESSION_CACHE: %d", errno);
		goto cleanup;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	zsock_inet_pton(AF_INET, SERVER_ADDR, &addr.sin_addr);

	LOG_INF("Connecting to %s:%d ...", SERVER_ADDR, SERVER_PORT);

	/*
	 * On connect():
	 *   1. tls_session_restore() checks NVS-backed RAM cache
	 *      for a saved session matching this peer address
	 *   2. If found, mbedTLS offers it in ClientHello (abbreviated)
	 *   3. After handshake, tls_session_store() saves the new
	 *      session to both RAM cache and NVS flash
	 */
	ret = zsock_connect(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		LOG_ERR("Failed to connect: %d", errno);
		goto cleanup;
	}

	LOG_INF("TLS connection established");

	/* Send a test message */
	const char *msg = "Hello from persistent session cache sample!\n";

	ret = zsock_send(sock, msg, strlen(msg), 0);
	if (ret < 0) {
		LOG_ERR("Failed to send: %d", errno);
	} else {
		LOG_INF("Sent %d bytes", ret);
	}

	/* Receive response */
	char buf[128];

	ret = zsock_recv(sock, buf, sizeof(buf) - 1, 0);
	if (ret > 0) {
		buf[ret] = '\0';
		LOG_INF("Received: %s", buf);
	}

cleanup:
	zsock_close(sock);
	return ret < 0 ? ret : 0;
}

int main(void)
{
	LOG_INF("TLS Persistent Session Cache Sample");
	LOG_INF("Session cache is %s",
		IS_ENABLED(CONFIG_NET_SOCKETS_TLS_SESSION_CACHE_PERSISTENT)
		? "persistent (NVS)" : "RAM-only");

	if (setup_tls_credential() < 0) {
		return 0;
	}

	/* Connect - first time does full handshake, after reboot resumes */
	connect_to_server();

	LOG_INF("Done. Reboot to test session resumption.");

	return 0;
}
