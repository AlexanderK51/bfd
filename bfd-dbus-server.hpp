#pragma once
#include <sdbus-c++/sdbus-c++.h>
#include "bfd.hpp"
#include <iostream>
#include <string>
#include <tuple>
#include <sstream>


namespace net {
    namespace msystems {
    
    class Bfd_adaptor
    {
    public:
        static constexpr const char* INTERFACE_NAME = "net.msystems.Bfd";
    
    protected:
        Bfd_adaptor(sdbus::IObject& object) : object_(object){}
    
        ~Bfd_adaptor() = default;
    
        void registerAdaptor()
        {
            //forInterface(INTERFACE_NAME)
            object_.addVTable(\
                sdbus::registerMethod("add").withInputParamNames("tablename", "tablebody").withOutputParamNames("status").implementedAs([this](const std::string& tablename, const std::vector<std::string>& tablebody){ return this->add(tablename, tablebody); }),\
                sdbus::registerMethod("mod").withInputParamNames("tablename", "idbody").withOutputParamNames("status").implementedAs([this](const std::string& tablename, const std::vector<std::string>& tablebody){ return this->mod(tablename, tablebody); }),\
                sdbus::registerMethod("rmv").withInputParamNames("tablename", "idbody").withOutputParamNames("status").implementedAs([this](const std::string& tablename, const std::string& idbody){ return this->rmv(tablename, idbody); }),\
                sdbus::registerMethod("lst").withInputParamNames("tablename", "idbody").withOutputParamNames("status").implementedAs([this](const std::string& tablename, const std::string& idbody){ return this->lst(tablename, idbody); }),\
                sdbus::registerMethod("dsp").withInputParamNames("tablename", "idbody").withOutputParamNames("status").implementedAs([this](const std::string& tablename, const std::string& idbody){ return this->dsp(tablename, idbody); }),\
                sdbus::registerMethod("rst").withInputParamNames("tablename", "idbody").withOutputParamNames("status").implementedAs([this](const std::string& tablename, const std::string& idbody){ return this->rst(tablename, idbody); }),\
                sdbus::registerMethod("set").withInputParamNames("tablename", "idbody").withOutputParamNames("status").implementedAs([this](const std::string& tablename, const std::string& idbody){ return this->set(tablename, idbody); }),\
                sdbus::registerSignal("start").withParameters<bool>("start"),\
                sdbus::registerSignal("alarm").withParameters<int32_t>("almid"),\
                sdbus::registerSignal("destroyroute").withParameters<std::vector<std::string>>("vpnvia"),\
                sdbus::registerSignal("recroyroute").withParameters<std::vector<std::string>>("vpnvia")
                            ).forInterface(INTERFACE_NAME);
        }
    
    
    public:
        void emitStart(const bool& start)
        {
            object_.emitSignal("start").onInterface(INTERFACE_NAME).withArguments(start);
        }
    
        void emitAlarm(const int32_t& almid)
        {
            object_.emitSignal("alarm").onInterface(INTERFACE_NAME).withArguments(almid);
        }
    
        void emitDestRoute(const std::vector<std::string>& vpnvia)
        {
            object_.emitSignal("destroyroute").onInterface(INTERFACE_NAME).withArguments(vpnvia);
        }
    
        void emitRecRoute(const std::vector<std::string>& vpnvia)
        {
            object_.emitSignal("recroyroute").onInterface(INTERFACE_NAME).withArguments(vpnvia);
        }
    
    private:
        virtual int32_t add(const std::string& tablename, const std::vector<std::string>& tablebody) = 0;
        virtual int32_t mod(const std::string& tablename, const std::vector<std::string>& tablebody) = 0;
        virtual int32_t rmv(const std::string& tablename, const std::string& idbody) = 0;
        virtual std::vector<std::string> lst(const std::string& tablename, const std::string& idbody) = 0;
        virtual std::vector<std::string> dsp(const std::string& tablename, const std::string& idbody) = 0;
        virtual int32_t rst(const std::string& tablename, const std::string& idbody) = 0;
        virtual int32_t set(const std::string& tablename, const std::string& idbody) = 0;
    
