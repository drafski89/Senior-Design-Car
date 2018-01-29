#include "RemoteCtlApp.h"

using namespace std;

RemoteCtlApp::RemoteCtlApp() : running(true), gamepad(0), sdl_gamepad(NULL),
                               robot_socket(0)
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
        printf("Failed to bind TCP socket to port. Error: 0x%08x\n", errno);
        return errno;
    }

    printf("TCP socket bound to port. Opening for connections...\n");

    // open the socket for at least one connection
    if (listen(robot_socket, 1))
    {
        printf("Failed to open TCP socket for connections. Error: 0x%08x\n", errno);
        return errno;
    }

    return EXIT_SUCCESS;
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
            if (value > 0) { value += 1; }

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
    static time_t last_update = 0;

    time_t current_time = time(NULL);

    if (current_time - last_update >= 1)
    {
        char button[33];
        button[32] = '\0';

        for (int index = 0; index < 32; index++)
        {
            button[index] = gamepad_state.button[index] + 0x30;
        }

        printf("X: %g    Y: %g    Z: %g\n"
               "Y: %g    P: %g    R: %g\n"
               "%s\n",
               gamepad_state.axis_x, gamepad_state.axis_y, gamepad_state.axis_z,
               gamepad_state.axis_yaw, gamepad_state.axis_pitch, gamepad_state.axis_roll,
               button);

        last_update = current_time;
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
