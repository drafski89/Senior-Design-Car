#include "ArduinoPWM.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <exception>
#include <stdexcept>

using namespace std;

void* send_mesg_loop(void* arduino_pwm_ptr)
{
    ArduinoPWM* arduino_pwm = (ArduinoPWM*)arduino_pwm_ptr;
    bool running = arduino_pwm->running;

    while (running)
    {
        pthread_mutex_lock(&arduino_pwm->write_lock);

        if (!arduino_pwm->mesg_queue.empty())
        {
            struct PWMMesg mesg = arduino_pwm->mesg_queue.front();
            arduino_pwm->mesg_queue.pop();

            pthread_mutex_unlock(&arduino_pwm->write_lock);

            if (write(arduino_pwm->uart_gateway, (void*)&mesg.header, sizeof(char)) == -1)
            {
                printf("Error: ArduinoPWM::send_mesg_loop(): failed to send message header: %d\n", errno);
            }
            else if (write(arduino_pwm->uart_gateway, (void*)mesg.packets, (int)mesg.packets_num) == -1)
            {
                printf("Error: ArduinoPWM::send_mesg_loop(): failed to send message body: %d\n", errno);
            }
            else if (write(arduino_pwm->uart_gateway, (void*)&mesg.checksum, sizeof(char)) == -1)
            {
                printf("Error: ArduinoPWM::send_mesg_loop(): failed to send message checksum: %d\n", errno);
            }
        }
        else
        {
            pthread_mutex_unlock(&arduino_pwm->write_lock);
        }

        // sleep until new data arrives
        pthread_cond_wait(&arduino_pwm->new_mesg_sig, &arduino_pwm->new_mesg_lock);

        // could have been waiting for a while. check for exit condition
        pthread_mutex_lock(&arduino_pwm->write_lock);
        running = arduino_pwm->running;
        pthread_mutex_unlock(&arduino_pwm->write_lock);
    }

    return NULL;
}

ArduinoPWM::ArduinoPWM()
{
    uart_gateway = open("/dev/ttyUSB0", O_RDWR);

    if (uart_gateway == -1)
    {
        printf("Error: ArduinoPWM(): open(): failed to open serial line: %d\n", errno);
        uart_gateway = 0;
        throw runtime_error("open(): failed to open serial line");
    }

    if (tcgetattr(uart_gateway, &uart_props) == -1)
    {
        printf("Error: ArduinoPWM(): tcgetattr(): failed to get serial line attributes: %d\n", errno);
        close(uart_gateway);
        throw runtime_error("tcgetattr(): failed to get serial line attributes");
    }

    if (cfsetspeed(&uart_props, B9600) == -1)
    {
        printf("Error: ArduinoPWM(): cfsetspeed(): failed to set serial line speed: %d\n", errno);
        close(uart_gateway);
        throw runtime_error("cfsetspeed(): failed to set serial line speed");
    }

    cfmakeraw(&uart_props);

    if (pthread_mutex_init(&write_lock, NULL) == -1)
    {
        printf("Error: ArduinoPWM(): pthread_mutex_init(): failed to initialize ArduinoPWM::write_lock mutex: %d\n", errno);
        close(uart_gateway);
        throw runtime_error("pthread_mutex_init(): failed to initialize ArduinoPWM::write_lock mutex");
    }

    if (pthread_mutex_init(&new_mesg_lock, NULL) == -1)
    {
        printf("Error: ArduinoPWM(): pthread_mutex_init(): failed to initialize ArduinoPWM::new_mesg_lock mutex: %d\n", errno);
        close(uart_gateway);
        throw runtime_error("pthread_mutex_init(): failed to initialize ArduinoPWM::new_mesg_lock mutex");
    }

    if (pthread_cond_init(&new_mesg_sig, NULL) == -1)
    {
        printf("Error: ArduinoPWM(): pthread_cond_init(): failed to initialize ArduinoPWM::new_mesg_sig condition: %d\n", errno);
        close(uart_gateway);
        throw runtime_error("pthread_cond_init(): failed to initialize ArduinoPWM::new_mesg_sig condition");
    }

    running = true;

    if (pthread_create(&send_mesg_thread, NULL, &send_mesg_loop, (void*)this) == -1)
    {
        printf("Error: ArduinoPWM(): pthread_create(): failed to create background transfer thread: %d\n", errno);
        close(uart_gateway);
        throw runtime_error("pthread_create(): failed to create background transfer thread");
    }
}

