#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "logger.h"

//#include "logger.h"
//#include <exception>


struct bfd_cp_mess
{
    uint8_t diagcode : 5;
    uint8_t protover : 3;
    uint8_t mlag : 6;
    uint8_t sstate : 2;
    uint8_t dtmult[1];
    uint8_t mlen[1];
    uint8_t mdiscr[4];
    uint8_t ydiscr[4];
    uint8_t txint[4];
    uint8_t rxint[4];
    uint8_t eint[4] = {0x00, 0x00, 0x00, 0x00};
};

struct bfdpeer
{
    int peerid;
    uint8_t localstate : 2 = 0x01;
    uint8_t peerstate : 2 =
        0x01; // 00 -- AdminDown, 01 -- Down, 10 -- Init, 11 -- Up
    uint8_t mdiscr[4];
    uint8_t ydiscr[4];
    uint8_t txint[4] = {0x00, 0x1e, 0x84, 0x80};
    uint8_t rxint[4] = {0x00, 0x1e, 0x84, 0x80};
    uint8_t eint[4];
    sockaddr assocket;
    sockaddr acsocket;
    int ssocket;
    int csocket;
    int peer_port;
    const char* ip_local;
    const char* ip_peer;
    size_t recvtimer = 2000000;
    size_t senttimer = 2000000;
    size_t retrytimer = 3;
    std::string name;
    std::string vpn_instance;
};

struct udphead
{
    uint16_t source;
    uint16_t dest;
    uint16_t len;
    uint16_t check;
};

class Udpsocket
{
  private:
    std::shared_ptr<logger::Logger> logger;

    bool ClientSocketinUse = 0;
    socklen_t AserAddrlen = sizeof(ServerAddr);

    void freeResources()
    {
        if (ip_local)
        {
            delete[] ip_local;
            ip_local = nullptr;
        }
        if (ip_peer)
        {
            delete[] ip_peer;
            ip_peer = nullptr;
        }
    }

    void llog(std::string level, std::string message)
    {
        try{
            if(level == "debug")
            {
                logger->debug(message);
            }
            if(level == "error")
            {
                logger->error(message);
            }
            if(level == "info")
            {
                logger->info(message);
            }
            if(level == "warning")
            {
                logger->warning(message);
            }
        }
        catch(const std::exception& e)
        {
            if (logger)
            {
                logger->error("Udpsocket::llog->Logger Error! ->" + level + ":" + message);
                logger->error(e.what());
            }
            else
            {
                std::cerr << "Udpsocket::llog->Error:" << level + ":" + message << e.what() << '\n';
            }
        }
        catch (...)
        {
            if (logger)
            {
                logger->error("Udpsocket::llog->Unknown error\n" + level + ":" + message);
            }
            else
            {
                std::cerr << "Udpsocket::llog->Unknown error\n" + level + ":" + message;
            }
        }
    }

  public:
    char* ip_local = nullptr;
    int port_local;
    char* ip_peer = nullptr;
    int port_peer;
    int holdtime;

    int ClientSocket;
    int ServerSocket;

    sockaddr ServerAddr;

    Udpsocket();
    Udpsocket(char* nip_local, int nport_local, char* nip_peer, int nholdtime, std::shared_ptr<logger::Logger> logger);
    ~Udpsocket();
    Udpsocket(const Udpsocket& copy);
    Udpsocket& operator=(const Udpsocket& copy);
    Udpsocket(Udpsocket&& moved) noexcept;
    Udpsocket& operator=(Udpsocket&& moved) noexcept;

    int udpserver();
    int updclient();

};


class Bfd
{
  private:

    std::shared_ptr<logger::Logger> logger;

    bfd_cp_mess _bfd_cp_recv;
    bfd_cp_mess _bfd_cp_send;

    uint8_t rawbuffer[4096];
    uint8_t sndbuffer[4096];

