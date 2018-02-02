#include "RemoteCtlApp.h"
#include <string>
#include <iostream>
#include "PWM.h"

using namespace std;
using namespace jetsongpio;

char* err_mesgs[] = {
    "No route to requested IP address",
    "Connection refused",
    "Connection timed out",
    "Connect was reset",
    "Unhandled connection error",
};

char* connect_err_mesg(int code)
{
    char* err_mesg = NULL;

    switch (code)
    {
    case EADDRNOTAVAIL:
        err_mesg = err_mesgs[0];
        break;

    case ECONNREFUSED:
        err_mesg = err_mesgs[1];
        break;

    case ENETUNREACH:
        err_mesg = err_mesgs[0];
        break;

    case ETIMEDOUT:
        err_mesg = err_mesgs[2];
        break;

    case ECONNRESET:
        err_mesg = err_mesgs[3];
        break;

    default:
        err_mesg = err_mesgs[4];
        break;
    }

    return err_mesg;
}

int main(int argc, char* argv[])
{
    string addr_str;
    struct in_addr host_addr;

    do
    {
        printf("Enter IPv4 address of control host (e.g. \"192.168.1.1\") > ");
        memset((void*)&host_addr, 0, sizeof(struct in_addr));
        cin >> addr_str;

    } while (!inet_aton(addr_str.c_str(), &host_addr));

    printf("Creating TCP socket for recieving commands...\n");
    int command_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (!command_socket)
    {
        printf("Failed to create TCP socket. Error: 0x%08x\n", errno);
        return EXIT_FAILURE;
    }

    printf("Attempting to connect to control host at %s:5309...\n", addr_str.c_str());

    struct sockaddr_in socket_addr;
    socket_addr.sin_family = AF_INET;         // internet type socket
    socket_addr.sin_port = htons(5309);       // port 5309. need to use correct byte order
    socket_addr.sin_addr.s_addr = host_addr.s_addr;

    if (connect(command_socket, (struct sockaddr*)&socket_addr, sizeof(struct sockaddr_in)))
    {
        printf("Failed to connect to control host. %s. Error: 0x%08x",
               connect_err_mesg(errno), errno);
        return EXIT_FAILURE;
    }

    printf("Success! Now listening for commands...\n");

    int bytes_recieved = DEFAULT_MESG_SIZE;
    struct GamepadState gamepad_state;
    memset((void*)&gamepad_state, 0, sizeof(struct GamepadState));

    char mesg[DEFAULT_MESG_SIZE];
    PWMGen generator;
    generator.set_pin(HeaderPin_t::GPIO_37);
    generator.set_pulse_width(1500);
    generator.start();
    int steering_offset = 500; // 500 microseconds

    while (bytes_recieved > -1)
    {
        memset((void*)mesg, 0, DEFAULT_MESG_SIZE);
        bytes_recieved = recv(command_socket, (void*)mesg, DEFAULT_MESG_SIZE, 0);

        if (bytes_recieved == -1)
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                continue;
            }
            else if (errno == ECONNREFUSED)
            {
                printf("Lost connection to host. Stopping...\n");
                close(command_socket);
            }
            else
            {
                printf("Connection error. recv() error 0x08%x\n", errno);
                close(command_socket);
            }
        }
        else
        {
            memcpy((void*)&gamepad_state, (void*)mesg, sizeof(struct GamepadState));

            char button[33];
            memset((void*)button, 0, 33);

            for (int index = 0; index < 32; index++)
            {
                button[index] = gamepad_state.button[index] + 0x30;
            }

            printf("X: %+06.4g    Y: %+06.4g    Z: %+06.4g    Y: %+06.4g    P: %+06.4g    R: %+06.4g    %s\r",
                   gamepad_state.axis_x, gamepad_state.axis_y, gamepad_state.axis_z,
                   gamepad_state.axis_yaw, gamepad_state.axis_pitch, gamepad_state.axis_roll,
                   button);
            
            generator.set_pulse_width(1500 + (long)(gamepad_state.axis_x * ((double)steering_offset)));
        }
    }
    
    generator.stop();

    return EXIT_SUCCESS;
}
