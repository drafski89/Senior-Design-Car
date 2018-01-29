#ifndef REMOTECTLAPP_H
#define REMOTECTLAPP_H

#include <SDL2/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

struct GamepadState
{
    double axis_x;
    double axis_y;
    double axis_z;

    double axis_yaw;
    double axis_pitch;
    double axis_roll;

    char button[32];
};

class RemoteCtlApp
{
private:
    bool running;
    int gamepad; // which gamepad we are using
    struct GamepadState gamepad_state;
    SDL_Joystick* sdl_gamepad;
    int robot_socket;

    int _init_tcp();

public:
    RemoteCtlApp();

    int execute();
    int _init_sdl();
    void _event(SDL_Event* Event);
    void _loop();
    void _cleanup();
};

#endif