    sockaddr_in _tempaddr;
    socklen_t tempaddrlen = sizeof(_bfdpeer.assocket);

    char packet[50];
    char udp[8];
    struct iphdr* ip = (struct iphdr*)packet;
    struct sockaddr_in daddr;
    bool ipready = 0;
    struct udphead* udphead = (struct udphead*)udp;

    struct pollfd fds[1];
    int SRECV = -1000;
    int checkSocket;
    int waitfail = 0;
    std::chrono::time_point<std::chrono::system_clock> epeertime;
    std::chrono::time_point<std::chrono::system_clock> elocaltime;

    void freeResources()
    {
        if (ip_local)
        {
            delete[] ip_local;
            ip_local = nullptr;
        }
        if (ip_peer)
        {
            delete[] ip_peer;
            ip_peer = nullptr;
        }
    }
    void llog(std::string level, std::string message)
    {
        if (!logger){
            return;
        }
        try{
            if(level == "debug")
            {
                logger->debug(message);
            }
            if(level == "error")
            {
                logger->error(message);
            }
            if(level == "info")
            {
                logger->info(message);
            }
            if(level == "warning")
            {
                logger->warning(message);
            }
        }
        catch(const std::exception& e)
        {
            if (logger)
            {
                logger->error("Bfd::llog->Logger Error! ->" + level + ":" + message);
                logger->error(e.what());
            }
            else
            {
                std::cerr << "Bfd::llog->Error:" << level + ":" + message << e.what() << '\n';
            }
        }
        catch (...)
        {
            if (logger)
            {
                logger->error("Bfd::llog->Unknown error\n" + level + ":" + message);
            }
            else
            {
                std::cerr << "Bfd::llog->Unknown error\n" + level + ":" + message;
            }
        }
    }

    void initiatepeer(int ssocket, int& id, const char* ip_local,
                      const char* ip_peer)
    {
        _bfdpeer.peerid = id;
        _bfdpeer.ip_local = ip_local;
        _bfdpeer.ip_peer = ip_peer;
        _bfdpeer.ssocket = ssocket;
        _bfdpeer.mdiscr[0] = 0x00;
        _bfdpeer.mdiscr[1] = 0x00;
        _bfdpeer.mdiscr[2] = 0x40;
        _bfdpeer.mdiscr[3] = static_cast<uint8_t>(id);
        recvtimer = senttimer = _bfdpeer.recvtimer * 1000;
    }

    void makerecvtimer(size_t recvtimer)
    {
        _bfdpeer.txint[0] = recvtimer >> 32;
        _bfdpeer.txint[1] = recvtimer >> 16;
        _bfdpeer.txint[2] = recvtimer >> 8;
        _bfdpeer.txint[3] = recvtimer & 0xFF;

        _bfdpeer.rxint[0] = recvtimer >> 32;
        _bfdpeer.rxint[1] = recvtimer >> 16;
        _bfdpeer.rxint[2] = recvtimer >> 8;
        _bfdpeer.rxint[3] = recvtimer & 0xFF;

        _bfdpeer.recvtimer = _bfdpeer.senttimer = recvtimer;
    }

    void getclisocketport()
    {
        memcpy(&_tempaddr, &_bfdpeer.assocket, tempaddrlen);
        _bfdpeer.peer_port = ntohs(_tempaddr.sin_port);
    }

