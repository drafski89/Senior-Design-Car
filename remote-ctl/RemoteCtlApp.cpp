#include "RemoteCtlApp.h"

using namespace std;

RemoteCtlApp::RemoteCtlApp() : running(true), gamepad(0), sdl_gamepad(NULL)
{
    memset((void*)&gamepad_state, 0, sizeof(struct GamepadState));
}

int RemoteCtlApp::_init()
{
    int status = 0;
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    status = SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS);

    printf("Joysticks detected: %d", SDL_NumJoysticks());
    sdl_gamepad = SDL_JoystickOpen(gamepad);
    SDL_JoystickEventState(SDL_ENABLE);

    return status;
}

void RemoteCtlApp::_event(SDL_Event* event)
{
    switch (event->type)
    {
    case SDL_QUIT:
        printf("User requested exit. Closing...\n");
        running = false;
        break;

    case SDL_APP_TERMINATING:
        printf("Interrupt signal recieved. Terminating...\n");
        running = false;
        break;

    case SDL_JOYAXISMOTION:
        //if (event->jaxis.which == gamepad)
        if (true)
        {
            short value = event->jaxis.value;
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
        char button[32];
        for (int index = 0; index < 32; index++)
        {
            button[index] = gamepad_state.button[index] + 0x30;
        }

        printf("X: %g    Y: %g    Z: %g\n"
               "%s\n",
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
    int status = _init();
    if (status < 0)
    {
        printf("Failed to initialize SDL input library. Error %x\n", status);
        return status;
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
