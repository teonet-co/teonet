/*
 * Copyright (c) 1996-2017 Kirill Scherba <kirill@scherba.ru>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * File:   teonet.hpp
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on October 18, 2017, 11:03 AM
 *
 * Teonet c++ wrapper
 */

/**
 * \mainpage Teonet library Documentation
 *
 * ### Installation information
 * See the \ref md_README "README.md" file to get Installation Info
 * <br>
 * ### See the latest news from library ChangeLog: <br>
 * \include "./ChangeLog"
 *
 */

#pragma once

#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <memory>

#include "ev_mgr.h"
#include "modules/teodb_com.h"
#include "modules/subscribe.h"

namespace teo {

typedef ksnetEvMgrEvents teoEvents;     //! Teonet Events
typedef ksnCorePacketData teoPacket;    //! Teonet Packet
typedef ksnetEvMgrAppParam teoAppParam; //! Teonet App parameters
typedef struct teoL0Client {            //! Teonet L0 Client address

    const char *name;
    uint8_t name_len;
    const char *addr;
    int port;

    teoL0Client(const char *name, uint8_t name_len, const char *addr, int port) :
    name(strdup(name)), name_len(name_len), addr(strdup(addr)), port(port) {}

    virtual ~teoL0Client() {
        free((void*)name);
        free((void*)addr);
    }

} teoL0Client;

/**
 * Teonet class.
 *
 * This is Teonet L0 client C++ wrapper
 */
class Teonet {

public:

    /**
     * Teonet event callback
     */
    typedef void (*teoEventsCb)(Teonet &teo, teoEvents event, void *data,
                         size_t data_len, void *user_data);

private:

    ksnetEvMgrClass *ke;                //! Pointer to ksnetEvMgrClass
    teoEventsCb event_cb;               //! Pointer to c++ callback function
    const char *classVersion = "0.0.4"; //! Teonet class version

public:

    ksnetEvMgrClass* getKe() const {
        return ke;
    }

public:

    /**
     * Teonet constructor
     *
     * Initialize KSNet Event Manager and network and set new default port
     *
     * @param argc Number of applications arguments (from main)
     * @param argv Applications arguments array (from main)
     * @param event_cb Events callback function called when an event happens
     * @param options Options set: \n
     *                READ_OPTIONS - read options from command line parameters; \n
     *                READ_CONFIGURATION - read options from configuration file
     * @param port Set default port number if non 0
     * @param user_data Pointer to user data or NULL if absent
     *
     */
    Teonet(int argc, char** argv,
            teoEventsCb event_cb = NULL,
            int options = READ_ALL,
            int port = 0,
            void *user_data = NULL) {

        this->event_cb = event_cb;
        ke = ksnetEvMgrInitPort(argc, argv, [](ksnetEvMgrClass *ke, teoEvents e,
            void *data, size_t data_len, void *user_data) {
              ((Teonet*) ke->teo_class)->eventCb(e, data, data_len, user_data);
            }, options, port, user_data);
        if(ke) ke->teo_class = this;
    }

    /**
     * Teonet simple destructor
     */
    virtual ~Teonet() { /*std::cout << "Destructor Teonet\n";*/ }

    /**
     * Set Teonet application type
     *
     * @param type Application type string
     */
    inline void setAppType(const char *type) {
        teoSetAppType(ke, (char*)type);
    }

    /**
     * Set Teonet application version
     *
     * @param version Application version string
     */
    inline void setAppVersion(const char *version) {
        teoSetAppVersion(ke, (char*)version);
    }

    /**
     * Start Teonet Event Manager and network communication
     *
     * @return Alway 0
     */
    inline int run() {
        return ksnetEvMgrRun(ke);
    }

    /**
     * Send command by name to peer
     *
     * @param kc Pointer to ksnCoreClass
     * @param to Peer name to send to
     * @param cmd Command
     * @param data Commands data
     * @param data_len Commands data length
     * @return Pointer to ksnet_arp_data or NULL if to peer is absent
     */
    inline ksnet_arp_data *sendTo(const char *to, uint8_t cmd, void *data,
        size_t data_len) const {

        return ksnCoreSendCmdto(ke->kc, (char*)to, cmd, data, data_len);
    }

