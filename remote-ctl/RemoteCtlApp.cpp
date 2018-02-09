#include "RemoteCtlApp.h"

using namespace std;

RemoteCtlApp::RemoteCtlApp() : running(true), last_refresh(0), last_broadcast(0),
                               gamepad(0), sdl_gamepad(NULL), robot_socket(0)
{
    memset((void*)&gamepad_state, 0, sizeof(struct GamepadState));
}

int RemoteCtlApp::_init_tcp()
{
    printf("Setting up TCP socket for robots...\n");

    // create new internet socket, TCP protocol, infer correct protocol
    robot_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (!robot_socket)
    {
        robot_socket = 0;
        printf("Failed to create TCP socket. Error: 0x%08x\n", errno);
        return errno;
    }

    struct sockaddr_in socket_addr;
    socket_addr.sin_family = AF_INET;         // internet type socket
    socket_addr.sin_port = htons(5309);       // port 5309. need to use correct byte order
    socket_addr.sin_addr.s_addr = INADDR_ANY; // use this machine's IP address

    printf("TCP socket created. Binding to port 5309...\n");

    // will not compile without that wierd looking type cast
    if (bind(robot_socket, (struct sockaddr*)&socket_addr, sizeof(struct sockaddr_in)))
    {
        close(robot_socket);
        robot_socket = 0;
        printf("Failed to bind TCP socket to port. Error: 0x%08x\n", errno);
        return errno;
    }

    printf("TCP socket bound to port. Opening for connections...\n");

    // open the socket for at least one connection
    if (listen(robot_socket, 1))
    {
        close(robot_socket);
        robot_socket = 0;
        printf("Failed to open TCP socket for connections. Error: 0x%08x\n", errno);
        return errno;
    }

    // set socket to nonblocking to avoid stopping whole application when handling connections
    int sock_flags = fcntl(robot_socket, F_GETFL, NULL);

    if (sock_flags < 0)
    {
        close(robot_socket);
        robot_socket = 0;
        printf("Failed to fetch socket options. fcntl(): F_GETFL error: 0x08%x\n", errno);
        return errno;
    }

    sock_flags |= O_NONBLOCK;
    if (fcntl(robot_socket, F_SETFL, sock_flags) == -1)
    {
        close(robot_socket);
        robot_socket = 0;
        printf("Failed to set socket options. fcntl(): F_SETFL error: 0x08%x\n", errno);
        return errno;
    }

    return EXIT_SUCCESS;
}

void RemoteCtlApp::_connect_handler()
{
    if (robot_socket)
    {
        struct sockaddr_in connection_addr;
        memset((void*)&connection_addr, 0, sizeof(struct sockaddr_in));

        socklen_t addr_bytes = sizeof(struct sockaddr_in);
        int connection_sock = accept(robot_socket, (struct sockaddr*)&connection_addr, &addr_bytes);

        if (connection_sock == -1)
        {
            if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
            {
                printf("Error establishing new connection. accept() error 0x08%x\n", errno);
            }
        }
        else
        {
            printf("New connection from %s\n", inet_ntoa(connection_addr.sin_addr));
            connections.push_back(pair<int, struct sockaddr_in>(connection_sock, connection_addr));
        }
    }
}

void RemoteCtlApp::_tx_handler()
{
    for (auto iter = connections.begin(); iter != connections.end(); )
    {
        pair<int, struct sockaddr_in>& connection = *iter;
        int connection_sock = connection.first;

        char mesg[DEFAULT_MESG_SIZE];
        memset((void*)mesg, 0, DEFAULT_MESG_SIZE);
        memcpy((void*)mesg, (void*)&gamepad_state, sizeof(struct GamepadState));

        int bytes_sent = send(connection_sock, (void*)mesg, DEFAULT_MESG_SIZE, MSG_NOSIGNAL);

        if (bytes_sent == -1)
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                // do nothing. this is okay.
            }
            else if (errno == ECONNRESET)
            {
                printf("Lost connection to %s. Reset by peer.\n", inet_ntoa(connection.second.sin_addr));
                close(connection_sock);
                iter = connections.erase(iter);
                continue;
            }
            else if (errno == ENOTCONN)
            {
                printf("Lost connection to %s. Dropped.\n", inet_ntoa(connection.second.sin_addr));
                close(connection_sock);
                iter = connections.erase(iter);
                continue;
            }
            else if (errno == EPIPE)
            {
                printf("Lost connection %s. Connection unexpectedly closed.\n", inet_ntoa(connection.second.sin_addr));
                close(connection_sock);
                iter = connections.erase(iter);
                continue;
            }
            else
            {
                printf("Unable to send to %s. send() error 0x08%x. Terminating connection.\n",
                       inet_ntoa(connection.second.sin_addr), errno);
                close(connection_sock);
                iter = connections.erase(iter);
                continue;
            }
        }

        iter++;
    }
}

