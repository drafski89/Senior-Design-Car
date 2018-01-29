#include "RemoteCtlApp.h"

using namespace std;

const char* err_mesgs[] = {
    "No route to requested IP address",
    "Connection refused",
    "Connection timed out",
    "Connect was reset",
    "Unhandled connection error",
}

const char* connect_err_mesg(int code)
{
    const char* err_mesg = NULL;

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

    } while (!inet_aton(addr_str.c_str(), &host_addr))

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
        printf("Failed to connect to control host. ");
    }

    return EXIT_SUCCESS;
}