    inline ksnet_arp_data *sendTo(const char *to, uint8_t cmd, const std::string &data) const {
        return sendTo((char*)to, cmd, (void*)data.c_str(), data.size() + 1);
    }
    /**
     * Send command by name to peer(async)
     *
     * @param kc Pointer to ksnCoreClass
     * @param to Peer name to send to
     * @param cmd Command
     * @param data Commands data
     * @param data_len Commands data length
     */
    inline void sendToA(const char *to, uint8_t cmd, void *data,
        size_t data_len) const {

        ksnCoreSendCmdtoA((void *)ke, to, cmd, data, data_len);
    }

    inline void sendToA(const char *to, uint8_t cmd, const std::string &msg) const {
        sendToA(to, cmd, (void*)msg.c_str(), msg.size() + 1);
    }

    inline void sendAnswerTo(teo::teoPacket *rd, const char *name, void *out_data, size_t out_data_len) const {
        sendCmdAnswerTo(ke, rd, (char*)name, out_data, out_data_len);
    }

    inline void sendAnswerTo(teo::teoPacket *rd, const char *name, const std::string &out_data) const {
        sendAnswerTo(rd, (char*)name, (void*)out_data.c_str(), out_data.size() + 1);
    }

    inline void sendAnswerToA(teo::teoPacket *rd, uint8_t cmd, const std::string &out_data) const {
        sendCmdAnswerToBinaryA(ke, rd, cmd, (void*)out_data.c_str(), out_data.size() + 1);
    }

    /**
     * Send ECHO command to peer
     *
     * @param to Peer name to send to
     * @param cmd Command
     * @param data Commands data
     * @param data_len Commands data length
     * @return True at success
     */
    inline int sendEchoTo(const char *to, uint8_t cmd, void *data,
      size_t data_len) const {

      return ksnCommandSendCmdEcho(ke->kc->kco, (char*)to, data, data_len);
    }

    /**
     * Send data to L0 client. Usually it is an answer to request from L0 client
     *
     * @param addr IP address of remote peer
     * @param port Port of remote peer
     * @param cname L0 client name (include trailing zero)
     * @param cname_length Length of the L0 client name
     * @param cmd Command
     * @param data Data
     * @param data_len Data length
     *
     * @return Return 0 if success; -1 if data length is too lage (more than 32319)
     */
    inline int sendToL0(const char *addr, int port, const char *cname,
        size_t cname_length, uint8_t cmd, void *data, size_t data_len) const {

        return ksnLNullSendToL0(ke, (char*)addr, port, (char*)cname,
          cname_length, cmd, data, data_len);
    }

    /**
     * Send data to L0 client. Usually it is an answer to request from L0 client(async)
     *
     * @param addr IP address of remote peer
     * @param port Port of remote peer
     * @param cname L0 client name (include trailing zero)
     * @param cname_length Length of the L0 client name
     * @param cmd Command
     * @param data Data
     * @param data_len Data length
     *
     */
    inline void sendToL0A(const char *addr, int port, const char *cname,
        size_t cname_length, uint8_t cmd, void *data, size_t data_len) const {
        ksnCorePacketData rd;
        rd.from = (char *)cname;
        rd.from_len = cname_length;
        rd.addr = (char *)addr;
        rd.port = port;
        rd.l0_f = 1;
        sendCmdAnswerToBinaryA((void *)ke, &rd, cmd, data, data_len);
    }
    
    /**
     * Send data to L0 client with the teoL0Client structure.
     *
     * @param l0cli Pointer to teoL0Client
     * @param cmd Command
     * @param data Data
     * @param data_len Data length
     *
     * @return Return 0 if success; -1 if data length is too lage (more than 32319)
     */
    inline int sendToL0(teoL0Client* l0cli, uint8_t cmd, void *data,
        size_t data_len) const {

        return sendToL0(l0cli->addr,l0cli->port, l0cli->name, l0cli->name_len,
                cmd, data, data_len);
    }

