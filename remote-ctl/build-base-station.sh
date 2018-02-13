#!/bin/bash

g++ -std=c++11 -pthread -Wall -o base-station -lSDL2 base-station.cpp RemoteCtlApp.cpp
