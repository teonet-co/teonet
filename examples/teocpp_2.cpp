/**
 * File:   teovpn_cpp.cpp
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Teonet c++ application.
 * CMD_USER
 * Created on July 14, 2015, 9:52 AM
 */

#include "teonet.hpp"

#define TCPP_VERSION "0.0.2"

class MyTeonet : public teo::Teonet {

// Constructor & destructor
public:

    MyTeonet(int argc, char** argv) : teo::Teonet(argc, argv) { }
    virtual ~MyTeonet() { /*std::cout << "Destructor myTeonet\n";*/  }

// Own class methods and data
public:

//    void showMessage(const std::string msg) {
//        std::cout << msg;
//    }
    
    /**
     * Teonet event handler
     *
     * @param event Teonet event
     * @param data Pointer to data
     * @param data_len Data length
     * @param user_data Pointer to user data
     */
    void eventCb(teo::teoEvents event, void *data, size_t data_len, 
        void *user_data) override {
        
        switch(event) {

            case EV_K_STARTED: {

                showMessage(DEBUG, "Event: EV_K_STARTED, Teonet class version: " 
                        << (std::string)getClassVersion() + "\n");

            } break;
            
            case EV_K_CONNECTED: {

                // Teonet packet
                auto rd = getPacket(data);
                showMessage(DEBUG, "Event: EV_K_CONNECTED, from: " 
                        << (const std::string)rd->from << "\n");

                // Request host info
                sendTo(rd->from, CMD_HOST_INFO, NULL, 0); //(void*)"JSON", 5);

            } break;
            
            case EV_K_RECEIVED: {

                // Teonet packet
                auto rd = getPacket(data);

                switch(rd->cmd) {

                    case CMD_ECHO_ANSWER:
                        break;

                    case CMD_HOST_INFO_ANSWER: {

                        auto host_info = teo::HostInfo(rd->data);
                        showMessage(DEBUG, "Event: EV_K_RECEIVED, "
                            "Cmd: CMD_HOST_INFO_ANSWER, from: " + (std::string)rd->from + ", "
                                "name: " + host_info.name + ", " +
                                "version: " + host_info.version + ", " +
                                "teonet version: " + host_info.teonetVersion + ", " +
                                "types: " + host_info.type.to_string() +
                                "\n");

                    } break;

                    default: {
                        showMessage(DEBUG, "Event: EV_K_RECEIVED, "
                                "cmd: " + std::to_string(rd->cmd) + ", "
                                "from: " + (const char*)rd->from + ",\t"
                                "data: " + (const char*)rd->data + "\n");
                    } break;
                }

            } break;
            
          case EV_K_TIMER: {
            
            showMessage(DEBUG, "Event: EV_K_TIMER\n");
            
//            char *arp_txt = ksnetArpShowStr(eventManager()->kc->ka);
//            std::cout << "Peers:\n" << arp_txt << "\n";
//            free(arp_txt);
            
            // Get peers data
            ksnet_arp_data_ar *peers_data = ksnetArpShowData(eventManager()->kc->ka);
            //size_t peers_data_length = ksnetArpShowDataLength(peers_data);
            for(int i=0; i < (int)peers_data->length; i++) {
              std::cout << peers_data->arp_data[i].name << "\n";
              sendEchoTo(peers_data->arp_data[i].name);
            }
            
          } break;

            // Calls after event manager stopped
            case EV_K_STOPPED:
                showMessage(DEBUG, "Event: EV_K_STOPPED\n");
                break;

            default:
                break;
            
        }
    }

};


/**
 * Main Teocpp application function
 *
 * @param argc Number of parameters
 * @param argv Parameters array
 *
 * @return EXIT_SUCCESS
 */
int main(int argc, char** argv) {

    std::cout << "Teocpp ver " TCPP_VERSION ", based on teonet ver "
              << teoGetLibteonetVersion() << "\n";

    // Initialize teonet event manager and Read configuration
    auto teo = new MyTeonet(argc, argv);

    // Set application type
    teo->setAppType("teo-cpp");
    teo->setAppVersion(TCPP_VERSION);
    teo->setCustomTimer(1.00);

    // Start teonet
    teo->run();

    delete(teo);

    return (EXIT_SUCCESS);
}
