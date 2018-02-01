#ifndef __PWM__
#define __PWM__

#include <cstdio>
#include "jetsontx2-gpio-ctl.h"
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>

class PWMGen
{
    friend void* pwm_loop(void* generator);

private:
    unsigned long period;
    unsigned long pulse_width;

    bool running;
    jetsongpio::GPIOCtl gpio;

    jetsongpio::HeaderPin_t pin;
    pthread_mutex_t write_lock;

    unsigned long period_offset;
    unsigned long pulse_width_offset;
    unsigned long pulse_count;

    pthread_t pwm_thread;

public:
    PWMGen();
    PWMGen(jetsongpio::GPIOCtl& gpio);
    ~PWMGen();

    void start();
    void stop();

    unsigned long get_period();
    unsigned long get_pulse_width();
    jetsongpio::HeaderPin_t get_pin();
    bool is_running();

    jetsongpio::GPIOCtl& get_gpio();

    void set_period(unsigned long period);
    void set_pulse_width(unsigned long pulse_width);
    void set_pin(jetsongpio::HeaderPin_t pin);
};

#endif
