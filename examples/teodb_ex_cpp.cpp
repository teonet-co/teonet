/**
 * \file   teodb_ex.c
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * \example teodb_ex.c
 *
 * ## Using the Teonet DB function
 * Module: teodb.c and teodb_com.c
 *
 * This is example application to test connection to teo-db application set and
 * get DB data.

 * Created on February 15, 2016, 2:11 AM
 */

#include <cstring>
#include <condition_variable>

#include "teonet"

#define TDB_VERSION "0.0.2"
#define APPNAME _ANSI_MAGENTA "teodb_ex" _ANSI_NONE

#define TEODB_PEER teo.getKe()->ksn_cfg.app_argv[1]
#define TEODB_EX_KEY "teo_db_ex"
#define TEST_KEY "test"
#define TEST_VALUE "{ \"name\": \"1\" }"

/**
 * Callback Queue callback (the same as callback queue event).
 *
 * This function calls at timeout or after ksnCQueExec calls
 *
 * @param id Calls ID
 * @param type Type: 0 - timeout callback; 1 - successful callback
 * @param data User data
 */

/**
 * Teonet Events callback
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param event Teonet Event (ksnetEvMgrEvents)
 * @param data Events data
 * @param data_len Data length
 * @param user_data Some user data (may be set in ksnetEvMgrInitPort())
 */
void event_cb(teo::Teonet &teo, teo::teoEvents event, void *data,
              size_t data_len, void *user_data) {

    static teo::Teonet::TeoDB::teoDbData *tdd;
    static auto teoDb = teo::Teonet::TeoDB(&teo, TEODB_PEER, &tdd);
    auto rd = teo.getPacket(data);
    //tdd = teo::Teonet::TeoDB::getData(rd);   // !!! ttd should be defined before teoDb.process call

    if(teoDb.process(event, data)) return;

    // Send commands when teo-db server connected
    if(event == EV_K_CONNECTED && teoDb.checkPeer(rd->from)) {

        std::cout << "The TeoDB peer: \"" << teoDb.getPeer() 
                  << "\" was connected\n\n";

        const char *key = TEODB_EX_KEY "." TEST_KEY;
        const size_t key_len = strlen(key) + 1;
        const char *value = TEST_VALUE;

        // 1) Add test key to the TeoDB
        std::cout << "1) Add test key to the TeoDB"
                  << ", key: \"" << key << "\""
                  << ", value: \"" << value << "\"\n";

        // Send SET command to DB peer
        teoDb.send(CMD_D_SET, key, key_len, value, strlen(value) + 1);

        // 2) \todo Send GET request to the TeoDB using cQueue
        std::cout << "2) Send GET request to the TeoDB using cQueue, "
               "key: \"" << key << "\"\n";

        // Send GET command to DB peer and wait answer in lambda function
        teoDb.send(CMD_D_GET, key, key_len, 
            [](uint32_t id, int type, void *data) {

                auto cqd = (teo::Teonet::TeoDB::teoDbCQueData*)data;

                std::cout 
                    << "3) Got CQueue lambda callback with"
                    << " id: " << id 
                    << ", type: " << type << " => " 
                    << (type ? "success" : "timeout") << "\n";

                if(type) {

                    std::cout 
                        << "Key: \"" << cqd->getKeyStr() << "\""
                        << ", Value: \"" << cqd->getValueStr() << "\""
                        << "\n";

                    // 4) Send LIST request to the TeoDB using cQueue
                    std::cout << "4) Send LIST request to the TeoDB using cQueue\n";

                    // Send CMD_D_LIST command to DB peer and wait answer in lambda function
                    cqd->teodb->send(CMD_D_LIST, TEODB_EX_KEY ".", sizeof(TEODB_EX_KEY) + 1, 
                        [](uint32_t id, int type, void *data) {

                            auto cqd = (teo::Teonet::TeoDB::teoDbCQueData*) data;
                            const size_t NUM_RECORDS_TO_SHOW = 25;

                            std::cout << "5) Got CQueue callback with id: " << id 
                                  << ", type: " << type << " => " << (type ? "success" : "timeout") 
                                  << "\n";

                            if(type) {

                                void *ar_data = cqd->getValue(); 
                                uint32_t ar_data_num = *(uint32_t*)ar_data;
                                size_t ptr = sizeof(ar_data_num);

                                std::cout << "Key: \"" << cqd->getKeyStr() << "\"\n";

                                std::cout << "Number of records in list: " << ar_data_num << "\n";
                                for(uint32_t i = 0; i < ar_data_num; i++) {
                                    size_t len = strlen((char*)ar_data + ptr) + 1;
                                    std::cout << i + 1 << " " << (char*)ar_data + ptr << "\n";
                                    ptr += len;
                                    if(i >= NUM_RECORDS_TO_SHOW - 1) {
                                        std::cout << "(the first " << NUM_RECORDS_TO_SHOW << " entries shows)\n";
                                        break;
                                    }
                                }
                                
                                const char *key = TEODB_EX_KEY "." TEST_KEY;
                                const size_t key_len = strlen(key) + 1;
                                std::cout << "6) Remove test key: " << key
                                          << "\n";
                                cqd->teodb->send(CMD_D_SET, key, key_len);
                            }
                            std::cout << "\nTest finished ...\n";
                            delete(cqd);
                        }
                    , 5.000, 0);                                         
                }
                delete(cqd);
            }
        , 5.000, 0);
    }            
}

/**
 * Main Teonet Database example function
 *
 * @param argc Number of arguments
 * @param argv Arguments array
 *
 * @return
 */
int main(int argc, char** argv) {

    std::cout << "Teodb example ver " TDB_VERSION ", "
                 "based on teonet ver. " VERSION "\n";

    // Application parameters
    const char *app_argv[] = { "", "teodb_peer"};
    teo::teoAppParam app_param = { 2, app_argv, NULL };

    // Initialize teonet event manager and Read configuration
    auto *teo = new teo::Teonet(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION|APP_PARAM, 0, &app_param);

    // Set application type
    teo->setAppType("teo-db-ex");
    teo->setAppVersion(TDB_VERSION);

    // Start teonet
    teo->run();

    return (EXIT_SUCCESS);
}