    /**
     * Send ECHO command to L0 client
     *
     * @param addr IP address of remote peer
     * @param port Port of remote peer
     * @param cname L0 client name (include trailing zero)
     * @param cname_length Length of the L0 client name
     * @param cmd Command
     * @param data Data
     * @param data_len Data length
     *
     * @return Return 0 if success; -1 if data length is too lage (more than 32319)
     */
    inline int sendEchoToL0(const char *addr, int port, const char *cname,
        size_t cname_length, void *data, size_t data_len) const {

      return ksnLNullSendEchoToL0(ke, (char*)addr, port, (char*)cname,
        cname_length, data, data_len);
    }

    /**
     * Send ECHO command to L0 client with teoL0Client structure
     *
     * @param l0cli Pointer to teoL0Client
     * @param data Data
     * @param data_len Data length
     *
     * @return Return 0 if success; -1 if data length is too lage (more than 32319)
     */
    inline int sendEchoToL0(teoL0Client* l0cli, void *data, size_t data_len) const {
      return sendEchoToL0(l0cli->addr, l0cli->port, l0cli->name,
                l0cli->name_len, data, data_len);
    }


    inline void subscribe(const char *peer_name, uint16_t ev) const {
        teoSScrSubscribe((teoSScrClass*)ke->kc->kco->ksscr, (char*)peer_name, ev);
    }

    inline void subscribeA(const char *peer_name, uint16_t ev) const {
        teoSScrSubscribeA((teoSScrClass*)ke->kc->kco->ksscr, (char*)peer_name, ev);
    }
    
    inline void sendToSscr(uint16_t ev, void *data, size_t data_length, uint8_t cmd = 0) const {
        teoSScrSend((teoSScrClass*)ke->kc->kco->ksscr, ev, data,data_length, cmd);
    }

    inline void sendToSscr(uint16_t ev, const std::string &data, uint8_t cmd = 0) const {
        sendToSscr(ev, (void*)data.c_str(), data.size() + 1, cmd);
    }
    
    inline void sendToSscrA(uint16_t ev, const std::string &data, uint8_t cmd = 0) const {
        teoSScrSendA(ke, ev, (void*)data.c_str(), data.size() + 1, cmd);
    }
    
    /**
     * Set custom timer interval
     *
     * @param time_interval
     */
    inline void setCustomTimer(double time_interval = 2.00) {
        ksnetEvMgrSetCustomTimer(ke, time_interval);
    }

    /**
     * Get this class version
     *
     * @return
     */
    inline const char* getClassVersion() const {
        return classVersion;
    }

    /**
     * Get Teonet library version
     *
     * @return
     */
    static inline const char* getTeonetVersion() {
        return teoGetLibteonetVersion();
    }

    /**
     * Return host name
     *
     * @return
     */
    inline const char* getHostName() const {
        return ksnetEvMgrGetHostName(ke);
    }

    /**
     * Get KSNet event manager time
     *
     * @param ke Pointer to ksnetEvMgrClass
     *
     * @return
     */
    inline double getTime() const {
      return ksnetEvMgrGetTime(ke);
    }
    /**
     * Stop Teonet event manager
     *
     */
    inline void stop() {
        ksnetEvMgrStop(ke);
    }

    /**
     * Cast data to teo::teoPacket
     *
     * @param data Pointer to data
     * @return Pointer to teo::teoPacket
     */
    inline teo::teoPacket *getPacket(void *data) const {
        return (teo::teoPacket*) data;
    }

    /**
     * Virtual Teonet event callback
     *
     * @param event
     * @param data
     * @param data_len
     * @param user_data
     */
    virtual inline void eventCb(teo::teoEvents event, void *data,
        size_t data_len, void *user_data) {

        if(event_cb) event_cb(*this, event, data, data_len, user_data);
    };

    virtual void cqueueCb(uint32_t id, int type, void *data) {

    }

    #ifdef DEBUG_KSNET
    #define teo_printf(module, type, format, ...) \
        ksnet_printf(&((getKe())->ksn_cfg), type, \
            _ksn_printf_format_(format), \
            _ksn_printf_type_(type), \
            module == NULL ? (getKe())->ksn_cfg.app_name : module, \
            __func__, __FILE__, __LINE__, __VA_ARGS__)

