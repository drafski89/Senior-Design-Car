#include "jetsontx2-gpio-ctl.h"
#include <string>
#include <errno.h>

using namespace jetsongpio;
using namespace std;

const char* GPIOCtl::PIN_TYPE_IN  = "in";
const char* GPIOCtl::PIN_TYPE_OUT = "out";

GPIOCtl::GPIOCtl()
{
    struct PinState pin_init_state;
    pin_init_state.enabled = false;
    pin_init_state.pin_type = PinType_t::IN;

    // all pins are initially in an unknown state
    pin_states[HeaderPin_t::GPIO_18] = pin_init_state;
    pin_states[HeaderPin_t::GPIO_29] = pin_init_state;
    pin_states[HeaderPin_t::GPIO_31] = pin_init_state;
    pin_states[HeaderPin_t::GPIO_33] = pin_init_state;
    pin_states[HeaderPin_t::GPIO_37] = pin_init_state;
}

GPIOCtl::~GPIOCtl()
{
    // TODO: disable all pins on object destruction
}


void GPIOCtl::enable_pin(HeaderPin_t pin_num)
{
    struct PinState* pin_state = NULL;

    try
    {
        pin_state = &pin_states.at(pin_num);
    }
    catch (exception& exc)
    {
        printf("Error: GPIOCtl.enable(): Invalid pin number: %d\n", (int)pin_num);
        return;
    }

    int enable_file = open(SYSFS_GPIO_PATH"/export", O_WRONLY);
    if (enable_file == -1)
    {
        printf("Error: GPIOCtl.enable(): open(): failed to enable pin: 0x%04x\n", errno);
        return;
    }

    string enable_pin = to_string((int)pin_num);

    if (write(enable_file, enable_pin.c_str(), enable_pin.length()) == -1)
    {
        printf("Error: GPIOCtl.enable(): write(): failed to enable pin: 0x%04x\n", errno);
    }
    else
    {
        pin_state->enabled = true;
        pin_state->pin_type = PinType_t::IN; // assume all pins are input. TODO: check what they are or force them to be input for safety
    }

    close(enable_file);
}

void GPIOCtl::pin_direction(HeaderPin_t pin_num, PinType_t pin_type)
{
    struct PinState* pin_state = NULL;

    try
    {
        pin_state = &pin_states.at(pin_num);
    }
    catch (exception& exc)
    {
        printf("Error: GPIOCtl.pin_direction(): invalid pin number: %d\n", pin_type);
        return;
    }

    string pin_path(SYSFS_GPIO_PATH"/gpio");
    pin_path += to_string((int)pin_num) + "/direction";

    int dir_file = open(pin_path.c_str(), O_WRONLY);
    if (dir_file == -1)
    {
        printf("Error: GPIOCtl.pin_direction(): open(): failed to set pin type: %d\n", errno);
        return;
    }

    int status = 0;

    if (pin_type == PinType_t::IN)
    {
        status = write(dir_file, (void*)GPIOCtl::PIN_TYPE_IN, 2);
    }
    else
    {
        status = write(dir_file, (void*)GPIOCtl::PIN_TYPE_OUT, 3);
    }

    if (status == -1)
    {
        printf("Error: GPIOCtl.pin_direction(): write(): failed to set pin type: %d\n", errno);
    }
    else
    {
        pin_state->pin_type = (int)pin_type;
    }

    close(dir_file);
}

void GPIOCtl::set_pin(HeaderPin_t pin_num, bool on)
{
    try
    {
        struct PinState& pin_state = pin_states.at(pin_num);

        if (pin_state.pin_type != PinType_t::OUT)
        {
            printf("Error: GPIOCtl.set_pin(): %d is not an output pin\n", (int)pin_num);
            return;
        }
    }
    catch (exception& exc)
    {
        printf("Error: GPIOCtl.set_pin(): invalid pin number: %d\n", (int)pin_num);
        return;
    }

    string pin_path(SYSFS_GPIO_PATH"/gpio");
    pin_path += to_string((int)pin_num) + "/value";

    int set_file = open(pin_path.c_str(), O_WRONLY);
    if (set_file == -1)
    {
        printf("Error: GPIOCtl.set_pin(): open(): failed to set pin state: %d\n", errno);
        return;
    }

    char state = '0';
    if (on)
    {
        state = '1';
    }

    if (write(set_file, (void*)&state, sizeof(char)) == -1)
    {
        printf("Error: GPIOCtl.set_pin(): write(): failed to set pin state: %d\n", errno);
    }

    close(set_file);
}

void GPIOCtl::disable_pin(HeaderPin_t pin_num)
{
    // not implemented
}
