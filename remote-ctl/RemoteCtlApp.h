#ifndef REMOTECTLAPP_H
#define REMOTECTLAPP_H

#include <SDL2/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <utility>

#define DEFAULT_MESG_SIZE 512

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
    // program behavior variables
    bool running;
    unsigned long last_refresh;   // when the console was last updated with debug info
    unsigned long last_broadcast; // when the gamepad's state was last transmitted to robot

    // SDL gamepad information
    int gamepad; // which gamepad we are using
    struct GamepadState gamepad_state;
    SDL_Joystick* sdl_gamepad;

    // connection information
    int robot_socket;
    std::vector<std::pair<int, struct sockaddr_in>> connections;

    int _init_tcp();
    void _connect_handler();
    void _tx_handler();

public:
    RemoteCtlApp();

    int execute();
    int _init_sdl();
    void _event(SDL_Event* Event);
    void _loop();
    void _cleanup();
};

#endif