int RemoteCtlApp::_init_sdl()
{
    int status = 0;
    // SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    status = SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);

    if (status < 0)
    {
        return status;
    }

    int joysticks_num = SDL_NumJoysticks();
    printf("Joysticks detected: %d\n", joysticks_num);

    if (joysticks_num >= 1)
    {
        printf("Attempting to use joystick 0x%02x\n", gamepad);
        sdl_gamepad = SDL_JoystickOpen(gamepad);
        SDL_JoystickEventState(SDL_ENABLE);

        if (sdl_gamepad)
        {
            printf("Error: %s\n"
                   "\n"
                   "*** Joystick control may be non-functional. Proceed with caution! ***\n"
                   "\n",
                   SDL_GetError());
        }
        else
        {
            printf("Success!\n");
        }
    }
    else
    {
        printf("No usable input devices detected.\n");
    }

    return status;
}

void RemoteCtlApp::_event(SDL_Event* event)
{
    switch (event->type)
    {
    case SDL_QUIT:
        printf("\nUser requested exit. Closing...\n");
        running = false;
        break;

    case SDL_APP_TERMINATING:
        printf("\nInterrupt signal recieved. Terminating...\n");
        running = false;
        break;

    case SDL_JOYAXISMOTION:
        //if (event->jaxis.which == gamepad)
        if (true)
        {
            int value = event->jaxis.value;
            if (value > 0) { value += 1; } // limit of short is 32,767. increment to make division by 32768 equal 1

            double axis_val = (double)value / 32768.0;

            if (event->jaxis.axis == 0)
            {
                gamepad_state.axis_x = axis_val;
            }
            else if (event->jaxis.axis == 1)
            {
                gamepad_state.axis_y = axis_val;
            }
            else if (event->jaxis.axis == 2)
            {
                gamepad_state.axis_z = axis_val;
            }
            else if (event->jaxis.axis == 3)
            {
                gamepad_state.axis_yaw = axis_val;
            }
            else if (event->jaxis.axis == 4)
            {
                gamepad_state.axis_pitch = axis_val;
            }
            else if (event->jaxis.axis == 5)
            {
                gamepad_state.axis_roll = axis_val;
            }
        }
        break;

    case SDL_JOYBUTTONDOWN:
        //if (event->jbutton.which == gamepad)
        if (true)
        {
            if (event->jbutton.button < 32)
            {
                gamepad_state.button[event->jbutton.button] = (char)0x01;
            }
        }
        break;

    case SDL_JOYBUTTONUP:
        //if (event->jbutton.which == gamepad)
        if (true)
        {
            if (event->jbutton.button < 32)
            {
                gamepad_state.button[event->jbutton.button] = (char)0x00;
            }
        }
        break;

    default:
        break;
    }
}

void RemoteCtlApp::_loop()
{
    struct timeval current_time;
    memset((void*)&current_time, 0, sizeof(struct timeval));

    gettimeofday(&current_time, NULL);

    unsigned long ctime_ms = current_time.tv_sec * 1000 +
                             current_time.tv_usec / 1000;

    /*
    if (ctime_ms - last_refresh >= 200)
    {
        char button[33];
        button[32] = '\0';

        for (int index = 0; index < 32; index++)
        {
            button[index] = gamepad_state.button[index] + 0x30;
        }

        printf("X: %06.4g    Y: %06.4g    Z: %06.4g    Y: %06.4g    P: %06.4g    R: %06.4g    %s\r",
               gamepad_state.axis_x, gamepad_state.axis_y, gamepad_state.axis_z,
               gamepad_state.axis_yaw, gamepad_state.axis_pitch, gamepad_state.axis_roll,
               button);

        last_refresh = ctime_ms;
    }
    */

    _connect_handler();

    if (ctime_ms - last_broadcast >= 100)
    {
        _tx_handler();

        last_broadcast = ctime_ms;
    }
}

void RemoteCtlApp::_cleanup()
{
    SDL_Quit();
}

int RemoteCtlApp::execute()
{
    // initialize SDL library. exit immediately if a failure occurs
    int status = _init_sdl();
    if (status < 0)
    {
        printf("Failed to initialize SDL input library. Error 0x%08x\n", status);
        return status;
    }

    status = _init_tcp();
    if (status)
    {
        printf("\n"
               "*** Creating TCP socket failed! Robot will not be able to connect! ***\n"
               "\n");
    }

    SDL_Event event;

    // while the user has no closed the application
    while (running)
    {
        // get all events
        while (SDL_PollEvent(&event))
        {
            _event(&event);
        }

        // process data and send to robot
        _loop();
    }

    // shutdown SDL library
    _cleanup();

    return EXIT_SUCCESS;
}