    void recvcpbfd()
    {
        memcpy(&_bfd_cp_recv, rawbuffer, 24);

        if (_bfd_cp_recv.sstate == 0x00)
        {
            _bfdpeer.peerstate = 0x00;
            _bfdpeer.localstate = 0x01;
            llog("debug","Bfd::recvcpbfd->State Admin DOWN->" + _bfdpeer.name);            
            return;
        }
        if (_bfd_cp_recv.sstate == 0x01)
        { // DOWN
            if (_bfdpeer.peerstate == 0x00)
            {
                // from down to first down
                _bfdpeer.peerstate = 0x01;
                _bfdpeer.localstate = 0x01;
                _bfdpeer.ydiscr[0] = 0x00; //_bfd_cp_recv.mdiscr[0];
                _bfdpeer.ydiscr[1] = 0x00; //_bfd_cp_recv.mdiscr[1];
                _bfdpeer.ydiscr[2] = 0x00; //_bfd_cp_recv.mdiscr[2];
                _bfdpeer.ydiscr[3] = 0x00; //_bfd_cp_recv.mdiscr[3];
                llog("debug","Bfd::recvcpbfd->State DOWN->" + _bfdpeer.name); 
                return;
            }
            if (_bfdpeer.peerstate == 0x01)
            {
                // from down to init
                _bfdpeer.peerstate = 0x02;
                _bfdpeer.localstate = 0x02;
                _bfdpeer.ydiscr[0] = _bfd_cp_recv.mdiscr[0];
                _bfdpeer.ydiscr[1] = _bfd_cp_recv.mdiscr[1];
                _bfdpeer.ydiscr[2] = _bfd_cp_recv.mdiscr[2];
                _bfdpeer.ydiscr[3] = _bfd_cp_recv.mdiscr[3];

                // _bfdpeer.rxint[0] = _bfd_cp_recv.txint[0];
                // _bfdpeer.rxint[1] = _bfd_cp_recv.txint[1];
                // _bfdpeer.rxint[2] = _bfd_cp_recv.txint[2];
                // _bfdpeer.rxint[3] = _bfd_cp_recv.txint[3];
                llog("debug","Bfd::recvcpbfd->State INIT->" + _bfdpeer.name); 
                return;
            }
            if (_bfdpeer.peerstate != 0x01)
            {
                _bfdpeer.peerstate = 0x01;
                _bfdpeer.localstate = 0x01;
                llog("debug","Bfd::recvcpbfd->To DOWN->" + _bfdpeer.name);
                makerecvtimer(2000000);
                return;
            }
        }
        if (_bfd_cp_recv.sstate == 0x02)
        {
            if (_bfdpeer.localstate != 0x00)
            {
                _bfdpeer.peerstate = 0x03;
                _bfdpeer.localstate = 0x03;
                _bfdpeer.ydiscr[0] = _bfd_cp_recv.mdiscr[0];
                _bfdpeer.ydiscr[1] = _bfd_cp_recv.mdiscr[1];
                _bfdpeer.ydiscr[2] = _bfd_cp_recv.mdiscr[2];
                _bfdpeer.ydiscr[3] = _bfd_cp_recv.mdiscr[3];

                // timer for send
                senttimer =
                    (_bfd_cp_recv.rxint[3] | _bfd_cp_recv.rxint[2] << 8 |
                     _bfd_cp_recv.rxint[1] << 16 |
                     _bfd_cp_recv.rxint[0] << 32); // mus
                llog("debug","Bfd::recvcpbfd->INIT->senttimer->" + std::to_string(senttimer) + "->" +_bfdpeer.name);
                // timer for recv
                recvtimer = senttimer;

                makerecvtimer(recvtimer);
                llog("debug","Bfd::recvcpbfd->State To UP->" + _bfdpeer.name);
                llog("info","Bfd::recvcpbfd->State To UP->" + _bfdpeer.name);
                return;
                // send to peer
                // sendclicpbfd();
            }
        }
        if (_bfd_cp_recv.sstate == 0x03)
        {
            if ((_bfdpeer.localstate != 0x00) && (_bfdpeer.peerstate != 0x03) &&
                (_bfdpeer.localstate != 0x03))
            {
                _bfdpeer.peerstate = 0x03;
                _bfdpeer.localstate = 0x03;

                // timer for send
                senttimer =
                    (_bfd_cp_recv.rxint[3] | _bfd_cp_recv.rxint[2] << 8 |
                     _bfd_cp_recv.rxint[1] << 16 |
                     _bfd_cp_recv.rxint[0] << 32); // mus
                llog("debug","Bfd::recvcpbfd->UP->senttimervalue->" + std::to_string(senttimer) + "->" +_bfdpeer.name);
                // timer for recv
                recvtimer = senttimer;
                makerecvtimer(recvtimer);
                llog("debug","Bfd::recvcpbfd->State UP->" + _bfdpeer.name);
                llog("info","Bfd::recvcpbfd->State UP->" + _bfdpeer.name);
                return;
            }
            if (_bfdpeer.localstate == 0x03)
            {
                size_t tempsenttimer =
                    (_bfd_cp_recv.rxint[3] | _bfd_cp_recv.rxint[2] << 8 |
                     _bfd_cp_recv.rxint[1] << 16 | _bfd_cp_recv.rxint[0] << 32);
                if (tempsenttimer != senttimer)
                {
                    senttimer = tempsenttimer;
                    recvtimer = senttimer;
                    makerecvtimer(recvtimer);
                }
                if (retrytimer > 0)
                {
                    retrytimer--;
                    llog("debug","Bfd::recvcpbfd->retrytimer--->" + std::to_string(retrytimer) + "->" +_bfdpeer.name);
                }
            }
        }
    }

