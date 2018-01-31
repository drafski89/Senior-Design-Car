#ifndef __JETSONTX2_GPIO_CTL__
#define __JETSONTX2_GPIO_CTL__

#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <map>
#include <unistd.h>

namespace jetsongpio
{

#define TOTAL_GPIO_PINS 5
#define SYSFS_GPIO_PATH "/sys/class/gpio"

// maps the GPIO's header pin number to its system FS number
enum HeaderPin_t
{
    GPIO_29 = 398,
    GPIO_31 = 298,
    GPIO_33 = 389,
    GPIO_37 = 388,
    GPIO_18 = 481
};

enum PinType_t
{
    OUT,
    IN
};

#define PIN_ON  true
#define PIN_OFF false

struct PinState
{
    bool enabled;
    int pin_type;
};

class GPIOCtl
{
private:
    std::map<HeaderPin_t, struct PinState> pin_states;
    
public:
    static const char* PIN_TYPE_OUT;
    static const char* PIN_TYPE_IN;

    GPIOCtl();
    ~GPIOCtl();

    void enable_pin(HeaderPin_t pin_num);
    void disable_pin(HeaderPin_t pin_num);
    void pin_direction(HeaderPin_t pin_num, PinType_t pin_type);
    void set_pin(HeaderPin_t pin_num, bool on);
};

} // end namespace

#endif