ArduinoPWM::~ArduinoPWM()
{
    if (send_mesg_thread)
    {
        pthread_mutex_lock(&write_lock);
        running = false;
        pthread_mutex_unlock(&write_lock);

        pthread_cond_signal(&new_mesg_sig);

        pthread_join(send_mesg_thread, NULL);

        pthread_cond_destroy(&new_mesg_sig);
        pthread_mutex_destroy(&write_lock);
        pthread_mutex_destroy(&new_mesg_lock);
    }

    if (uart_gateway)
    {
        close(uart_gateway);
    }
}

int ArduinoPWM::buf_to_mesg(struct PWMMesg& mesg, int address, void* buf, int size)
{
    if (address >= MAX_ADDR || address < 0)
    {
        printf("Error: ArduinoPWM.buf_to_mesg(): address %d is out of range\n", address);
        return -1;
    }

    if (size >= MAX_MESG_SIZE || size < 1)
    {
        printf("Error: ArduinoPWM.buf_to_mesg(): message size of %d bytes is out of range\n", size);
        return -1;
    }

    mesg.address = (unsigned char)address;
    mesg.bytes = (unsigned char)size - 1;
    mesg.packets_num = (unsigned char)(size * 8 / 7);

    if (size * 8 % 7 > 0)
    {
        mesg.packets_num += 1;
    }

    unsigned char* byte_buf = (unsigned char*)buf;

    // translate the 8-bit per byte stream into 7-bit per byte stream
    for (int index = 0; index < size; index++)
    {
        unsigned char next_byte = byte_buf[index];
        unsigned char remainder_mask = 0x7F >> (6 - index % 7);
        unsigned char new_bits_mask = 0xFE << (index % 7);

        mesg.packets[index + index / 7] |= ((new_bits_mask & next_byte) >> (index % 7 + 1)) & 0x7F;
        mesg.packets[index + index / 7 + 1] |= ((remainder_mask & next_byte) << (6 - index % 7));
    }

    mesg.checksum = mesg.packets[0] ^ mesg.packets[1];

    for (int index = 2; index < mesg.packets_num; index++)
    {
        mesg.checksum ^= mesg.packets[index];
    }

    mesg.checksum &= 0x7F; // ensure top bit is clear
    mesg.header = 0x80 | (mesg.address << 4) | mesg.bytes;

    return 0;
}

void display_mesg(struct PWMMesg& mesg)
{
    for (int bit = 0; bit < 8; bit++)
    {
        if (bit % 4 == 0)
        {
            printf(" ");
        }

        char digit = '0';
        if (mesg.header & (0x80 >> bit))
        {
            digit = '1';
        }

        printf("%c", digit);
    }

    printf("    ");

    for (int packet = 0; packet < mesg.packets_num; packet++)
    {
        for (int bit = 0; bit < 8; bit++)
        {
            if (bit % 4 == 0)
            {
                printf(" ");
            }

            char digit = '0';
            if (mesg.packets[packet] & (0x80 >> bit))
            {
                digit = '1';
            }

            printf("%c", digit);
        }

        printf("    ");
    }

    for (int bit = 0; bit < 8; bit++)
    {
        if (bit % 4 == 0)
        {
            printf(" ");
        }

        char digit = '0';
        if (mesg.checksum & (0x80 >> bit))
        {
            digit = '1';
        }

        printf("%c", digit);
    }

    printf("\n");
}

int ArduinoPWM::send_mesg(int address, void* buf, int size)
{
    struct PWMMesg mesg;
    memset(&mesg, 0, sizeof(struct PWMMesg));

    if (buf_to_mesg(mesg, address, buf, size) == -1)
    {
        return -1;
    }

    display_mesg(mesg);
    pthread_mutex_lock(&write_lock);
    mesg_queue.push(mesg);
    pthread_mutex_unlock(&write_lock);

    pthread_cond_signal(&new_mesg_sig);

    return 0;
}

int ArduinoPWM::send_mesg(struct PWMMesg& mesg)
{
    if (mesg.address >= MAX_ADDR)
    {
        printf("Error: ArduinoPWM.send_mesg(): address %d is out of range\n", (int)mesg.address);
        return -1;
    }

    if (mesg.bytes >= MAX_MESG_SIZE)
    {
        printf("Error: ArduinoPWM.send_mesg(): message size of %d bytes is out of range\n", (int)mesg.bytes + 1);
        return -1;
    }

    //display_mesg(mesg);
    pthread_mutex_lock(&write_lock);
    mesg_queue.push(mesg);
    pthread_mutex_unlock(&write_lock);

    pthread_cond_signal(&new_mesg_sig);

    return 0;
}