    void makeipbuff()
    {

        memset(sndbuffer, 0, 4096);
        if (ipready == 0)
        {
            memset(packet, 0, sizeof(packet)); /* payload will be all As */
            ip->ihl = 5;
            ip->version = 4;
            ip->tos = 0xe0;
            ip->tot_len = htons(40);    /* 16 byte value */
            ip->frag_off = 0;           /* no fragment */
            ip->ttl = 255;              /* default value */
            ip->protocol = IPPROTO_UDP; /* protocol at L4 */
            ip->check = 0;              /* not needed in iphdr */
            ip->saddr = inet_addr(_bfdpeer.ip_local);
            ip->daddr = inet_addr(_bfdpeer.ip_peer);
            ipready = 1;
            daddr.sin_family = AF_INET;
            daddr.sin_port = 0;
            inet_pton(AF_INET, _bfdpeer.ip_peer,
                      (struct in_addr*)&daddr.sin_addr.s_addr);
            memset(daddr.sin_zero, 0, sizeof(daddr.sin_zero));

            udphead->source = htons(_bfdpeer.peer_port + 7);
            udphead->dest = htons(3784);
            udphead->len = htons(32);
            udphead->check = 0;
        }

        memcpy(&sndbuffer, (char*)packet, sizeof(packet));
        memcpy(sndbuffer + 20, (char*)udp, sizeof(udp));
    }

    void sendsercpbfd()
    {
        _bfd_cp_send.sstate = _bfdpeer.localstate;
        _bfd_cp_send.mlen[0] = 0x18;
        _bfd_cp_send.protover = 0x01;
        _bfd_cp_send.mlag = 0;
        _bfd_cp_send.dtmult[0] = 0x03;
        _bfd_cp_send.mdiscr[0] = _bfdpeer.mdiscr[0];
        _bfd_cp_send.mdiscr[1] = _bfdpeer.mdiscr[1];
        _bfd_cp_send.mdiscr[2] = _bfdpeer.mdiscr[2];
        _bfd_cp_send.mdiscr[3] = _bfdpeer.mdiscr[3];
        _bfd_cp_send.ydiscr[0] = _bfdpeer.ydiscr[0];
        _bfd_cp_send.ydiscr[1] = _bfdpeer.ydiscr[1];
        _bfd_cp_send.ydiscr[2] = _bfdpeer.ydiscr[2];
        _bfd_cp_send.ydiscr[3] = _bfdpeer.ydiscr[3];

        _bfd_cp_send.txint[0] = _bfdpeer.txint[0];
        _bfd_cp_send.txint[1] = _bfdpeer.txint[1];
        _bfd_cp_send.txint[2] = _bfdpeer.txint[2];
        _bfd_cp_send.txint[3] = _bfdpeer.txint[3];

        _bfd_cp_send.rxint[0] = _bfdpeer.rxint[0];
        _bfd_cp_send.rxint[1] = _bfdpeer.rxint[1];
        _bfd_cp_send.rxint[2] = _bfdpeer.rxint[2];
        _bfd_cp_send.rxint[3] = _bfdpeer.rxint[3];

        makeipbuff();
        memcpy(sndbuffer + 20 + 8, &_bfd_cp_send, 24);
        sendto(_bfdpeer.csocket, &sndbuffer, 20 + 8 + 24, 0,
               (struct sockaddr*)&daddr, (socklen_t)sizeof(daddr));
        llog("debug", "Bfd::sendsercpbfd->" + std::to_string(daddr.sin_addr.s_addr));
    }

