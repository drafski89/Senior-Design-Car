#ifndef __ROVER_APP__
#define __ROVER_APP__

#include "RemoteCtlApp.h"
#include "ArduinoPWM.h"
#include <string>
#include <iostream>
#include <cstdlib>
#include <cmath>

class RoverApp
{
    friend void* ctl_loop(void* rover_ptr);

private:
    bool running;
    ArduinoPWM pwm_gateway;
    struct GamepadState gamepad_state;

    int cmd_socket;
    struct sockaddr_in cmd_host_addr;

    pthread_t ctl_thread;
    pthread_mutex_t write_lock;

public:
    RoverApp(const struct in_addr& host);
    ~RoverApp();

    void recieve_cmds();
};

#endif
