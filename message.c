#include "message.h"

#include <stdio.h>

void message_out(unsigned char message_type, char *file, unsigned int line, unsigned int ch, char *message) {
    char *t;
    if (message_type == MESSAGE_ERROR) {
        t = "error";
    }
    else if (message_type == MESSAGE_WARN) {
        t = "warning";
    }
    
    printf("%s:%d:%d: %s: %s\n", file, line, ch, t, message);
}