    private:
        sdbus::IObject& object_;
    };
    
    }} // namespaces
    
   


class BfdDbusServer : sdbus::AdaptorInterfaces<net::msystems::Bfd_adaptor>
{
public:
    BfdDbusServer(sdbus::IConnection& connection, sdbus::ObjectPath objectPath, std::shared_ptr<logger::Logger> logger) : AdaptorInterfaces(connection, std::move(objectPath)), logger(logger)
    {
        llog("debug","BfdDbusServer::constructor->begin");
        registerAdaptor();
        llog("debug","BfdDbusServer::constructor->end");
    }

    ~BfdDbusServer()
    {
        llog("debug","BfdDbusServer::destructor->begin");
        unregisterAdaptor();
        killth();
        llog("debug","BfdDbusServer::destructor->end");
    }



//BFD PROCESS
bool bfdenable = 0;
std::vector <int> v_iddestroypeers;
std::vector <Bfd> v_bfdsess;
void bfdtrack();
void makebfd(const char * ip_local, const char * ip_peer, std::string name, std::string vpn_instalnce);
bool findid(int id, std::vector<int> v);
void destroyroute(int vector_id);
void recoveryroute(int vector_id);
std::thread th1;
std::atomic_bool stopflag = false;
//END BFDPROCESS

protected:

int32_t add(const std::string& tablename, const std::vector<std::string>& tablebody)
{
    llog("debug","BfdDbusServer::add::" + tablename + "->" + tablebody[0]);
    if(tablename == "bfdsession"){
        const char * ip_local;
        const char * ip_peer;
        ip_local = tablebody[0].c_str();
        ip_peer = tablebody[1].c_str();
        makebfd(ip_local, ip_peer, tablebody[2], tablebody[3]);
    }
    
    return 0;
}

int32_t mod(const std::string& tablename, const std::vector<std::string>& tablebody)
{
    //tablebody[0] - name
    //tablebody[1] - UP/DOWN
    llog("debug","BfdDbusServer::mod::" + tablename + "->" + tablebody[0]);
    if(tablename == "bfdsession"){
        for (size_t i = 0; i < v_bfdsess.size(); i++)
        {
            if(v_bfdsess[i]._bfdpeer.name == tablebody[0])
            {
                if (tablebody[1] == "DOWN")
                    v_bfdsess[i].admindown();
                if (tablebody[1] == "UP")
                    v_bfdsess[i].adminup();
            }
        }
    }
    return 0;
}

int32_t rmv(const std::string& tablename, const std::string& idbody)
{
    llog("debug","BfdDbusServer::rmv::" + tablename + "->" + idbody);
    return 0;
}

std::vector<std::string> lst(const std::string& tablename, const std::string& idbody)
{
    llog("debug","BfdDbusServer::lst::" + tablename + "->" + idbody);
    std::vector<std::string> result;
    std::string temp_res = "";
    if (tablename == "bfd"){
        if (bfdenable == 0){
            result.push_back("BFD DISABLE");
        }
        if (bfdenable == 1){
            result.push_back("BFD ENABLE");
        }
        return result;
    }
    if (tablename == "bfdsession"){
        if (idbody != "%"){
            for (size_t i = 0; i < v_bfdsess.size(); i++)
            {
                if(v_bfdsess[i]._bfdpeer.name == idbody){
                    std::string status;
                    if (v_bfdsess[i]._bfdpeer.localstate == 0x00){status = "ADMINDOWN";}
                    else{status = "ADMINUP";}
                    temp_res = v_bfdsess[i]._bfdpeer.name + " | " + v_bfdsess[i]._bfdpeer.vpn_instance + " | " + std::string(v_bfdsess[i]._bfdpeer.ip_local) + " | " + std::string(v_bfdsess[i]._bfdpeer.ip_peer) + " | " +\
                        status + " | " + std::to_string(v_bfdsess[i]._bfdpeer.senttimer) + " | " + std::to_string(v_bfdsess[i]._bfdpeer.recvtimer) + " | " + \
                        std::to_string(static_cast<int>(v_bfdsess[i]._bfdpeer.mdiscr[0]<<8 | v_bfdsess[i]._bfdpeer.mdiscr[1]) << static_cast<int>(v_bfdsess[i]._bfdpeer.mdiscr[2]<<8 | v_bfdsess[i]._bfdpeer.mdiscr[3])) + " | " + \
                        std::to_string(static_cast<int>(v_bfdsess[i]._bfdpeer.ydiscr[0]<<8 | v_bfdsess[i]._bfdpeer.ydiscr[1]) << static_cast<int>(v_bfdsess[i]._bfdpeer.ydiscr[2]<<8 | v_bfdsess[i]._bfdpeer.ydiscr[3]));
                    result.push_back(temp_res);
                }
            }
            return result;
        }
        if (idbody == "%"){
            for (size_t i = 0; i < v_bfdsess.size(); i++)
            {
                std::string status;
                if (v_bfdsess[i]._bfdpeer.localstate == 0x00){status = "ADMINDOWN";}
                else{status = "ADMINUP";}
                temp_res = v_bfdsess[i]._bfdpeer.name + " | " + v_bfdsess[i]._bfdpeer.vpn_instance + " | " + std::string(v_bfdsess[i]._bfdpeer.ip_local) + " | " + std::string(v_bfdsess[i]._bfdpeer.ip_peer) + " | " +\
                        status + " | " + std::to_string(v_bfdsess[i]._bfdpeer.senttimer) + " | " + std::to_string(v_bfdsess[i]._bfdpeer.recvtimer) + " | " + \
                        std::to_string(static_cast<int>(v_bfdsess[i]._bfdpeer.mdiscr[0]<<8 | v_bfdsess[i]._bfdpeer.mdiscr[1]) << static_cast<int>(v_bfdsess[i]._bfdpeer.mdiscr[2]<<8 | v_bfdsess[i]._bfdpeer.mdiscr[3])) + " | " + \
                        std::to_string(static_cast<int>(v_bfdsess[i]._bfdpeer.ydiscr[0]<<8 | v_bfdsess[i]._bfdpeer.ydiscr[1]) << static_cast<int>(v_bfdsess[i]._bfdpeer.ydiscr[2]<<8 | v_bfdsess[i]._bfdpeer.ydiscr[3]));
                result.push_back(temp_res);
            }
            return result;
        }
    }
    else{result.push_back("NOTFOUND");}
    return result;
}

std::vector<std::string> dsp(const std::string& tablename, const std::string& idbody)
{
    llog("debug","BfdDbusServer::dsp::" + tablename + "->" + idbody);
    std::vector<std::string> result;
    std::string temp_res = "";
    if(tablename == "bfdsession"){
        if (idbody != "%"){
            for (size_t i = 0; i < v_bfdsess.size(); i++)
            {
                if(v_bfdsess[i]._bfdpeer.name == idbody){
                    std::string status;
                    if (v_bfdsess[i]._bfdpeer.localstate == 0x00){status = "ADMINDOWN";}
                    if (v_bfdsess[i]._bfdpeer.localstate == 0x01){status = "DOWN";}
                    if (v_bfdsess[i]._bfdpeer.localstate == 0x02){status = "INIT";}
                    if (v_bfdsess[i]._bfdpeer.localstate == 0x03){status = "UP";}

                    temp_res = v_bfdsess[i]._bfdpeer.name + " | " + std::string(v_bfdsess[i]._bfdpeer.ip_local) + " | " + std::string(v_bfdsess[i]._bfdpeer.ip_peer) + " | " + \
                        status;

                    result.push_back(temp_res);
                    return result;
                }
            }
        }
        if (idbody == "%"){
            for (size_t i = 0; i < v_bfdsess.size(); i++){
                std::string status;
                if (v_bfdsess[i]._bfdpeer.localstate == 0x00){status = "ADMINDOWN";}
                if (v_bfdsess[i]._bfdpeer.localstate == 0x01){status = "DOWN";}
                if (v_bfdsess[i]._bfdpeer.localstate == 0x02){status = "INIT";}
                if (v_bfdsess[i]._bfdpeer.localstate == 0x03){status = "UP";}

                temp_res = v_bfdsess[i]._bfdpeer.name + " | " + std::string(v_bfdsess[i]._bfdpeer.ip_local) + " | " + std::string(v_bfdsess[i]._bfdpeer.ip_peer) + " | " + \
                status;

                result.push_back(temp_res);
                                
            }
            return result;
        }
    }
    else{result.push_back("NOTFOUND");}
    return result;
}

int32_t rst(const std::string& tablename, const std::string& idbody)
{
    llog("debug","BfdDbusServer::rst::" + tablename + "->" + idbody);
    return 0;    
}

int32_t set(const std::string& tablename, const std::string& idbody)
{
    llog("debug","BfdDbusServer::set::" + tablename + "->" + idbody);
    if (tablename == "bfd"){
        if (idbody == "TRUE"){
            bfdenable = 1;
            th1 = std::thread(&BfdDbusServer::bfdtrack, this);
            llog("debug","BfdDbusServer::set::BFD ENALED");            
            return 0;
        }
        if (idbody == "FALSE"){
            bfdenable = 0;
            killth();
            v_bfdsess.clear();
            v_iddestroypeers.clear();
            llog("debug","BfdDbusServer::set::BFD DISABLE");  
            return 0;
        }
        else {//norm table name, wrong id
            llog("debug","BfdDbusServer::set::bfd->else" + idbody);
            return 2;
        } 
    }
    if (tablename == "bfdloglevel")
    {
        if ((idbody == "error") || (idbody == "debug") || (idbody == "info") || (idbody == "warning"))
        {
            llogSetLevel(idbody);
            llog("debug","BfdDbusServer::set::loglevel->" + idbody);
            return 0;
        }
        else 
        {
            llog("debug","BfdDbusServer::set::bfdloglevel->else" + idbody);
            return 2;
        }
    } 
    llog("debug","BfdDbusServer::set::wrongtablename" + tablename);
    return 1;//wrong table name     
}
private:

std::shared_ptr<logger::Logger> logger;

void killth()
{
    llog("debug","BfdDbusServer::killth->try");
    if (th1.joinable()){
        stopflag = true;
        th1.join();
        stopflag = false;
        llog("debug","BfdDbusServer::killth->finish");
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

void llogSetLevel(std::string level)
{
    try{
        if(level == "debug")
        {
            logger->setLevel(logger::Level::Debug);
            llog("debug","BfdDbusServer::llogSetLevel->" + level);
        }
        if(level == "error")
        {
            logger->setLevel(logger::Level::Error);
            llog("debug","BfdDbusServer::llogSetLevel->" + level);
        }
        if(level == "info")
        {
            logger->setLevel(logger::Level::Info);
            llog("debug","BfdDbusServer::llogSetLevel->" + level);
        }
        if(level == "warning")
        {
            logger->setLevel(logger::Level::Warning);
            llog("debug","BfdDbusServer::llogSetLevel->" + level);
        }
    }
    catch(const std::exception& e)
    {
        if (logger)
        {
            logger->error("dbus::llogSetLevet->Logger Error! ->" + level);
            logger->error(e.what());
        }
        else
        {
            std::cerr << "dbus::llogSetLevet->Error:" << level + ":" << e.what() << '\n';
        }
    }
    catch (...)
    {
        if (logger)
        {
            logger->error("dbus::llogSetLevet->Unknown error\n" + level);
        }
        else
        {
            std::cerr << "dbus::llogSetLevet->Unknown error\n" + level;
        }
    }
}

};

void BfdDbusServer::bfdtrack()
{
    int x, i, j;
    llog("debug","BfdDbusServer::bfdtrack->start"); 
    while(!stopflag){
        x = v_bfdsess.size();
        i = 0;
        
        while (i < x){
            v_bfdsess[i].mainprocess(); //сам процесс BFD
            if((v_bfdsess[i]._bfdpeer.localstate == 0x00)||(v_bfdsess[i]._bfdpeer.localstate == 0x01)){
                //REMOVE ROUTE
                if (findid(i, v_iddestroypeers) == 0){
                    destroyroute(i);//как то рисковано работать с ID вектора....
                    llog("debug","BfdDbusServer::bfdtrack->destroyroute->" + v_bfdsess[i]._bfdpeer.name);
                    llog("warning","BfdDbusServer::bfdtrack->destroyroute->" + v_bfdsess[i]._bfdpeer.name); 
                    v_iddestroypeers.push_back(i);
                }
            } 
            i++;
        }
        j = 0;
        while(j < v_iddestroypeers.size()){//если пир поднялся оживить руты
            v_bfdsess[v_iddestroypeers[j]].mainprocess();
            if(v_bfdsess[v_iddestroypeers[j]]._bfdpeer.localstate == 0x03){
                //RECOVERY ROUTE
                llog("debug","BfdDbusServer::bfdtrack->destroyroute->" + v_bfdsess[v_iddestroypeers[j]]._bfdpeer.name);
                recoveryroute(v_iddestroypeers[j]);
                v_iddestroypeers.erase(v_iddestroypeers.begin() + j);
            }
            j++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

}

void BfdDbusServer::makebfd(const char *ip_local, const char *ip_peer, std::string name, std::string vpn_instalnce)
{
    llog("debug","BfdDbusServer::makebfd");
    char * char_ip_loc = new char[strlen(ip_local) + 1];
    strcpy(char_ip_loc, ip_local);
    char * char_ip_peer = new char[strlen(ip_peer) + 1];
    strcpy(char_ip_peer, ip_peer);
    v_bfdsess.push_back(Bfd(char_ip_loc, 3784, char_ip_peer, 10, name, vpn_instalnce, logger));
    v_bfdsess[v_bfdsess.size()-1].mainprocess();
    llog("debug","BfdDbusServer::makebfd->vectorsize->" + std::to_string(v_bfdsess.size()));
    delete[] char_ip_loc, char_ip_peer;

}

bool BfdDbusServer::findid(int id, std::vector<int> v)
{
    for (int i; i<v.size(); i++){
        if (v[i] == id){
            return 1;
        }
    }
    return 0;
}

void BfdDbusServer::destroyroute(int vector_id)
{
    //signal kill route
    std::vector<std::string> v_string;
    v_string.push_back(v_bfdsess[vector_id]._bfdpeer.name);
    v_string.push_back(v_bfdsess[vector_id]._bfdpeer.vpn_instance);
    llog("debug","BfdDbusServer::destroyroute->" + v_bfdsess[vector_id]._bfdpeer.name);
    emitDestRoute(v_string);
}

void BfdDbusServer::recoveryroute(int vector_id)
{
    //signal recovery route
    std::vector<std::string> v_string;
    v_string.push_back(v_bfdsess[vector_id]._bfdpeer.name);
    v_string.push_back(v_bfdsess[vector_id]._bfdpeer.vpn_instance);
    llog("debug","BfdDbusServer::recoveryroute->" + v_bfdsess[vector_id]._bfdpeer.name);
    emitRecRoute(v_string);
}


