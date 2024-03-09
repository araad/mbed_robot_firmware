#include "mbed.h"

#define BUFFER_SIZE 10

class MegaPiProxy
{
private:
    BufferedSerial serial_port;
    uint8_t buffer[BUFFER_SIZE];
    bool is_open = false;

public:
    MegaPiProxy(PinName tx, PinName rx) : serial_port(tx, rx, 115200)
    {
        is_open = true;
    }

    void encoder_motors_run(int16_t left_speed, int16_t right_speed)
    {
        buffer[0] = 0xff;
        buffer[1] = 0x55;
        buffer[2] = 7;
        buffer[3] = 0;
        buffer[4] = 2;
        buffer[5] = 5;
        buffer[6] = left_speed & 0xFF;
        buffer[7] = (left_speed >> 8) & 0xFF;
        buffer[8] = right_speed & 0xFF;
        buffer[9] = (right_speed >> 8) & 0xFF;

        serial_port.write(buffer, BUFFER_SIZE);
    }
};