    #define teo_puts(module, type, format) \
        ksnet_printf(&((getKe())->ksn_cfg), type, \
            _ksn_printf_format_(format) "\n", \
            _ksn_printf_type_(type), \
            module == NULL ? (getKe())->ksn_cfg.app_name : module, \
            __func__, __FILE__, __LINE__)
    #else
    #define teo_printf(module, type, format, ...) ;
    #define teo_puts(module, type, format) ;
    #endif

typedef ksnCQueData cqueData;           //! Teonet CQue data structure
typedef ksnCQueCallback cqueCallback;   //! Teonet CQue callback function

/**
 * Teonet CQue class
 */
class CQue {

public:

typedef std::unique_ptr<CQue> cquePtr;

private:

    Teonet *teo;
    ksnetEvMgrClass *ke;

public:

    CQue(Teonet *t) : teo(t), ke(teo->getKe()) { /*std::cout << "CQue::CQue\n";*/  }
    CQue(const CQue &obj) : teo(obj.teo), ke(obj.ke) { /*std::cout << "CQue::CQue copy\n";*/ }
    virtual ~CQue() { /*std::cout << "~CQue::CQue\n";*/ }

    /**
     * Add callback to queue
     *
     * @param cb Callback [function](@ref ksnCQueCallback) or NULL. The teonet event
     *           EV_K_CQUE_CALLBACK should be send at the same time.
     * @param timeout Callback timeout. If equal to 0 than timeout sets automatically to 5 sec
     * @param user_data The user data which should be send to the Callback function
     *
     * @return Pointer to added ksnCQueData or NULL if error occurred
     */

    template<typename Callback, typename T>
    cqueData *add(Callback cb, T timeout = 5.00,
        void *user_data = NULL) {

        struct userData { void *user_data; Callback cb; };
        userData *ud = new userData { user_data, cb };
        return ksnCQueAdd(ke->kq, [](uint32_t id, int type, void *data) {

            auto ud = (userData*) data;
            ud->cb(id, type, ud->user_data);
            delete(ud);

        }, (double)timeout, ud);
    }

    template<typename T>
    inline cqueData * add(cqueCallback cb, T timeout = 5.00,
        void *user_data = NULL) {

        return ksnCQueAdd(ke->kq, cb, (double)timeout, user_data);
    }

    template<typename Callback>
    inline void *find(void *find, const Callback compare, size_t *key_length = NULL) {
        return ksnCQueFindData(ke->kq, find, compare, key_length);
    }

    template<typename Callback>
    inline void *find(const std::string& find, const Callback compare, size_t *key_length = NULL) {
        return ksnCQueFindData(ke->kq, (void*)find.c_str(), compare, key_length);
    }
    
    /**
     * Execute callback queue record
     *
     * Execute callback queue record and remove it from queue
     *
     * @param id Required ID
     *
     * @return return 0: if callback executed OK; !=0 some error occurred
     */
    inline int exec(uint32_t id) {
        return ksnCQueExec(ke->kq, id);
    }

    /**
     * Get pointer to CQue data structure
     *
     * @param data Pointer to data at EV_K_CQUE_CALLBACK event
     *
     * @return Pointer to CQue data structure
     */
    inline cqueData *getData(void *data) const {
        return (cqueData *)data;
    }

    /**
     * Get CQue data id
     *
     * @param cd Pointer to CQue data structure
     * @return CQue id
     */
    inline uint32_t getId(cqueData *cd) const {
        return cd->id;
    }

    /**
     * Get pointer to teonet class object
     *
     * @return Pointer to teonet class object
     */
    inline Teonet* getTeonet() const {
        return teo;
    }

    /**
     * Set callback queue data, update data set in ksnCQueAdd
     *
     * @param id Existing callback queue ID
     * @param data Pointer to new user data
     *
     * @return return 0: if data set OK; !=0 some error occurred
     */
    int setUserData(uint32_t id, void *data) {
      int retval = -1;
      struct userData { void *user_data; void *cb; };
      auto ud = (userData *) ksnCQueGetData(ke->kq, id);
      if(ud) {
        ud->user_data = data;
        retval = 0;
      }
      return retval;
    }
};

/**
 * CQue class factory(maker)
 *
 * @return Unique pointer to CQue object
 */
inline CQue::cquePtr getCQueP() {
    return getCQue<std::unique_ptr<teo::Teonet::CQue>>();
}

/**
 * CQue class factory(maker)
 *
 * @return Reference to CQue object
 */
inline CQue &getCQueR() {
    static auto cque = getCQueP();
    return *cque;
}

/**
 * CQue class factory(maker)
 *
 * @return T
 */
template<typename T> T getCQue() {
    T cque(new CQue(this));
    return cque;
}

class TeoDB {

public:

    typedef teo_db_data teoDbData;
    typedef teo_db_data_range teoDbDataRange;
    typedef struct teoDbCQueData {

        TeoDB *teodb;
        teoDbData **tdd;
        teoL0Client *l0client;
        uint8_t cb_type;
        void *user_data;

        teoDbCQueData(TeoDB *teoDb, teoDbData **tdd, teoL0Client * l0client = NULL,
            uint8_t cb_type = 0) : teodb(teoDb), tdd(tdd), l0client(l0client),
            cb_type(cb_type) {}

        virtual ~teoDbCQueData() {
            if(l0client) delete(l0client);
        }

        static inline teoL0Client* setl0Cli(const char *name, uint8_t name_len,
          const char *addr, int port) {
            return new teoL0Client((char*)name, name_len, (char*)addr, port);
        }
        static inline teoL0Client* setl0Cli(teo::teoPacket* rd) {
            return rd ? setl0Cli(rd->from, rd->from_len, rd->addr, rd->port) : NULL;
        }

        inline auto getId() const -> decltype ((*tdd)->id) {
            return (*tdd)->id;
        }

        inline auto getCbType() const -> decltype (cb_type) {
            return cb_type;
        }

        inline auto getL0Client() const -> decltype (l0client) {
            return l0client;
        }

        inline void *getKey() const {
            return (*tdd)->key_data;
        }
        inline char *getKeyStr() const {
            return (char*) getKey();
        }

        inline void *getValue() const {
            return (*tdd)->key_data + (*tdd)->key_length;
        }
        inline char *getValueStr() const {
            return (char*) getValue();
        }

    } teoDbCQueData;

private:

    Teonet *teo;
    teoDbData **tdd;
    std::string peer;
    CQue cque = CQue(teo);

public:

    TeoDB(Teonet *t, const std::string peer, teoDbData **tdd) : teo(t), tdd(tdd), peer(peer) { /*std::cout << "TeoDB::TeoDB\n";*/  }
    TeoDB(const TeoDB &obj) : teo(obj.teo), tdd(obj.tdd), peer(obj.peer) { /*std::cout << "TeoDB::TeoDB copy\n";*/ }
    virtual ~TeoDB() { /*std::cout << "~TeoDB::TeoDB\n";*/ }

    /**
     * Get pointer to teonet class object
     *
     * @return Pointer to teonet class object
     */
    inline Teonet* getTeonet() const {
        return teo;
    }

    inline ksnetEvMgrClass* getKe() const {
        return getTeonet()->getKe();
    }

    inline CQue* getCQue()  {
        return &cque;
    }

    inline const char* getPeer() const {
        return peer.c_str();
    }

    inline bool checkPeer(const char *peer_name) {
        return !strcmp(peer.c_str(), peer_name);
    }

    inline std::unique_ptr<teoDbData> prepareRequest(const void *key, size_t key_len,
        const void *data, size_t data_len, uint32_t id, size_t *tdd_len)    {

        return (std::unique_ptr<teoDbData>) prepare_request_data(key, key_len,
                data, data_len, id, tdd_len);
    }

    static inline teoDbData* getData(teo::teoPacket *rd) {
        return (teoDbData*) (rd ? rd->data : NULL);
    }

    inline ksnet_arp_data *send(uint8_t cmd, const void *key, size_t key_len,
        const void *data = NULL, size_t data_len = 0) {

        return sendTDD(cmd, key, key_len, data, data_len);
    }

    template<typename Callback>
    ksnet_arp_data *send(uint8_t cmd, const void *key, size_t key_len,
        Callback cb, double timeout,
        uint8_t cb_type, teo::teoPacket *rd = NULL,
        teoDbData **tdd = NULL, void *user_data = NULL) {

        // Create user data if it  is NULL
        if(!user_data) {
            if(!tdd) tdd = this->tdd;
            user_data = new teoDbCQueData(this, tdd, teoDbCQueData::setl0Cli(rd), cb_type);
        }

        // Add callback to queue and wait timeout after 5 sec ...
        struct userData { void *user_data; Callback cb; };
        auto ud = new userData { user_data, cb };
        auto cq = cque.add([](uint32_t id, int type, void *data) {
            auto ud = (userData *) data;
            ud->cb(id, type, ud->user_data);
            delete((TeoDB::teoDbCQueData*)ud->user_data);
            delete(ud);
        }, timeout, ud);
        teo_printf(NULL, DEBUG, "Register callback id %u\n", cq->id);
        return sendTDD(cmd, key, key_len, NULL, 0, cq->id);
    }

