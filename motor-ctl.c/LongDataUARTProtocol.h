#ifndef __LONG_DATA_UART_PROTOCOL__
#define __LONG_DATA_UART_PROTOCOL__

#include <Arduino.h>

#define MAX_MESG_SIZE 14

enum LDProtocolState
{
    GET_HEADER,
    GET_DATA,
    GET_CHECKSUM,
};

struct LDProtocol
{
    char mailbox_addr;
    char state;

    char bytes_total;
    char packets_total;
    char packets_current;
    unsigned char checksum;

    unsigned char mesg_buffer[18];
    char buffer[MAX_MESG_SIZE];

    char mailbox_sizes[8];
    unsigned char* mailboxes[8];
};

void init_ldprotocol(struct LDProtocol* proto_handle);

void register_mailbox(char which, void* mailbox, char size, struct LDProtocol* proto_handle);

char bytes_to_packets(char bytes);

void recieve_message(struct LDProtocol* proto_handle);

void ldproto_state_machine(struct LDProtocol* proto_handle);

#endif