  public:
    char* ip_local = nullptr;
    int port_local;
    char* ip_peer = nullptr;
    int holdtime;

    Udpsocket _bfdserv;
    bfdpeer _bfdpeer;

    size_t recvtimer = 2000;
    size_t senttimer = 2000;
    size_t retrytimer = 0;

    /// @brief
    Bfd();
    Bfd(char* nip_local, int nport_local, char* nip_peer, int nholdtime,
        std::string nname, std::string nvpn_instalnce, std::shared_ptr<logger::Logger> logger);
    ~Bfd();
    Bfd(const Bfd& copy);
    Bfd& operator=(const Bfd& copy);
    Bfd(Bfd&& moved) noexcept;
    Bfd& operator=(Bfd&& moved) noexcept;

    void mainprocess();
    void admindown();
    void adminup();
};

inline Udpsocket::Udpsocket() = default;

inline Udpsocket::Udpsocket(char* nip_local, int nport_local, char* nip_peer, int nholdtime, std::shared_ptr<logger::Logger> logger) :
    ip_local(new char[strlen(nip_local) + 1]),
    port_local(nport_local), ip_peer(new char[strlen(nip_peer) + 1]),
    holdtime(nholdtime), ClientSocket(ClientSocket), ServerSocket(ServerSocket),
    ServerAddr(ServerAddr), AserAddrlen(AserAddrlen), logger(logger)
{
    llog("debug", "Udpsocket::constructor->begin");
    strcpy(ip_local, nip_local);
    strcpy(ip_peer, nip_peer);
    llog("debug", "Udpsocket::constructor->end");
}

inline Udpsocket::~Udpsocket()
{
    llog("debug", "Udpsocket::destructor->begin");
    freeResources();
    llog("debug","Udpsocket::destructor->end");
}

inline Udpsocket::Udpsocket(const Udpsocket& copy) :
    ip_local(new char[strlen(copy.ip_local) + 1]), port_local(copy.port_local),
    ip_peer(new char[strlen(copy.ip_peer) + 1]), holdtime(copy.holdtime),
    ClientSocket(ClientSocket), ServerSocket(ServerSocket),
    ServerAddr(ServerAddr), AserAddrlen(AserAddrlen),
    ClientSocketinUse(ClientSocketinUse),
    logger(copy.logger)
{
    llog("debug","Udpsocket::Udpsocket(const Udpsocket& copy)->begin");
    strcpy(ip_local, copy.ip_local);
    strcpy(ip_peer, copy.ip_peer);
    llog("debug","Udpsocket::Udpsocket(const Udpsocket& copy)->end");
}

