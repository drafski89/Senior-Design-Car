# Simple Remote Control App

Simple application for controlling the car with a gamepad from a remote machine.

_(NOTE: these instructions may need some tweaking)_

## To Build:

Build the base station executable:

    $ g++ -Wall -o base-station -lSDL2 base-station.cpp RemoteCtlApp.cpp

Build the Jetson reciever application:

    // TODO - not implemented yet

## Dependencies

You will need

- SDL2:
  https://libsdl.org
  
    $ sudo apt-get install libsdl2


