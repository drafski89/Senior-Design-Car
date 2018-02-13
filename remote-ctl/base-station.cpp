#include "RemoteCtlApp.h"

int main(int argc, char* argv[])
{
    RemoteCtlApp* ctl_app = new RemoteCtlApp();

    int status = ctl_app->execute();

    delete ctl_app;
    return status;
}
