TLS Persistent Session Cache
############################

Overview
********

This sample demonstrates TLS session resumption across device reboots
using Zephyr's NVS (Non-Volatile Storage) backed session cache.

When ``CONFIG_NET_SOCKETS_TLS_SESSION_CACHE_PERSISTENT`` is enabled,
TLS client sessions are saved to flash after each successful handshake
and restored during system initialization. This allows the device to
perform an abbreviated TLS handshake (session resumption) when
reconnecting to a known server after a reboot.

Requirements
************

- A board with flash storage and a ``storage_partition`` defined in DTS
- A TLS server to connect to
- Network connectivity

Configuration
*************

The key Kconfig options for this feature:

- ``CONFIG_NET_SOCKETS_TLS_SESSION_CACHE_PERSISTENT`` — enables NVS persistence
- ``CONFIG_NET_SOCKETS_TLS_SESSION_CACHE_NVS_SECTOR_COUNT`` — NVS sectors (default: 2)
- ``CONFIG_NET_SOCKETS_TLS_MAX_CLIENT_SESSION_COUNT`` — max cached sessions (default: 1)

Building and Running
********************

.. code-block:: console

   west build -b <board> samples/net/sockets/tls_session_cache
   west flash

Testing Session Resumption
**************************

1. Flash and boot the device — observe full TLS handshake in debug logs
2. Reboot the device
3. On second boot, the TLS session is restored from NVS flash
4. The handshake is abbreviated (session ID/ticket based resumption)

Look for these log messages:

- ``TLS session NVS initialized`` — NVS backend ready
- ``TLS session 0 saved to NVS`` — session persisted after handshake
- ``TLS session 0 restored from NVS`` — session loaded on boot