inline Udpsocket& Udpsocket::operator=(const Udpsocket& copy)
{
    llog("debug","Udpsocket::Udpsocket::operator=(const Udpsocket& copy)->begin");
    if (this != &copy)
    {
        freeResources(); // Освобождаем старую память
        ip_local = new char[strlen(copy.ip_local) + 1];
        port_local = copy.port_local;
        ip_peer = new char[strlen(copy.ip_peer) + 1];
        holdtime = copy.holdtime;
        ClientSocket = copy.ClientSocket;
        ServerSocket = copy.ServerSocket;
        ServerAddr = copy.ServerAddr;
        AserAddrlen = copy.AserAddrlen;
        ClientSocketinUse = copy.ClientSocketinUse;
        logger = copy.logger;
        strcpy(ip_local, copy.ip_local);
        strcpy(ip_peer, copy.ip_peer);
        llog("debug","Udpsocket::Udpsocket::operator=(const Udpsocket& copy)->if (this != &copy)");
    }
    llog("debug","Udpsocket::Udpsocket::operator=(const Udpsocket& copy)->end");
    return *this;
}

inline Udpsocket::Udpsocket(Udpsocket&& moved) noexcept :
    ip_local(moved.ip_local), port_local(moved.port_local),
    ip_peer(moved.ip_peer), holdtime(moved.holdtime),
    ClientSocket(moved.ClientSocket), ServerSocket(moved.ServerSocket),
    ServerAddr(moved.ServerAddr), AserAddrlen(moved.AserAddrlen), logger(moved.logger)
{
    llog("debug","Udpsocket::Udpsocket(Udpsocket&& moved) noexcept->begin");
    moved.ip_local = nullptr;
    moved.ip_peer = nullptr;
    moved.port_local = 0;
    moved.holdtime = 0;
    llog("debug","Udpsocket::Udpsocket(Udpsocket&& moved) noexcept->end");
}

inline Udpsocket& Udpsocket::operator=(Udpsocket&& moved) noexcept
{
    //llog("debug","Udpsocket::Udpsocket::operator=(Udpsocket&& moved) noexcept->begin");
    if (this != &moved)
    {
        freeResources(); // Освобождаем старую память
        ip_local = moved.ip_local;
        port_local = moved.port_local;
        ip_peer = moved.ip_peer;
        holdtime = moved.holdtime;
        ClientSocket = moved.ClientSocket;
        ServerSocket = moved.ServerSocket;
        ServerAddr = moved.ServerAddr;
        AserAddrlen = moved.AserAddrlen;
        logger = moved.logger;
        // ClientSocketinUse = moved.ClientSocketinUse;
        moved.ip_local = nullptr;
        moved.ip_peer = nullptr;
        moved.port_local = 0;
        moved.holdtime = 0;
        //llog("debug","Udpsocket::Udpsocket::operator=(Udpsocket&& moved) noexcept->if (this != &moved)");
    }
    //llog("debug","Udpsocket::Udpsocket::operator=(Udpsocket&& moved) noexcept->end");
    return *this;
}

int Udpsocket::udpserver()
{
    ServerSocket = socket(AF_INET, SOCK_DGRAM, 17);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port_local);
    serverAddress.sin_addr.s_addr = inet_addr(ip_local);

    bind(ServerSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    listen(ServerSocket, 5);
    const struct timeval tv = {holdtime, 0};
    setsockopt(ServerSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,
               sizeof(tv));
    llog("debug","Udpsocket::udpserver->" + std::to_string(serverAddress.sin_addr.s_addr) + "." + std::to_string(port_local));
    return ServerSocket;
}

int Udpsocket::updclient()
{
    if ((ClientSocketinUse == 0) || (ClientSocket == -1))
    {
        ClientSocket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
        if (ClientSocket == -1)
        {
            logger->error("Udpsocket::updclient->ClientSocketmakeError");
        }
        else
        {
            sockaddr_in ClientAddress;
            ClientAddress.sin_family = AF_INET;
            ClientAddress.sin_addr.s_addr = inet_addr(ip_local);
            ClientAddress.sin_port = 0;
            bind(ClientSocket, (struct sockaddr*)&ClientAddress,
                 sizeof(ClientAddress));
            ClientSocketinUse = 1;
            llog("debug","Udpsocket::updclient->ClientSocketmakeOK " + std::to_string(ClientAddress.sin_addr.s_addr));
        }
    }
    llog("debug","Udpsocket::updclient->ClientSocketReturn");
    return ClientSocket;
}

