
#include "./definitions.h"

#if MBED_CONF_APP_UROS_AGENT_TRANSPORT == UROS_TRANSPORT_UDP

#include "mbed-trace/mbed_trace.h"
#include "mbed.h"
#include <rmw_microros/rmw_microros.h>

struct MbedUdpTransportArgs
{
    UDPSocket *socket;
    NetworkInterface *network;
    const char *server_address;
    int server_port;
};

// --- micro-ROS Transports - UDP ---
bool mbed_udp_open(struct uxrCustomTransport *transport);
bool mbed_udp_close(struct uxrCustomTransport *transport);
size_t mbed_udp_write(struct uxrCustomTransport *transport, const uint8_t *buf, size_t len, uint8_t *err);
size_t mbed_udp_read(struct uxrCustomTransport *transport, uint8_t *buf, size_t len, int timeout, uint8_t *err);

NetworkInterface *network;
UDPSocket udpSocket;
MbedUdpTransportArgs args;

NetworkInterface *network_init()
{
    tr_info("Connecting to the network... (Thread Id: %d)", ThisThread::get_id());
    auto network = NetworkInterface::get_default_instance();
    if (network == NULL)
    {
        tr_error("No network interface found");
        return NULL;
    }

    auto ret = network->connect();
    if (ret != 0)
    {
        tr_error("Connection error: %x", ret);
        return NULL;
    }

    tr_info("Connection Success");
    tr_info("MAC: %s", network->get_mac_address());
    SocketAddress ip_address;
    network->get_ip_address(&ip_address);
    tr_info("IP: %s", ip_address.get_ip_address());

    return network;
}

extern "C" bool init_transport()
{
    network = network_init();
    if (network == NULL)
    {
        tr_error("network_init failed");
        return false;
    }

    tr_info("opened udp socket sucessfully");

    args.network = network;
    args.socket = &udpSocket;
    args.server_port = MBED_CONF_APP_UROS_AGENT_PORT;
    args.server_address = MBED_CONF_APP_UROS_AGENT_HOST;

    tr_info("Setting the custom udp transport hooks");
    auto ret = rmw_uros_set_custom_transport(
        false,
        (void *)&args,
        mbed_udp_open,
        mbed_udp_close,
        mbed_udp_write,
        mbed_udp_read);

    if (ret != RMW_RET_OK)
    {
        tr_error("setting custom transport failed: %d", (int)ret);
        return false;
    }

    return true;
}

bool mbed_udp_open(struct uxrCustomTransport *transport)
{
    tr_info("mbed_udp_open - begin (Thread Id: %d)", ThisThread::get_id());
    auto args = (MbedUdpTransportArgs *)transport->args;

    auto ret = args->socket->open(args->network);
    if (ret != MBED_SUCCESS)
    {
        tr_error("mbed_udp_open - Socket connect error: %d", ret);
        return false;
    }

    tr_info("mbed_udp_open - udp socket open complete");
    return true;
}

bool mbed_udp_close(struct uxrCustomTransport *transport)
{
    auto args = (MbedUdpTransportArgs *)transport->args;

    auto ret = args->socket->close();
    if (ret != MBED_SUCCESS)
    {
        tr_error("Socket close error: %d", ret);
        return false;
    }

    tr_info("mbed_udp_close - udp socket close complete");
    return true;
}

size_t mbed_udp_write(struct uxrCustomTransport *transport, const uint8_t *buf, size_t len, uint8_t *err)
{
    auto args = (MbedUdpTransportArgs *)transport->args;

    SocketAddress addr(args->server_address, args->server_port);
    auto ret = args->socket->sendto(addr, (void *)buf, len);

    if (ret >= 0)
    {
        *err = 0;
        return (size_t)ret;
    }

    tr_error("mbed_udp_write - error: %d", ret);
    *err = 1;
    return 0;
}

size_t mbed_udp_read(struct uxrCustomTransport *transport, uint8_t *buf, size_t len, int timeout, uint8_t *err)
{
    auto args = (MbedUdpTransportArgs *)transport->args;

    SocketAddress addr(args->server_address, args->server_port);
    args->socket->set_timeout(timeout);
    auto ret = args->socket->recvfrom(&addr, (void *)buf, len);

    if (ret >= 0)
    {
        *err = 0;
        return (size_t)ret;
    }

    if (ret != NSAPI_ERROR_WOULD_BLOCK)
    {
        tr_error("mbed_udp_read - error: %d", ret);
        *err = 1;
    }
    else
    {
        *err = 0;
    }

    return 0;
}

#endif