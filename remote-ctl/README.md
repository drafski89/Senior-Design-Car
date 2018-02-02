# Simple Remote Control App

Simple application for controlling the car with a gamepad from a remote machine.
Currently capable of steering the car but not driving it.

## To Build:

*Build the base station executable:*

    $ ./build-base-station.sh

*Build the Jetson reciever application:*

_NOTE: You will need to build the GPIO API before this. If you already did, skip ahead._

    $ cd ../gpio-lib
    $ ./build-gpiolib.sh
    $ cd ../remote-ctl

---

    $ ./build-rover.sh

## Dependencies

You will need

- SDL2:
  https://libsdl.org
  
    $ sudo apt-get install sdl2

# To Run:

On the host machine with Xbox controller attached, run the 'base-station' executable.

    $ ./base-station

This will start a server for transmitting the gamepad state to the car.
After starting the base station, run the 'rover' executable on the Jetson.

    $ sudo ./rover   // you need root privelege for using the GPIO

