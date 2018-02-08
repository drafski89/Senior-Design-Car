#ifndef __ARDUINO_PWM__
#define __ARDUINO_PWM__

#include <termios.h>
#include <unistd.h>
#include <queue>
#include <pthread.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>

struct PWMMesg
{
    unsigned char address;
    unsigned char bytes;
    unsigned char packets_num;
    unsigned char header;
    unsigned char checksum;
    unsigned char packets[16];
};

#define MAX_ADDR      8  ///< Only 8 PWMs are addressable
#define MAX_MESG_SIZE 14 ///< 14 bytes maximum message size

class ArduinoPWM
{
    friend void* send_mesg_loop(void* arduino_pwm_ptr);

private:
    struct termios uart_props;
    int uart_gateway;
    bool running;

    std::queue<struct PWMMesg> mesg_queue;

    pthread_mutex_t write_lock;
    pthread_cond_t new_mesg_sig;
    pthread_mutex_t new_mesg_lock; ///< dummy mutex. required to use pthread conditions but this class manages its own mutexes for finer control
    pthread_t send_mesg_thread;

public:

    ArduinoPWM();
    ~ArduinoPWM();

    int buf_to_mesg(struct PWMMesg& mesg, int address, void* buf, int size);
    int send_mesg(int address, void* buf, int size);
    int send_mesg(struct PWMMesg& mesg);
};

#endif
