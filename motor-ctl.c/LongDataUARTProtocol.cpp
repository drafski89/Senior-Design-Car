#include "LongDataUARTProtocol.h"

void init_ldprotocol(struct LDProtocol* proto_handle)
{
    unsigned char* init_handle = (unsigned char*)proto_handle;

    for (unsigned int index = 0; index < sizeof(struct LDProtocol); index++)
    {
        init_handle[index] = '\0';
    }

    //pinMode(11, OUTPUT);
    //digitalWrite(11, HIGH);
}

void register_mailbox(char which, void* mailbox, char size, struct LDProtocol* proto_handle)
{
    if (size > MAX_MESG_SIZE || which > 7)
    {
        return;
    }

    proto_handle->mailbox_sizes[which] = size;
    proto_handle->mailboxes[which] = (unsigned char*)mailbox;
}

char bytes_to_packets(char bytes)
{
    int bits = bytes * 8;

    if (bits % 7)
    {
        return (char)(bits / 7 + 1);
    }

    return (char)(bits / 7);
}

void process_mesg(struct LDProtocol* proto_handle)
{
    char mailbox_addr = (proto_handle->mesg_buffer[0] & 0x70) >> 4;
    unsigned char byte_buffer[MAX_MESG_SIZE];
    for (char index = 0; index < MAX_MESG_SIZE; index++) { byte_buffer[index] = '\0'; }
    unsigned char checksum = '\0';

    for (char index = 0; index < proto_handle->packets_total; index++)
    {
        unsigned char packet = proto_handle->mesg_buffer[index + 1];
        checksum ^= packet;

        unsigned char new_bits_mask = 0x7F >> (index % 7);
        unsigned char remainder_mask = 0x7F & (0xFF << (7 - index % 7));

        unsigned char new_bits = (packet & new_bits_mask) << (1 + index % 7);
        unsigned char remainder_bits = (packet & remainder_mask) >> (7 - index % 7);

        if (remainder_mask && (index != 0))
        {
            byte_buffer[index - 1 - index / 7] |= remainder_bits;

            if (index < proto_handle->packets_total)
            {
                byte_buffer[index - index / 7] |= new_bits;
            }
        }
        else
        {
            byte_buffer[index - index / 7] |= new_bits;
        }
    }

    if (checksum == proto_handle->mesg_buffer[proto_handle->packets_total + 1])
    {
        unsigned char* mailbox = proto_handle->mailboxes[mailbox_addr];
        char mailbox_size = proto_handle->mailbox_sizes[mailbox_addr];

        for (char index = 0; index < mailbox_size; index++)
        {
            mailbox[index] = byte_buffer[index];
        }
    }
}

void recieve_message(struct LDProtocol* proto_handle)
{
    unsigned char packet = (unsigned char)Serial.read();

    switch (proto_handle->state)
    {
    case GET_HEADER:
        if (packet & 0x80)
        {
            proto_handle->packets_total = bytes_to_packets((packet & 0x0F) + 1);

            if (proto_handle->packets_total <= 16)
            {
                proto_handle->mesg_buffer[0] = packet;
                proto_handle->state = GET_DATA;
                proto_handle->packets_current = 0;
            }
        }
        break;

    case GET_DATA:
        if ((~packet) & 0x80)
        {
            proto_handle->mesg_buffer[proto_handle->packets_current + 1] = packet;

            if (++proto_handle->packets_current >= proto_handle->packets_total)
            {
                proto_handle->state = GET_CHECKSUM;
            }
        }
        else
        {
            proto_handle->state = GET_HEADER;
        }
        break;

    case GET_CHECKSUM:
        if ((~packet) & 0x80)
        {
            proto_handle->mesg_buffer[proto_handle->packets_current + 1] = packet;

            unsigned char* mailbox = proto_handle->mailboxes[0];
            char mailbox_size = proto_handle->mailbox_sizes[0];

            process_mesg(proto_handle);
        }
        proto_handle->state = GET_HEADER;
        break;
    }
}

void ldproto_state_machine(struct LDProtocol* proto_handle)
{
    unsigned char packet = (unsigned char)Serial.read();

    if (proto_handle->state == GET_HEADER)
    {
        // if control header bit is set
        if (packet & 0x80)
        {
            proto_handle->mailbox_addr = (packet & 0x70) >> 4;
            char bytes_total = (packet & 0x0F) + 1;

            if (bytes_total < 14)
            {
                proto_handle->packets_total = bytes_to_packets(bytes_total);
                proto_handle->packets_current = 0;
                proto_handle->checksum = 0;
                proto_handle->state = GET_DATA;

                //digitalWrite(11, LOW);

                for (char index = 0; index < 14; index++)
                {
                    proto_handle->buffer[index] = '\0';
                }
            }
        }
    }
    else if (proto_handle->state == GET_DATA)
    {
        // if control header bit is clear
        if ((~packet) & 0x80)
        {
            //digitalWrite(11, HIGH);
            char packet_current = proto_handle->packets_current;
            unsigned char remainder_mask = 0x7F & (0xFF << (7 - packet_current % 7));
            unsigned char new_bits_mask = 0x7F >> (packet_current % 7);

            unsigned char new_bits = (packet & new_bits_mask) << (1 + packet_current % 7);
            unsigned char remainder_bits = (packet & remainder_mask) >> (7 - packet_current % 7);

            if (remainder_mask && (packet_current != 0))
            {
                proto_handle->buffer[packet_current - 1 - packet_current / 7] |= remainder_bits;

                if (packet_current < proto_handle->packets_total)
                {
                    proto_handle->buffer[packet_current - packet_current / 7] |= new_bits;
                }
            }
            else
            {
                proto_handle->buffer[packet_current - packet_current / 7] |= new_bits;
            }

            proto_handle->checksum ^= packet;

            if (++proto_handle->packets_current >= proto_handle->packets_total)
            {
                proto_handle->state = GET_CHECKSUM;
            }
        }
        else
        {
            proto_handle->state = GET_HEADER;
        }
    }
    else if (proto_handle->state == GET_CHECKSUM)
    {
        proto_handle->checksum &= 0x7F; // 7 data bits. ignore top bit

        if (packet == proto_handle->checksum)
        {
            unsigned char* mailbox = proto_handle->mailboxes[proto_handle->mailbox_addr];
            char mailbox_size = proto_handle->mailbox_sizes[proto_handle->mailbox_addr];

            for (char index = 0; (index < proto_handle->bytes_total) &&
                                 (index < MAX_MESG_SIZE) &&
                                 (index < mailbox_size);
                index++)
            {
                mailbox[index] = proto_handle->buffer[index];
            }

            proto_handle->state = GET_HEADER;
        }
        else
        {
            proto_handle->state = GET_HEADER;
        }
    }
}