    ksnet_arp_data *send(uint8_t cmd, const void *key, size_t key_len,
        cqueCallback cb, double timeout,
        uint8_t cb_type, teo::teoPacket *rd = NULL,
        teoDbData **tdd = NULL, void *user_data = NULL) {

        // Create user data if it  is NULL
        if(!user_data) {
            if(!tdd) tdd = this->tdd;
            user_data = new teoDbCQueData(this, tdd, teoDbCQueData::setl0Cli(rd), cb_type);
        }

        // Add callback to queue and wait timeout after 5 sec ...
        std::cout << "!!! cqueCallback !!!\n";
        auto cq = cque.add(cb, timeout, user_data);
        teo_printf(NULL, DEBUG, "Register simple callback id %u\n", cq->id);
        return sendTDD(cmd, key, key_len, NULL, 0, cq->id);
    }

    inline int exec(uint32_t id) {
        return cque.exec(id);
    }

    bool process(teoEvents event, void *data) {

        bool processed = false;
        auto rd = teo->getPacket(data);
        *tdd = getData(rd);

        // Check teonet event
        switch(event) {

            case EV_K_RECEIVED:

                // DATA event
                if(rd && checkPeer(rd->from)) {

                    switch(rd->cmd) {

                        // Get data response #132
                        case CMD_D_GET_ANSWER:
                        // Get list response #133
                        case CMD_D_LIST_ANSWER:
                        // Get list length response #135
                        case CMD_D_LIST_LENGTH_ANSWER:
                        // Get List range response #137
                        case CMD_D_LIST_RANGE_ANSWER:

                            if(*tdd && (*tdd)->id && !exec((*tdd)->id)) processed = true;
                            break;
                    }
                }
                break;

            default:
                break;
        }

        return processed;
    }

private:

    ksnet_arp_data *sendTDD(uint8_t cmd, const void *key, size_t key_len,
        const void *data = NULL, size_t data_len = 0, uint32_t id = 0) {

        size_t tdd_len;
        auto tddr = prepareRequest(key, key_len, data, data_len, id, &tdd_len);
        return teo->sendTo(peer.c_str(), cmd, tddr.get(), tdd_len);
        /*auto tddr = prepare_request_data(key, key_len, data, data_len, id, &tdd_len);
        auto retval = teo->sendTo(peer.c_str(), cmd, tddr, tdd_len);
        free(tddr);
        return retval;*/
    }
};

};

/**
 * Teonet host info processing class
 */
struct HostInfo {


    std::string name;
    struct {
        std::vector<std::string> v;
        std::string to_string() {
            std::string type_str = "";
            for(size_t i = 0; i < v.size(); i++) {
                type_str += v[i] + (i < v.size() - 1 ? ",":"");
            }
            return type_str;
        }
    } type;
    std::string version;
    std::string teonetVersion;

    void makeTeonetVersion(uint8_t *ver, int num_dig = DIG_IN_TEO_VER) {
        teonetVersion = "";
        for(auto i=0; i < num_dig; i++) {
            teonetVersion += std::to_string(ver[i]) +
                    (i < num_dig - 1 ? "." : "");
        }
    }

    HostInfo(void *data) {
        auto host_info = (host_info_data *) data;
        makeTeonetVersion(host_info->ver);
        size_t ptr = 0;
        for(auto i = 0; i <= host_info->string_ar_num; i++)  {
            if(!i) name = host_info->string_ar + ptr;
            else if(i == host_info->string_ar_num) version = host_info->string_ar + ptr;
            else type.v.push_back(host_info->string_ar + ptr);
            ptr += std::char_traits<char>::length(host_info->string_ar + ptr) + 1;
        }
    }
};

}
