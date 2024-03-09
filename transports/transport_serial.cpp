#include "./definitions.h"

#if MBED_CONF_APP_UROS_AGENT_TRANSPORT == UROS_TRANSPORT_SERIAL

#include "mbed-trace/mbed_trace.h"
#include "mbed.h"
#include <rmw_microros/rmw_microros.h>

// --- micro-ROS Transports - SERIAL ---
bool mbed_serial_open(struct uxrCustomTransport *transport);
bool mbed_serial_close(struct uxrCustomTransport *transport);
size_t mbed_serial_write(struct uxrCustomTransport *transport, const uint8_t *buf, size_t len, uint8_t *err);
size_t mbed_serial_read(struct uxrCustomTransport *transport, uint8_t *buf, size_t len, int timeout, uint8_t *err);

extern "C" bool init_transport()
{
    tr_info("Setting the custom serial transport hooks");

    auto ret = rmw_uros_set_custom_transport(
        true,
        NULL,
        mbed_serial_open,
        mbed_serial_close,
        mbed_serial_write,
        mbed_serial_read);

    if (ret != RMW_RET_OK)
    {
        tr_error("setting custom transport failed: %d", (int)ret);
        return false;
    }

    return true;
}

// --- micro-ROS Transports - Serial ---
static UnbufferedSerial serial_port(USBTX, USBRX);
Timer t;

#define UART_DMA_BUFFER_SIZE 2048

static uint8_t dma_buffer[UART_DMA_BUFFER_SIZE];
static size_t dma_head = 0, dma_tail = 0;

void on_rx_interrupt()
{
    if (serial_port.read(&dma_buffer[dma_tail], 1))
    {
        dma_tail = (dma_tail + 1) % UART_DMA_BUFFER_SIZE;
        if (dma_tail == dma_head)
        {
            dma_head = (dma_head + 1) % UART_DMA_BUFFER_SIZE;
        }
    }
}

bool mbed_serial_open(struct uxrCustomTransport *transport)
{
    serial_port.baud(115200);
    serial_port.format(
        /* bits */ 8,
        /* parity */ BufferedSerial::None,
        /* stop bit */ 1);
    serial_port.attach(&on_rx_interrupt, SerialBase::RxIrq);

    return true;
}

bool mbed_serial_close(struct uxrCustomTransport *transport)
{
    return (serial_port.close() == 0 ? true : false);
}

size_t mbed_serial_write(struct uxrCustomTransport *transport, const uint8_t *buf, size_t len, uint8_t *err)
{
    ssize_t wrote = serial_port.write(buf, len);
    serial_port.sync();
    return (wrote >= 0) ? wrote : 0;
}

size_t mbed_serial_read(struct uxrCustomTransport *transport, uint8_t *buf, size_t len, int timeout, uint8_t *err)
{
    t.start();
    std::chrono::microseconds elapsed = std::chrono::microseconds(0);
    std::chrono::microseconds timeout_us = std::chrono::microseconds(timeout * 1000);

    size_t wrote = 0;
    while (elapsed < timeout_us && wrote < len)
    {
        if (dma_head != dma_tail)
        {
            buf[wrote] = dma_buffer[dma_head];
            dma_head = (dma_head + 1) % UART_DMA_BUFFER_SIZE;
            wrote++;
        }
        elapsed = t.elapsed_time();
    }

    return wrote;
}

#endif
