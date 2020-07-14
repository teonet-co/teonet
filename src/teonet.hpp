/*
 * Copyright (c) 1996-2019 Kirill Scherba <kirill@scherba.ru>
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

//#pragma once
#ifndef THIS_TEONET_H
#define THIS_TEONET_H

#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ev_mgr.h"
#include "modules/log_reader.h"
#include "modules/subscribe.h"
#include "modules/teodb_com.h"

namespace teo {

typedef ksnetEvMgrEvents teoEvents;     //! Teonet Events
typedef ksnCorePacketData teoPacket;    //! Teonet Packet
typedef ksnetEvMgrAppParam teoAppParam; //! Teonet App parameters
typedef struct teoL0Client {            //! Teonet L0 Client address

  const char* name;
  uint8_t name_len;
  const char* addr;
  int port;

  teoL0Client(const char* name, uint8_t name_len, const char* addr, int port)
      : name(strdup(name)), name_len(name_len), addr(strdup(addr)), port(port) {}

  teoL0Client(const std::string& name, const std::string& addr, int port)
      : teoL0Client(name.c_str(), name.size() + 1, addr.c_str(), port) {}

  virtual ~teoL0Client() {
    free((void*)name);
    free((void*)addr);
  }

} teoL0Client;

/**
 * The unique_raw_ptr::make make unique_ptr from allocated raw pointer,
 * and free this pointer pointer when the unique_ptr does not used more.
 */
namespace unique_raw_ptr {

template <typename T> struct destroy {
  void operator()(T x) { free((void*)x); }
};

template <typename T> std::unique_ptr<T[], destroy<T*>> make(T* raw_data) {
  std::unique_ptr<T[], destroy<T*>> str_ptr(raw_data);
  return std::move(str_ptr);
}
} // namespace unique_raw_ptr

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
  typedef void (*teoEventsCb)(Teonet& teo, teoEvents event, void* data, size_t data_len,
                              void* user_data);

private:
  ksnetEvMgrClass* ke;                //! Pointer to ksnetEvMgrClass
  teoEventsCb event_cb;               //! Pointer to c++ callback function
  const char* classVersion = "0.0.4"; //! Teonet class version

