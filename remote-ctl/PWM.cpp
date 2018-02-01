#include "PWM.h"

using namespace jetsongpio;
using namespace std;

void* pwm_loop(void* gen_ptr)
{
    PWMGen* generator = (PWMGen*)gen_ptr;
    pthread_mutex_lock(&generator->write_lock);
    generator->running = true;

    while (generator->running)
    {
        // get values from class while mutex is locked
        unsigned long period = generator->period + generator->period_offset;
        unsigned long pulse_width = generator->pulse_width + generator->pulse_width_offset;

        // start pulse
        generator->gpio.set_pin(generator->pin, true);

        // UNLOCK
        pthread_mutex_unlock(&generator->write_lock);

        // get time of rising edge
        struct timeval now;
        gettimeofday(&now, NULL);
        unsigned long rising_edge_us = now.tv_sec * 1000000 + now.tv_usec;

        // wait
        int status = usleep(pulse_width);

        // stop pulse
        generator->gpio.set_pin(generator->pin, false);

        // if sleep was interrupted by SIGCONT, abort the loop
        if ((status == -1) && (errno == EINTR))
        {
            pthread_mutex_lock(&generator->write_lock);
            continue;
        }

        gettimeofday(&now, NULL);
        unsigned long actual_sleep = (now.tv_sec * 1000000 + now.tv_usec) - rising_edge_us;

        // LOCK
        pthread_mutex_lock(&generator->write_lock);

        unsigned long pulse_count = generator->pulse_count;
        generator->pulse_width_offset = pulse_count * generator->pulse_width_offset / (pulse_count + 1) +
                                       (pulse_width - actual_sleep) / (pulse_count + 1);

        // UNLOCK
        pthread_mutex_unlock(&generator->write_lock);

        status = usleep(period - actual_sleep);

        // if sleep was interrupted by SIGCONT, abort the loop
        if ((status == -1) && (errno == EINTR))
        {
            pthread_mutex_lock(&generator->write_lock);
            continue;
        }

        gettimeofday(&now, NULL);
        unsigned long pulse_sleep = actual_sleep;
        actual_sleep = (now.tv_sec * 1000000 + now.tv_usec) - rising_edge_us;

        // LOCK
        pthread_mutex_lock(&generator->write_lock);

        generator->period_offset = pulse_count * generator->period_offset / (pulse_count + 1) +
                                  (period - (actual_sleep + pulse_sleep)) / (pulse_count + 1);
        generator->pulse_count++;
    }

    pthread_mutex_unlock(&generator->write_lock);

    return NULL;
}

PWMGen::PWMGen() : period(10000), pulse_width(1500), running(false), pin(HeaderPin_t::GPIO_29),
                   period_offset(0), pulse_width_offset(0), pulse_count(1)
{
    gpio = GPIOCtl();
    pthread_mutex_init(&write_lock, NULL);
}

PWMGen::PWMGen(GPIOCtl& _gpio) : period(10000), pulse_width(1500), running(false),
                                 gpio(_gpio), pin(HeaderPin_t::GPIO_29), period_offset(0),
                                 pulse_width_offset(0), pulse_count(1)
{
    pthread_mutex_init(&write_lock, NULL);
}

PWMGen::~PWMGen()
{
    if (running)
    {
        pthread_mutex_lock(&write_lock);
        running = false;
        pthread_kill(pwm_thread, SIGCONT); // wake thread up if sleeping
        pthread_mutex_unlock(&write_lock);
        
        gpio.set_pin(pin, false);
        gpio.disable_pin(pin);
    }

    pthread_join(pwm_thread, NULL);
    pthread_mutex_destroy(&write_lock);
}

void PWMGen::start()
{
    if (!running)
    {
        gpio.enable_pin(pin);
        gpio.pin_direction(pin, PinType_t::OUT);
        
        pthread_mutex_lock(&write_lock); // ensure nothing interrupts thread creation
        pthread_create(&pwm_thread, NULL, &pwm_loop, (void*)this);
        pthread_mutex_unlock(&write_lock);
    }
    else
    {
        printf("Info: PWMGen.start(): generator is already running\n");
    }
}

void PWMGen::stop()
{
    if (running)
    {
        pthread_mutex_lock(&write_lock);
        running = false;
        pthread_kill(pwm_thread, SIGCONT); // wake thread up if sleeping
        pthread_mutex_unlock(&write_lock);
        
        gpio.set_pin(pin, false);
        gpio.disable_pin(pin);
    }
    else
    {
        printf("Info: PWMGen.stop(): generator is not currently running\n");
    }
}

GPIOCtl& PWMGen::get_gpio()
{
    return gpio;
}


unsigned long PWMGen::get_period()
{
    return period;
}


unsigned long PWMGen::get_pulse_width()
{
    return pulse_width;
}

HeaderPin_t PWMGen::get_pin()
{
    return pin;
}

void PWMGen::set_period(unsigned long period)
{
    pthread_mutex_lock(&write_lock);
    this->period = period;
    pulse_count = 1;
    period_offset = 0;
    pthread_mutex_unlock(&write_lock);
}

void PWMGen::set_pulse_width(unsigned long pulse_width)
{
    pthread_mutex_lock(&write_lock);
    this->pulse_width = pulse_width;
    pulse_count = 1;
    pulse_width_offset = 0;
    pthread_mutex_unlock(&write_lock);
}

void PWMGen::set_pin(HeaderPin_t pin)
{
    if (!running)
    {
        this->pin = pin;
    }
    else
    {
        printf("Error: PWMGen.set_pin(): attempted to change output pin while generator is running!\n");
    }
}

bool PWMGen::is_running()
{
    return running;
}