//########################BFD
Bfd::Bfd() = default;

 Bfd::Bfd(char* nip_local, int nport_local, char* nip_peer, int nholdtime, std::string nname, std::string nvpn_instalnce, std::shared_ptr<logger::Logger> logger) :
    ip_local(new char[strlen(nip_local) + 1]),
    port_local(nport_local), ip_peer(new char[strlen(nip_peer) + 1]),
    holdtime(nholdtime),
    logger(logger)
{
    llog("debug","Bfd::constructor->begin" + nname);
    strcpy(ip_local, nip_local);
    strcpy(ip_peer, nip_peer);
    llog("debug","Bfd::constructor->startudpsocket");
    _bfdserv = Udpsocket(ip_local, port_local, ip_peer, holdtime, logger);
    _bfdpeer.ssocket = fds[0].fd = _bfdserv.udpserver();
    _bfdpeer.name = nname;
    _bfdpeer.vpn_instance = nvpn_instalnce;
    _bfdpeer.recvtimer = recvtimer;
    _bfdpeer.senttimer = senttimer;
    fds[0].events = POLLIN;
    int x = 1;
    initiatepeer(_bfdpeer.ssocket, x, ip_local, ip_peer);
    elocaltime = epeertime = std::chrono::system_clock::now();
    llog("debug","Bfd::constructor->end" + nname);
}

Bfd::~Bfd()
{
    llog("debug","Bfd::destructor->begin");
    freeResources();
    llog("debug","Bfd::destructor->end");
}

inline Bfd::Bfd(const Bfd& copy) :
    ip_local(new char[strlen(copy.ip_local) + 1]), port_local(copy.port_local),
    ip_peer(new char[strlen(copy.ip_peer) + 1]), holdtime(copy.holdtime),
    _bfdpeer(copy._bfdpeer), _bfdserv(copy._bfdserv), _tempaddr(copy._tempaddr),
    tempaddrlen(copy.tempaddrlen), daddr(copy.daddr), ipready(copy.ipready), logger(copy.logger)
{
    llog("debug","Bfd::Bfd(const Bfd& copy)->begin");
    strcpy(ip_local, copy.ip_local);
    strcpy(ip_peer, copy.ip_peer);
    llog("debug","Bfd::Bfd(const Bfd& copy)->end");
}

inline Bfd& Bfd::operator=(const Bfd& copy)
{
    llog("debug","Bfd::Bfdoperator=(const Bfd& copy)->begin");
    if (this != &copy)
    {
        freeResources();
        ip_local = new char[strlen(copy.ip_local) + 1];
        port_local = copy.port_local;
        ip_peer = new char[strlen(copy.ip_peer) + 1];
        holdtime = copy.holdtime;
        _bfdpeer = copy._bfdpeer;
        _bfdserv = copy._bfdserv;
        _tempaddr = copy._tempaddr;
        tempaddrlen = copy.tempaddrlen;
        daddr = copy.daddr;
        ipready = copy.ipready;
        strcpy(ip_local, copy.ip_local);
        strcpy(ip_peer, copy.ip_peer);
        ip = new struct iphdr(*copy.ip);
        udphead = new struct udphead(*copy.udphead);
        logger = copy.logger;
        llog("debug","Bfd::Bfdoperator=(const Bfd& copy)->if (this != &copy)");
    }
    llog("debug","Bfd::Bfdoperator=(const Bfd& copy)->end");
    return *this;
}

 Bfd::Bfd(Bfd&& moved) noexcept :
    ip_local(moved.ip_local), port_local(moved.port_local),
    ip_peer(std::move(moved.ip_peer)), holdtime(moved.holdtime),
    _bfdpeer(std::move(moved._bfdpeer)), _bfdserv(std::move(moved._bfdserv)),
    fds{std::move(moved.fds[0])}, SRECV(moved.SRECV),
    checkSocket(moved.checkSocket), _tempaddr(std::move(moved._tempaddr)),
    tempaddrlen(std::move(tempaddrlen)), daddr(std::move(daddr)),
    ipready(moved.ipready), logger(moved.logger)