public:
  inline ksnetEvMgrClass* getKe() const { return ke; }

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
  Teonet(int argc, char** argv, teoEventsCb event_cb = NULL, int options = READ_ALL, int port = 0,
         void* user_data = NULL) {

    this->event_cb = event_cb;
    ke = ksnetEvMgrInitPort(
        argc, argv,
        [](ksnetEvMgrClass* ke, teoEvents e, void* data, size_t data_len, void* user_data) {
          ((Teonet*)ke->teo_class)->eventCb(e, data, data_len, user_data);
        },
        options, port, user_data);
    if(ke) ke->teo_class = this;
  }

  /**
   * Teonet simple destructor
   */
  virtual ~Teonet() { /*std::cout << "Destructor Teonet\n";*/
  }

  /**
   * Set Teonet application type
   *
   * @param type Application type string
   */
  inline void setAppType(const char* type) { teoSetAppType(ke, (char*)type); }

  /**
   * Set Teonet application version
   *
   * @param version Application version string
   */
  inline void setAppVersion(const char* version) { teoSetAppVersion(ke, (char*)version); }

  /**
   * Start Teonet Event Manager and network communication
   *
   * @return Always 0
   */
  inline int run() { return ksnetEvMgrRun(ke); }

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
  void BroadcastSend(const char* to, uint8_t cmd, void* data, size_t data_len) const {
    teoBroadcastSend(ke->kc, (char*)to, cmd, data, data_len);
  }
  inline ksnet_arp_data* sendTo(const char* to, uint8_t cmd, void* data, size_t data_len) const {
    return ksnCoreSendCmdto(ke->kc, (char*)to, cmd, data, data_len);
  }
  inline ksnet_arp_data* sendTo(const char* to, uint8_t cmd, const std::string& data) const {
    return sendTo((char*)to, cmd, (void*)data.c_str(), data.size() + 1);
  }
  /**
   * Send command by name to peer(asynchronously)
   *
   * @param kc Pointer to ksnCoreClass
   * @param to Peer name to send to
   * @param cmd Command
   * @param data Commands data
   * @param data_len Commands data length
   */
  inline void sendToA(const char* to, uint8_t cmd, void* data, size_t data_len) const {
    ksnCoreSendCmdtoA((void*)ke, to, cmd, data, data_len);
  }
  inline void sendToA(const char* to, uint8_t cmd, const std::string& data) const {
    sendToA(to, cmd, (void*)data.c_str(), data.size() + 1);
  }

  /**
   * Sent teonet command to peer or l0 client depend of input rd
   *
   * @param ke Pointer to ksnetEvMgrClass
   * @param rd Pointer to rd
   * @param name Name to send to
   * @param data
   * @param data_len
   *
   * @return
   */
  inline void sendAnswerTo(teo::teoPacket* rd, const char* name, void* data,
                           size_t data_len) const {
    sendCmdAnswerTo(ke, rd, (char*)name, data, data_len);
  }
  inline void sendAnswerTo(teo::teoPacket* rd, const char* name, const std::string& data) const {
    sendAnswerTo(rd, (char*)name, (void*)data.c_str(), data.size() + 1);
  }
  /**
   * Sent teonet command to peer or l0 client depend of input rd (asynchronously)
   *
   * @param ke Pointer to ksnetEvMgrClass
   * @param rd Pointer to rd
   * @param name Name to send to
   * @param data
   * @param data_len
   *
   * @return
   */
  inline void sendAnswerToA(teo::teoPacket* rd, uint8_t cmd, void* data, size_t data_len) const {
    sendCmdAnswerToBinaryA(ke, rd, cmd, (void*)data, data_len);
  }
  inline void sendAnswerToA(teo::teoPacket* rd, uint8_t cmd, const std::string& data) const {
    sendCmdAnswerToBinaryA(ke, rd, cmd, (void*)data.c_str(), data.size() + 1);
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
  int sendEchoTo(const char* to, void* data = NULL, size_t data_len = 0) const {
    if(!data || !data_len) {
      data = (void*)"Hello Teonet!";
      data_len = std::strlen((char*)data) + 1;
    }
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
  inline int sendToL0(const char* addr, int port, const char* cname, size_t cname_length,
                      uint8_t cmd, void* data, size_t data_len) const {
    return ksnLNullSendToL0(ke, (char*)addr, port, (char*)cname, cname_length, cmd, data, data_len);
  }
  inline int sendToL0(const char* addr, int port, const std::string& cname, uint8_t cmd, void* data,
                      size_t data_len) const {
    return sendToL0(addr, port, cname.c_str(), cname.size() + 1, cmd, data, data_len);
  }
  inline int sendToL0(const char* addr, int port, const std::string& cname, uint8_t cmd,
                      const std::string& data) const {
    return sendToL0(addr, port, cname.c_str(), cname.size() + 1, cmd, (void*)data.c_str(),
                    data.size() + 1);
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
  inline int sendToL0(teoL0Client* l0cli, uint8_t cmd, void* data, size_t data_len) const {

    return sendToL0(l0cli->addr, l0cli->port, l0cli->name, l0cli->name_len, cmd, data, data_len);
  }

  /**
   * Send data to L0 client. Usually it is an answer to request from L0 client(asynchronously)
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
  inline void sendToL0A(const char* addr, int port, const char* cname, size_t cname_length,
                        uint8_t cmd, void* data, size_t data_len) const {
    ksnCorePacketData rd;
    rd.from = (char*)cname;
    rd.from_len = cname_length;
    rd.addr = (char*)addr;
    rd.port = port;
    rd.l0_f = 1;
    sendCmdAnswerToBinaryA((void*)ke, &rd, cmd, data, data_len);
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
  inline int sendEchoToL0(const char* addr, int port, const char* cname, size_t cname_length,
                          void* data, size_t data_len) const {

    return ksnLNullSendEchoToL0(ke, (char*)addr, port, (char*)cname, cname_length, data, data_len);
  }

  inline int sendEchoToL0A(const char* addr, int port, const char* cname, size_t cname_length,
                           void* data, size_t data_len) const {

    return ksnLNullSendEchoToL0A(ke, (char*)addr, port, (char*)cname, cname_length, data, data_len);
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
  inline int sendEchoToL0(teoL0Client* l0cli, void* data, size_t data_len) const {
    return sendEchoToL0(l0cli->addr, l0cli->port, l0cli->name, l0cli->name_len, data, data_len);
  }
  inline int sendEchoToL0A(teoL0Client* l0cli, void* data, size_t data_len) const {
    return sendEchoToL0A(l0cli->addr, l0cli->port, l0cli->name, l0cli->name_len, data, data_len);
  }

  inline void subscribe(const char* peer_name, uint16_t ev) const {
    teoSScrSubscribe((teoSScrClass*)ke->kc->kco->ksscr, (char*)peer_name, ev);
  }
  inline void subscribeA(const char* peer_name, uint16_t ev) const {
    teoSScrSubscribeA((teoSScrClass*)ke->kc->kco->ksscr, (char*)peer_name, ev);
  }

  inline void sendToSscr(uint16_t ev, void* data, size_t data_length, uint8_t cmd = 0) const {
    teoSScrSend((teoSScrClass*)ke->kc->kco->ksscr, ev, data, data_length, cmd);
  }
  inline void sendToSscr(uint16_t ev, const std::string& data, uint8_t cmd = 0) const {
    sendToSscr(ev, (void*)data.c_str(), data.size() + 1, cmd);
  }
  inline void sendToSscrA(uint16_t ev, const std::string& data, uint8_t cmd = 0) const {
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
  inline const char* getClassVersion() const { return classVersion; }

  /**
   * Get Teonet library version
   *
   * @return
   */
  static inline const char* getTeonetVersion() { return teoGetLibteonetVersion(); }

  /**
   * Return host name
   *
   * @return
   */
  inline const char* getHostName() const { return ksnetEvMgrGetHostName(ke); }

  /**
   * Create Application parameters
   *
   * @param params
   * @param params_description
   * @return
   */
  static teo::teoAppParam* appParam(std::vector<const char*> params,
                                    std::vector<const char*> params_description) {

    params.insert(params.begin(), "");
    params_description.insert(params_description.begin(), "");

    static teo::teoAppParam app_param;
    app_param.app_argc = params.size();
    app_param.app_argv = reinterpret_cast<const char**>(params.data());
    app_param.app_descr = reinterpret_cast<const char**>(params_description.data());
    return &app_param;
  };

  /**
   * Get additional application command line parameter defined in teoAppParam
   *
   * @param parm_number Number of application parameter (started from 1)
   * @return
   */
  inline const char* getParam(int parm_number) const {
    return getKe()->ksn_cfg.app_argv[parm_number];
  }

  /**
   * Get KSNet event manager time
   *
   * @param ke Pointer to ksnetEvMgrClass
   *
   * @return
   */
  inline double getTime() const { return ksnetEvMgrGetTime(ke); }

  /**
   * Get path to teonet data folder
   *
   * @return Null terminated static string
   */
  inline const std::string getPath() {
    char *data_path = getDataPath();
    std::string data_path_str(data_path);
    free(data_path);
    return data_path_str;
    }

  /**
   * Stop Teonet event manager
   *
   */
  inline void stop() { ksnetEvMgrStop(ke); }

  /**
   * Cast data to teo::teoPacket
   *
   * @param data Pointer to data
   * @return Pointer to teo::teoPacket
   */
  inline teo::teoPacket* getPacket(void* data) const { return (teo::teoPacket*)data; }

  /**
   * Send counter metric
   * 
   * @param name Metric name
   * @param value Metric counter value 
   */
  inline void metricCounter(const std::string &name, int value) {
    teoMetricCounter(ke->tm, name.c_str(), value);
  }

  /**
   * Send counter metric
   * 
   * @param name Metric name
   * @param value Metric counter value 
   */
  inline void metricCounter(const std::string &name, double value) {
    teoMetricCounterf(ke->tm, name.c_str(), value);
  }

  /**
   * Send time(ms) metric
   * 
   * @param name Metric name
   * @param value Metric ms value 
   */
  inline void metricMs(const std::string &name, double value) {
    teoMetricMs(ke->tm, name.c_str(), value);
  }

  /**
   * Send gauge metric
   * 
   * @param name Metric name
   * @param value Metric gauge value 
   */
  inline void metricGauge(const std::string &name, int value) {
    teoMetricGauge(ke->tm, name.c_str(), value);
  }

  /**
   * Send gauge metric
   * 
   * @param name Metric name
   * @param value Metric gauge value 
   */
  inline void metricGauge(const std::string &name, double value) {
    teoMetricGaugef(ke->tm, name.c_str(), value);
  }

  /**
   * Virtual Teonet event callback
   *
   * @param event
   * @param data
   * @param data_len
   * @param user_data
   */
  virtual inline void eventCb(teo::teoEvents event, void* data, size_t data_len, void* user_data) {

    if(event_cb) event_cb(*this, event, data, data_len, user_data);
  };

  virtual void cqueueCb(uint32_t id, int type, void* data) {}

#ifdef DEBUG_KSNET
#define teo_printf(module, type, format, ...)                                                      \
  ksnet_printf(&((getKe())->ksn_cfg), type,                                                        \
               type != DISPLAY_M ? _ksn_printf_format_(format)                                     \
                                 : _ksn_printf_format_display_m(format),                           \
               type != DISPLAY_M ? _ksn_printf_type_(type) : "",                                   \
               type != DISPLAY_M ? (module == NULL ? (getKe())->ksn_cfg.app_name : module) : "",   \
               type != DISPLAY_M ? __func__ : "", type != DISPLAY_M ? __FILE__ : "",               \
               type != DISPLAY_M ? __LINE__ : 0, __VA_ARGS__)

#define teo_puts(module, type, format)                                                             \
  ksnet_printf(&((getKe())->ksn_cfg), type,                                                        \
               type != DISPLAY_M ? _ksn_printf_format_(format) "\n"                                \
                                 : _ksn_printf_format_display_m(format) "\n",                      \
               type != DISPLAY_M ? _ksn_printf_type_(type) : "",                                   \
               type != DISPLAY_M ? (module == NULL ? (getKe())->ksn_cfg.app_name : module) : "",   \
               type != DISPLAY_M ? __func__ : "", type != DISPLAY_M ? __FILE__ : "",               \
               type != DISPLAY_M ? __LINE__ : 0)

#else
#define teo_printf(module, type, format, ...) ;
#define teo_puts(module, type, format) ;
#endif

  template <typename... Arguments>
  inline std::string formatMessage(const char* format, const Arguments&... args) {
    const char* cstr = ksnet_formatMessage(format, args...);
    std::string str(cstr);
    free((void*)cstr);
    return str;
  }

  typedef ksnCQueData cqueData;         //! Teonet CQue data structure
  typedef ksnCQueCallback cqueCallback; //! Teonet CQue callback function

  /**
   * Teonet CQue class
   */
  class CQue {

  public:
    typedef std::unique_ptr<CQue> cquePtr;

  private:
    Teonet* teo;
    ksnCQueClass* kq;

  public:
    CQue(Teonet* t, bool send_event = false)
        : teo(t), kq(ksnCQueInit(t->getKe(), send_event)) { /*std::cout << "CQue::CQue\n";*/
    }
    CQue(const CQue& obj) : teo(obj.teo), kq(obj.kq) { /*std::cout << "CQue::CQue copy\n";*/
    }
    virtual ~CQue() { ksnCQueDestroy(kq); /*std::cout << "~CQue::CQue\n";*/ }

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

    template <typename Callback, typename T>
    cqueData* add(Callback cb, T timeout = 5.00, void* user_data = NULL) {

      struct userData {
        void* user_data;
        Callback cb;
      };
      userData* ud = new userData{user_data, cb};
      return ksnCQueAdd(kq,
                        [](uint32_t id, int type, void* data) {
                          userData* ud = (userData*)data;
                          ud->cb(id, type, ud->user_data);
                          delete(ud);
                        },
                        (double)timeout, ud);
    }

    template <typename T>
    inline cqueData* add(cqueCallback cb, T timeout = 5.00, void* user_data = NULL) {

      return ksnCQueAdd(kq, cb, (double)timeout, user_data);
    }

    template <typename T> inline cqueData* add(T timeout = 5.00, void* user_data = NULL) {
      return ksnCQueAdd(kq, NULL, (double)timeout, user_data);
    }

    inline int remove(uint32_t id) {
      return ksnCQueRemove(kq, id);
    }

    template <typename Callback>
    inline void* find(void* find, const Callback compare, size_t* key_length = NULL) {
      return ksnCQueFindData(kq, find, compare, key_length);
    }

    template <typename Callback>
    inline void* find(const std::string& find, const Callback compare, size_t* key_length = NULL) {
      return ksnCQueFindData(kq, (void*)find.c_str(), compare, key_length);
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
    inline int exec(uint32_t id) { return ksnCQueExec(kq, id); }

    /**
     * Get pointer to CQue data structure
     *
     * @param data Pointer to data at EV_K_CQUE_CALLBACK event
     *
     * @return Pointer to CQue data structure
     */
    inline cqueData* getData(void* data) const { return (cqueData*)data; }

    inline void* getCQueData(int id) const { return ksnCQueGetData(kq, id); }

    /**
     * Get CQue data id
     *
     * @param cd Pointer to CQue data structure
     * @return CQue id
     */
    inline uint32_t getId(cqueData* cd) const { return cd->id; }

    /**
     * Get pointer to teonet class object
     *
     * @return Pointer to teonet class object
     */
    inline Teonet* getTeonet() const { return teo; }

    /**
     * Set callback queue data, update data set in ksnCQueAdd
     *
     * @param id Existing callback queue ID
     * @param data Pointer to new user data
     *
     * @return return 0: if data set OK; !=0 some error occurred
     */
    int setUserData(uint32_t id, void* data) {
      int retval = -1;
      struct userData {
        void* user_data;
        void* cb;
      };
      auto ud = (userData*)ksnCQueGetData(kq, id);
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
  inline CQue::cquePtr getCQueP() { return getCQue<std::unique_ptr<teo::Teonet::CQue>>(); }

  /**
   * CQue class factory(maker)
   *
   * @return Reference to CQue object
   */
  inline CQue& getCQueR() {
    static auto cque = getCQueP();
    return *cque;
  }

  /**
   * CQue class factory(maker)
   *
   * @return T
   */
  template <typename T> T getCQue() {
    T cque(new CQue(this));
    return cque;
  }

  class TeoDB {

  public:
    typedef teo_db_data teoDbData;
    typedef teo_db_data_range teoDbDataRange;
    typedef struct teoDbCQueData {

      TeoDB* teodb;
      teoDbData** tdd;
      teoL0Client* l0client;
      uint8_t cb_type;
      void* user_data;

      teoDbCQueData(TeoDB* teoDb, teoDbData** tdd, teoL0Client* l0client = NULL,
                    uint8_t cb_type = 0)
          : teodb(teoDb), tdd(tdd), l0client(l0client), cb_type(cb_type) {}

      virtual ~teoDbCQueData() {
        if(l0client) delete(l0client);
      }

      static inline teoL0Client* setl0Cli(const char* name, uint8_t name_len, const char* addr,
                                          int port) {
        return new teoL0Client((char*)name, name_len, (char*)addr, port);
      }
      static inline teoL0Client* setl0Cli(teo::teoPacket* rd) {
        return rd ? setl0Cli(rd->from, rd->from_len, rd->addr, rd->port) : NULL;
      }

      inline auto getId() const -> decltype((*tdd)->id) { return (*tdd)->id; }

      inline auto getCbType() const -> decltype(cb_type) { return cb_type; }

      inline auto getL0Client() const -> decltype(l0client) { return l0client; }

      inline void* getKey() const { return (*tdd)->key_data; }
      inline char* getKeyStr() const { return (char*)getKey(); }

      inline void* getValue() const { return (*tdd)->key_data + (*tdd)->key_length; }
      inline char* getValueStr() const { return (char*)getValue(); }

    } teoDbCQueData;

  private:
    Teonet* teo;
    teoDbData** tdd;
    std::string peer;
    CQue cque = CQue(teo);

  public:
    TeoDB(Teonet* t, const std::string& peer, teoDbData** tdd)
        : teo(t), tdd(tdd), peer(peer) { /*std::cout << "TeoDB::TeoDB\n";*/
    }
    TeoDB(const TeoDB& obj)
        : teo(obj.teo), tdd(obj.tdd), peer(obj.peer) { /*std::cout << "TeoDB::TeoDB copy\n";*/
    }
    virtual ~TeoDB() { /*std::cout << "~TeoDB::TeoDB\n";*/
    }

    /**
     * Get pointer to teonet class object
     *
     * @return Pointer to teonet class object
     */
    inline Teonet* getTeonet() const { return teo; }

    inline ksnetEvMgrClass* getKe() const { return getTeonet()->getKe(); }

    inline CQue* getCQue() { return &cque; }

    inline const char* getPeer() const { return peer.c_str(); }

    inline bool checkPeer(const char* peer_name) { return !strcmp(peer.c_str(), peer_name); }

    inline std::unique_ptr<teoDbData> prepareRequest(const void* key, size_t key_len,
                                                     const void* data, size_t data_len, uint32_t id,
                                                     size_t* tdd_len) {

      return (std::unique_ptr<teoDbData>)prepare_request_data(key, key_len, data, data_len, id,
                                                              tdd_len);
    }

    static inline teoDbData* getData(teo::teoPacket* rd) {
      return (teoDbData*)(rd ? rd->data : NULL);
    }

    inline ksnet_arp_data* send(uint8_t cmd, const void* key, size_t key_len,
                                const void* data = NULL, size_t data_len = 0) {

      return sendTDD(cmd, key, key_len, data, data_len);
    }

    template <typename Callback>
    ksnet_arp_data* send(uint8_t cmd, const void* key, size_t key_len, Callback cb, double timeout,
                         uint8_t cb_type, teo::teoPacket* rd = NULL, teoDbData** tdd = NULL,
                         void* user_data = NULL) {

      // Create user data if it  is NULL
      if(!user_data) {
        if(!tdd) tdd = this->tdd;
        user_data = new teoDbCQueData(this, tdd, teoDbCQueData::setl0Cli(rd), cb_type);
      }

      // Add callback to queue and wait timeout after 5 sec ...
      struct userData {
        void* user_data;
        Callback cb;
      };
      auto ud = new userData{user_data, cb};
      auto cq = cque.add(
          [](uint32_t id, int type, void* data) {
            userData* ud = (userData*)data;
            ud->cb(id, type, ud->user_data);
            delete((TeoDB::teoDbCQueData*)ud->user_data);
            delete(ud);
          },
          timeout, ud);
      teo_printf(NULL, DEBUG, "Register callback id %u\n", cq->id);
      return sendTDD(cmd, key, key_len, NULL, 0, cq->id);
    }

    ksnet_arp_data* send(uint8_t cmd, const void* key, size_t key_len, cqueCallback cb,
                         double timeout, uint8_t cb_type, teo::teoPacket* rd = NULL,
                         teoDbData** tdd = NULL, void* user_data = NULL) {

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

    inline int exec(uint32_t id) { return cque.exec(id); }

    bool process(teoEvents event, void* data) {

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

      default: break;
      }

      return processed;
    }

  private:
    ksnet_arp_data* sendTDD(uint8_t cmd, const void* key, size_t key_len, const void* data = NULL,
                            size_t data_len = 0, uint32_t id = 0) {

      size_t tdd_len;
      auto tddr = prepareRequest(key, key_len, data, data_len, id, &tdd_len);
      return teo->sendTo(peer.c_str(), cmd, tddr.get(), tdd_len);
      /*auto tddr = prepare_request_data(key, key_len, data, data_len, id, &tdd_len);
      auto retval = teo->sendTo(peer.c_str(), cmd, tddr, tdd_len);
      free(tddr);
      return retval;*/
    }
  };

  class LogReader {

  public:
    using Watcher = teoLogReaderWatcher;
    using Flags = teoLogReaderFlag;

  private:
    Teonet& teo;

  public:
    LogReader(Teonet& teo) : teo(teo) {}
    LogReader(Teonet* teo) : teo(*teo) {}

    struct PUserData {
      virtual ~PUserData() { std::cout << "~PUserData" << std::endl; }
    };

    template <typename AnyCallback>
    inline Watcher* open(const char* name, const char* file_name, Flags flags = READ_FROM_BEGIN,
                         const AnyCallback& cb = nullptr) const {
      struct UserData : PUserData {
        AnyCallback cb;
        UserData(const AnyCallback& cb) : cb(cb) {}
        virtual ~UserData() { std::cout << "~UserData" << std::endl; }
      };
      auto ud = new UserData(cb);
      return teoLogReaderOpenCbPP(teo.getKe()->lr, name, file_name, flags,
                                  [](void* data, size_t data_length, Watcher* wd) {
                                    ((UserData*)wd->user_data)->cb(data, data_length, wd);
                                  },
                                  ud);
    }

    inline int close(Watcher* wd) const {
      if(wd->user_data) delete(PUserData*)wd->user_data;
      return teoLogReaderClose(wd);
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
      for(size_t i = 0; i < v.size(); i++) { type_str += v[i] + (i < v.size() - 1 ? "," : ""); }
      return type_str;
    }
  } type;
  std::string version;
  std::string teonetVersion;

  void makeTeonetVersion(uint8_t* ver, int num_dig = DIG_IN_TEO_VER) {
    teonetVersion = "";
    for(auto i = 0; i < num_dig; i++) {
      teonetVersion += std::to_string(ver[i]) + (i < num_dig - 1 ? "." : "");
    }
  }

  HostInfo(void* data) {
    auto host_info = (host_info_data*)data;
    makeTeonetVersion(host_info->ver);
    size_t ptr = 0;
    for(auto i = 0; i <= host_info->string_ar_num; i++) {
      if(!i)
        name = host_info->string_ar + ptr;
      else if(i == host_info->string_ar_num)
        version = host_info->string_ar + ptr;
      else
        type.v.push_back(host_info->string_ar + ptr);
      ptr += std::char_traits<char>::length(host_info->string_ar + ptr) + 1;
    }
  }
};

/**
 * Teonet string array class
 */
class StringArray {

private:
  ksnet_stringArr sa;
  std::string sep = ",";

  inline ksnet_stringArr create() const { return ksnet_stringArrCreate(); }
  inline ksnet_stringArr split(const char* str, const char* separators, int with_empty,
                               int max_parts) {
    sep = separators;
    return ksnet_stringArrSplit(str, separators, with_empty, max_parts);
  }
  inline ksnet_stringArr destroy(ksnet_stringArr* arr) const { return ksnet_stringArrFree(arr); }
  inline const char* _to_string(const char* separator) const {
    return ksnet_stringArrCombine(sa, separator ? separator : sep.c_str());
  }

public:
  // Default constructor
  StringArray() {
    // std::cout << "Default constructor" << std::endl;
    sa = create();
  }

  // Split from const char* constructor
  StringArray(const char* str, const char* separators = ",", bool with_empty = true,
              int max_parts = 0) {
    // std::cout << "Split from const char* constructor" << std::endl;
    sa = split(str, separators, with_empty, max_parts);
  }

  // Split from std::string constructor
  StringArray(const std::string& str, const std::string& separators = ",", bool with_empty = true,
              int max_parts = 0)
      : StringArray(str.c_str(), separators.c_str(), with_empty, max_parts) {
    // std::cout << "Split from std::string constructor" << std::endl;
  }

  // Combine from std::vector constructor
  StringArray(const std::vector<const char*>& vstr, const char* separators = ",") {
    // std::cout << "Combine from std::vector constructor" << std::endl;
    sa = create();
    sep = separators;
    for(auto& st : vstr) add(st);
  }

  // Assignment operator
  StringArray& operator=(const StringArray& ar) {
    sa = create();
    sep = ar.sep;
    for(int i = 0; i < ar.size(); i++) { add(ar[i]); }
    return *this;
  }

  virtual ~StringArray() { destroy(&sa); }

public:
  inline const char* operator[](int i) const { return sa[i]; }

  inline int size() const { return ksnet_stringArrLength(sa); }
  inline StringArray& add(const char* str) {
    ksnet_stringArrAdd(&sa, str);
    return *this;
  };
  inline StringArray& add(const std::string& str) {
    add(str.c_str());
    return *this;
  };
  inline std::string to_string(const char* separator = NULL) const {
    return std::string(unique_raw_ptr::make<const char>(_to_string(separator)).get());
  }
  inline std::string to_string(const std::string& separator) const {
    return to_string(separator.c_str());
  }
  inline bool move(unsigned int fromIdx, unsigned int toIdx) {
    return !!ksnet_stringArrMoveTo(sa, fromIdx, toIdx);
  }

  class iterator {

  private:
    using pointer = ksnet_stringArr;
    using reference = char*&;

    pointer ptr_;
    int idx_ = 0;

  public:
    iterator(pointer ptr, int idx) : ptr_(ptr), idx_(idx) {}
    const pointer operator->() { return ptr_; }
    iterator operator++() {
      idx_++;
      return *this;
    }
    iterator operator++(int junk) {
      auto rv = *this;
      idx_++;
      return rv;
    }
    bool operator==(const iterator& rhs) { return idx_ == rhs.idx_; }
    bool operator!=(const iterator& rhs) { return idx_ != rhs.idx_; }
    reference operator*() { return (ptr_)[idx_]; }
  };

  inline iterator begin() { return iterator(sa, 0); }
  inline iterator end() { return iterator(sa, size()); }
};

} // namespace teo

/*
 * This an Utils submodule.
 *
 * - The showMessage used to show teonet messages:
 *    showMessage(DEBUG, "Some debuf message\n");
 *
 * - The watch(x) is very useful debugging define to print variable name and it
 * value in next format:
 *    variable = value
 *
 */
#ifndef THIS_UTILS_H
#define THIS_UTILS_H

#define msg_to_string(msg)                                                                         \
  ({                                                                                               \
    std::ostringstream foo;                                                                        \
    foo << msg;                                                                                    \
    foo.str();                                                                                     \
  })

#define showMessage(mtype, msg) teo_printf(NULL, mtype, "%s", msg_to_string(msg).c_str())
#define showMessageLn(mtype, msg) teo_printf(NULL, mtype, "%s\n", msg_to_string(msg).c_str())

#define watch(x) std::cout << (#x) << " = " << (x) << std::endl

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* THIS_UTILS_H */

#endif /*THIS_TEONET_H */
