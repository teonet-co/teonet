/** 
 * File:   teovpn_cpp.cpp
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Teonet c++ application.
 * CMD_USER
 * Created on July 14, 2015, 9:52 AM
 */

#include "teonet"

#define TCPP_VERSION "0.0.1"

/**
 * Teonet event handler
 *
 * @param ke
 * @param event
 * @param data
 * @param data_len
 * @param user_data
 */
static void event_cb(teo::Teonet &teo, teo::teoEvents event, void *data,
              size_t data_len, void *user_data) {

    switch(event) {

        case EV_K_STARTED:
            std::cout << "Event: EV_K_STARTED\n";
            break;
            
        // Calls after event manager stopped
        case EV_K_STOPPED:
            std::cout << "Event: EV_K_STOPPED\n";
            break;
            
        default:
            break;
    }
}

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
    auto *teo = new teo::Teonet(argc, argv, event_cb);
    
    // Set application type
    teo->setAppType("teo-cpp");
    teo->setAppVersion(TCPP_VERSION);
    
    // Start teonet
    teo->run();
    
    delete(teo);
    
    return (EXIT_SUCCESS);
}
