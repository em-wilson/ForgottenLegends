#ifndef __SOCKETHELPER_H__
#define __SOCKETHELPER_H__

typedef struct descriptor_data DESCRIPTOR_DATA;

class Character;

class SocketHelper {
    public:
        static bool check_reconnect(DESCRIPTOR_DATA *d, bool fConn);
        static void close_socket(DESCRIPTOR_DATA *dclose);
        static void send_to_char(const char *txt, Character *ch);
        static void write_to_buffer(DESCRIPTOR_DATA *d, const char *txt, int length);
        static bool write_to_descriptor(int desc, const char *txt, int length);
};

#endif