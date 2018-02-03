#ifndef __JETSONTX2_GPIO_CTL__
#define __JETSONTX2_GPIO_CTL__

#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <map>
#include <unistd.h>
#include <pthread.h>

namespace jetsongpio
{

#define TOTAL_GPIO_PINS 5                  ///< Reference value. How many pins total
#define SYSFS_GPIO_PATH "/sys/class/gpio"  ///< Reference and convenience value. Used to easily define GPIO file paths

/**
 * This maps the the pins by their number on the header to their actual pin number
 * on the Tegra chip. They are intended to disambiguate and ease the location of
 * GPIO pins by allowing to address them simply by their position on the header.
 * These are the values that should be used when making calls to enable,
 * disable, set, etc the pins on the board through the GPIO class.
 */
enum HeaderPin_t
{
    GPIO_29 = 398,
    GPIO_31 = 298,
    GPIO_33 = 389,
    GPIO_37 = 388,
    GPIO_18 = 481
};

/**
 * This is used to define whether a pin is an input or output pin when making
 * calls to GPIOCtl.pin_type().
 */
enum PinType_t
{
    OUT,
    IN
};

#define PIN_ON  true  ///< Reference value for use with GPIOCtl.set_pin(). Pins are set to ON / HIGH with a value of true.
#define PIN_OFF false ///< Reference value for use with GPIOCtl.set_pin(). Pins are set to OFF / LOW with a value of false.

/**
 * The PinState struct is used by the GPIOCtl class to keep track of a pin's
 * basic parameters to eliminate, for instance, attempts to write to a pin that
 * is actually an input.
 */
struct PinState;

/**
 * This class acts as a hardware abstract layer of sorts for dealing with the
 * GPIO pins onboard the Jetson TX2.
 */
class GPIOCtl
{
private:
    static std::map<HeaderPin_t, struct PinState> pin_states;
    static pthread_mutex_t write_lock;
    static bool initialized;

    static void initialize();
    static void cleanup();

    static int enable_pin(HeaderPin_t pin_num, GPIOCtl* requestor);
    static int disable_pin(HeaderPin_t pin_num, GPIOCtl* requestor);
    static int pin_direction(HeaderPin_t pin_num, PinType_t pin_type, GPIOCtl* requestor);
    static int set_pin(HeaderPin_t pin_num, bool on, GPIOCtl* requestor);

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

struct PinState
{
    bool enabled; ///< True when a pin has been exported / enabled successfully
    int pin_type; ///< Tracks the pin type. Uses the values directly from PinType_t enum
    GPIOCtl* owner; ///< Tracks which object owns this pin to prevent clobbering and unauthroized access
};

} // end namespace

#endif
