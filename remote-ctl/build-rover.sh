#!/bin/bash

g++ -std=c++11 -pthread -I../gpio-lib -Wall -o rover rover.cpp PWM.cpp ../gpio-lib/jetsontx2-gpio-ctl.o
