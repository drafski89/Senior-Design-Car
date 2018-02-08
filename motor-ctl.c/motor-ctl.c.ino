#include <Servo.h>
#include "LongDataUARTProtocol.h"

Servo motor_ctl;
struct LDProtocol pwm_reciever;
short drive_power;
short steering_angle;

void setup()
{
    drive_power = 0;
    steering_angle = 0;

    init_ldprotocol(&pwm_reciever);
    register_mailbox(0, (void*)&drive_power, sizeof(short), &pwm_reciever);
    register_mailbox(1, (void*)&steering_angle, sizeof(short), &pwm_reciever);

    Serial.begin(9600);

    pinMode(11, OUTPUT);
    digitalWrite(11, HIGH);
}

void loop()
{
    static char current_byte = 0;
    static unsigned long last_update = 0;

    while (Serial.available())
    {
        recieve_message(&pwm_reciever);
    }

    if (millis() - last_update > 100)
    {
        if (drive_power > 255)
        {
            drive_power = 255;
        }

        if (drive_power < 0)
        {
            drive_power = 0;
        }

        if (steering_angle > 255)
        {
            steering_angle = 255;
        }

        if (steering_angle < 0)
        {
            steering_angle = 0;
        }

        analogWrite(3, drive_power);
        analogWrite(5, steering_angle);

        last_update = millis();
    }
}
