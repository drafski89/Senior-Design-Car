#include "ros/ros.h"
#include "RemoteCtlApp.h"
#include "RoverApp.h"
#include <string>
#include <iostream>
#include "ArduinoPWM.h"
#include <cstdlib>
#include <cmath>

using namespace std;

int main(int argc, char* argv[])
{
    string addr_str;
    struct in_addr host_addr;
    RoverApp* rover = NULL;

    do
    {
        printf("Enter IPv4 address of control host (e.g. \"192.168.1.1\") > ");
        memset((void*)&host_addr, 0, sizeof(struct in_addr));
        cin >> addr_str;

    } while (!inet_aton(addr_str.c_str(), &host_addr));

    try
    {
        ros::init(argc, argv, "base_ctl");
        rover = new RoverApp(host_addr);
    }
    catch (exception& exc)
    {
        printf("%s\nFatal error. Exiting.\n", exc.what());
        return EXIT_FAILURE;
    }

    rover->recieve_cmds();
    delete rover;

    return EXIT_SUCCESS;
}
