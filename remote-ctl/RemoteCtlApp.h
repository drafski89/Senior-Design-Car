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

#define DEFAULT_MESG_SIZE 512 ///< 512 bytes default packet size for transmitting gamepad state

/**
 * This struct is used for storing the state of the Xbox controller in a code
 * friendly manner and for transmitting its state over TCP. Note that this struct
 * created on and transmitted from an Intel machine meaning multibyte values,
 * namely the first 6 fields which are doubles, will be in little-endian byte
 * order. The button array, being a boolean array, is unaffected.
 */
struct GamepadState
{
    double axis_x; ///< Horizontal axis of left stick. Range [-1, 1]. -1 is full left.
    double axis_y; ///< Vertical axis of left stick. Range [-1, 1]. -1 is full up.
    double axis_z;

    double axis_yaw;
    double axis_pitch;
    double axis_roll;

    char button[32]; ///< button's number is its index into the array. Boolean: 0 or 1. 1 is pressed.
};

/**
 * This is a server for remote control of the Jetson car. It accepts connections
 * from the Jetson car over a TCP socket on port 5309 and sends a copy of the
 * current gamepad state at 20 Hz. It expects the Xbox controller to be the first
 * device available i.e. /dev/js0. If no joystick is detected, the program will
 * still run, though manual controll will not be possible. If there is a server
 * already running, this server will explicitly fail to start.
 */
class RemoteCtlApp
{
private:
    bool running;                 ///< keep the server loop running and controls application exit
    unsigned long last_refresh;   ///< millisecond Unix time when the console was last updated with debug info
    unsigned long last_broadcast; ///< millisecond Unix time when the gamepad's state was last transmitted to robot

    int gamepad;                       ///< index number of the gamepad we are using (0)
    struct GamepadState gamepad_state; ///< current gamepad state for transmission
    SDL_Joystick* sdl_gamepad;         ///< SDL library handle for the gamepad

    int robot_socket; ///< Unix file handle for the TCP server socket
    std::vector<std::pair<int, struct sockaddr_in>> connections; ///< Holds all active connection sockets and the IP information of the client

    /**
     * Initializes the TCP server socket for listening for connections from the
     * Jetson car. Uses port 5309. The socket created will be non-blocking to
     * avoid blocking the application when checking for connections.
     *
     * @return Returns 0 on successful establishment of the socket. On failure,
     * returns the POSIX error code that occured during socket creation.
     */
    int _init_tcp();         ///< Initializes the server socket

    /**
     * Checks for any pending connections on the server socket and accepts them.
     * Accepted connections are placed into the 'connections' vector along with
     * the IP information of the client. Failed connections will simply be dropped.
     */
    void _connect_handler(); ///< Handles incoming connections from Jetson car

    /**
     * Transmits the current gamepad state to all connected clients. If errors
     * arise during transmission, the connection will be closed and the client
     * will need to reconnect.
     */
    void _tx_handler(); ///< Transmits gamepad state to all clients.

public:
    RemoteCtlApp();

    int execute();
    int _init_sdl();
    void _event(SDL_Event* Event);
    void _loop();
    void _cleanup();
};

#endif
