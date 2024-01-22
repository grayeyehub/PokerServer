#pragma once

#include "plotocol.h"

enum OP_TYPE { OP_RECV, OP_SEND, OP_ACCEPT };
struct EX_OVER
{
    WSAOVERLAPPED   m_over;
    WSABUF         m_wsabuf[1];
    unsigned char   m_packetbuf[MAX_BUFFER];
    OP_TYPE         m_op;
    SOCKET          m_csocket;      // OP_ACCEPT에서만 사용
};

enum PL_STATE { PL_ST_FREE, PL_ST_CONNECTED, PL_ST_INGAME };

struct SESSION
{
    mutex m_slock;
    PL_STATE  m_state;
    SOCKET   m_socket;
    int      id;

    EX_OVER m_recv_over;
    int m_prev_size;
    //Game Contents Data
    char   m_name[200];
    short   x, y;
};

constexpr int SERVER_ID = 0;

array< SESSION, MAX_USER + 1> players;