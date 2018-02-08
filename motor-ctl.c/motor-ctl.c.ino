#include <Servo.h>
#include "LongDataUARTProtocol.h"

Servo motor_ctl;
struct LDProtocol pwm_reciever;
unsigned char led_buffer[4];

void setup()
{
    init_ldprotocol(&pwm_reciever);
    register_mailbox(2, (void*)led_buffer, 4, &pwm_reciever);
    Serial.begin(9600);

    for (char index = 0; index < 4; index++) { led_buffer[index] = 0x00; }

    for (char pin = 0; pin < 8; pin++)
    {
        pinMode(pin + 2, OUTPUT);
        digitalWrite(pin + 2, HIGH);
    }

    pinMode(11, OUTPUT);
    digitalWrite(11, HIGH);
}

void loop()
{
    static char current_byte = 0;
    static char current_pack = 0;
    static unsigned long last_update = 0;

    while (Serial.available())
    {
        recieve_message(&pwm_reciever);
        //ldproto_state_machine(&pwm_reciever);
        /*
        led_buffer[current_pack] = (unsigned char)Serial.read();

        if (++current_pack >= 4)
        {
            current_pack = 0;
        }
        */
    }

    if (millis() - last_update > 1000)
    {
        unsigned char data = ~(led_buffer[current_byte]);

        for (char pin = 0; pin < 8; pin++)
        {
            if (data & (0x01 << pin))
            {
                digitalWrite(pin + 2, HIGH);
            }
            else
            {
                digitalWrite(pin + 2, LOW);
            }
        }

        if (current_byte == 0)
        {
            digitalWrite(11, LOW);
        }
        else
        {
            digitalWrite(11, HIGH);
        }

        if (++current_byte >= 4)
        {
            current_byte = 0;
        }

        last_update = millis();
    }
}