{
    llog("debug","Bfd::Bfd(Bfd&& moved) noexcept->begin");
    moved.ip_local = nullptr;
    moved.ip_peer = nullptr;
    moved.port_local = 0;
    moved.holdtime = 0;
    llog("debug","Bfd::Bfd(Bfd&& moved) noexcept->end");
}

inline Bfd& Bfd::operator=(Bfd&& moved) noexcept
{
    llog("debug","Bfd::operator=(Bfd&& moved) noexcept->if (this != &moved)->begin");
    if (this != &moved)
    {
        // freeResources(); // Освобождаем старую память
        ip_local = moved.ip_local;
        port_local = moved.port_local;
        ip_peer = std::move(moved.ip_peer);
        holdtime = moved.holdtime;
        _bfdpeer = std::move(moved._bfdpeer);
        _bfdserv = std::move(moved._bfdserv);
        fds[0] = std::move(moved.fds[0]);
        SRECV = moved.SRECV;
        checkSocket = moved.checkSocket;
        _tempaddr = std::move(moved._tempaddr);
        tempaddrlen = std::move(moved.tempaddrlen);
        daddr = std::move(daddr);
        ipready = moved.ipready;
        logger = moved.logger;

        moved.ip_local = nullptr;
        moved.ip_peer = nullptr;
        moved.port_local = 0;
        moved.holdtime = 0;
        llog("debug","Bfd::operator=(Bfd&& moved) noexcept->if (this != &moved)");
    }
    llog("debug","Bfd::operator=(Bfd&& moved) noexcept->if (this != &moved)->end");
    return *this;
}

void Bfd::mainprocess()
{
    if ((_bfdpeer.localstate == 0x00) || (_bfdpeer.localstate == 0x01))
    {
        epeertime = std::chrono::system_clock::now();
    }

    checkSocket = -1000;
    checkSocket = poll(*&fds, 1, _bfdpeer.senttimer / 1000);
    if ((checkSocket > 0) && (_bfdpeer.localstate != 0x00))
    {
        SRECV = -1000;
        SRECV = recvfrom(_bfdpeer.ssocket, &rawbuffer, 4096, 0, (struct sockaddr*)&_bfdpeer.assocket, &tempaddrlen);
        if (SRECV > 0)
        {
            epeertime = std::chrono::system_clock::now();
            getclisocketport();

            _bfdpeer.csocket = _bfdserv.updclient();

            _bfdpeer.acsocket = _bfdserv.ServerAddr;
            recvcpbfd();
            sendsercpbfd();
            elocaltime = std::chrono::system_clock::now();
        }
    }

    if ((std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - epeertime).count() > _bfdpeer.recvtimer) && (_bfdpeer.localstate == 0x03) && (retrytimer < _bfdpeer.retrytimer))
    {
        retrytimer++;
        llog("warning","Bfd::mainprocess->retrytimer++->" + std::to_string(retrytimer));
    }

    if ((retrytimer >= _bfdpeer.retrytimer) && (_bfdpeer.localstate == 0x03))
    {
        _bfdpeer.localstate = 0x01;
        logger->error("Bfd::mainprocess->todown->" + _bfdpeer.name + "->failtimes->" + std::to_string(retrytimer));
        retrytimer = 0;
    }
}

void Bfd::admindown()
{
    _bfdpeer.localstate = 0x00;
    llog("info","Bfd::admindown");
    sendsercpbfd();
}

void Bfd::adminup()
{
    _bfdpeer.localstate = 0x03; //????
    llog("info","Bfd::adminup");
    sendsercpbfd();
}