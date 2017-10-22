/**
 * \file   teocquecpp.cpp
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * \example teocquecpp.cpp
 *
 * ## Using the Teonet QUEUE callback
 * Module: cque.c
 *
 * This is example application to register callback and receive timeout or
 * success call of callback function or(and) in teonet event callback when event
 * type equal to EV_K_CQUE_CALLBACK.
 *
 * In this example we:
 *
 * * Initialize teonet event manager and Read configuration
 * * Set custom timer with 2 second interval. To generate continously timer
 *   events
 * * Start teonet
 * * Check timer event in teonet event callback
 *   * At first timer event we: Add callback to queue and start waiting timeout
 *     call after 5 sec ...
 *   * At fifth timer event we: Add next callback to queue and start waiting
 *     for success result ...
 *   * At seventh timer event we: Execute callback queue function to make
 *     success result and got it at the same time.
 *   * At tenth timer event we: Stop the teonet to finish this Example
 *
 *
 * Created on October 19, 2017, 6:07 PM
 */

#include "teonet"

#define TCQUE_VERSION "0.0.2"

class MyTeonet : public teo::Teonet {

// Constructor & destructor
public:

    CQue cque = CQue(this);

    MyTeonet(int argc, char** argv) : teo::Teonet(argc, argv) { /*std::cout << "MyTeonet::MyTeonet\n";*/ }
    virtual ~MyTeonet() { /*std::cout << "~MyTeonet::MyTeonet\n";*/  }

// Own class methods and data
public:
    
    /**
     * Callback Queue callback (the same as callback queue event).
     *
     * This function calls at timeout or after ksnCQueExec calls
     *
     * @param id Calls ID
     * @param type Type: 0 - timeout callback; 1 - successful callback
     * @param data User data inserted in teo::CQue::add
     */
    static void kq_cb(uint32_t id, int type, void *data) {

        std::cout << "Got Callback Queue callback with id: " << id << ", type: "
                  << type << " => " << (type ? "success" : "timeout")
                  << ", data: " << (const char*)data << "\n";
    }

    /**
     * Teonet Events callback
     *
     * @param ke Pointer to ksnetEvMgrClass
     * @param event Teonet Event (ksnetEvMgrEvents)
     * @param data Events data
     * @param data_len Data length
     * @param user_data Some user data (may be set in ksnetEvMgrInitPort())
     */
    void eventCb(teo::teoEvents event, void *data, size_t data_len, 
            void *user_data) override {

        // Check teonet event
        switch(event) {

            // Teonet started
            case EV_K_STARTED: {

            } break;

            // Teonet started
            case EV_K_STOPPED: {

            } break;

            // Send by timer
            case EV_K_TIMER: {

                static int num = 0,     // Timer event counter
                       success_id = -1; // Success ID number

                std::cout << "\nTimer event " << ++num << "...\n";

                switch(num) {

                    // After 5 sec
                    case 1: {

                        // Add lambda callback to queue and wait timeout after 5 sec ...
                        auto cq = cque.add(
                            [](teo::Teonet::CQue &cque, uint32_t id, int type, void *data) {

                                std::cout
                                    << "Got lambda Callback Queue callback with"
                                    << " id: " << id
                                    << ", type: " << type
                                    << " => " << (type ? "success" : "timeout")
                                    << ", data: " << (const char*)data
                                    << ", teonet class version: " << cque.getTeonet()->getClassVersion()
                                    << "\n";

                            }, 5.000, (void*)(const char*)"Hello");

                        std::cout << "Register callback id " << cque.getId(cq) << "\n";
                    } break;

                    // After 25 sec
                    case 5: {

                        // Add callback to queue and wait success result ...
                        auto cq = cque.add(kq_cb, 5.000, (void*)(const char*)"Hello2");
                        std::cout << "Register callback id " << cque.getId(cq) << " \n";
                        success_id = cque.getId(cq);
                    } break;

                    // After 35 sec
                    case 7: {

                        // Execute callback queue to make success result
                        cque.exec(success_id);
                    } break;

                    // After 50 sec
                    case 10: {

                        // Stop teonet to finish this Example
                        std::cout << "\nExample stopped...\n\n";
                        stop();
                    } break;

                    default: break;
                }

            } break;

            // Callback QUEUE event (the same as callback queue callback). This
            // event send at timeout or after ksnCQueExec calls
            case EV_K_CQUE_CALLBACK: {

                int type = *(int*) user_data; // Callback type: 1 - success; 0 - timeout
                auto *cq = cque.getData(data); // Pointer to Callback QUEUE data

                std::cout << "Got Callback Queue event with id: " << cque.getId(cq)
                          << ", type: " << type
                          << " => " << (type ? "success" : "timeout")
                          //<< ", data: " << (const char*)cq->data
                          << "\n";
            } break;

            // Other events
            default: break;
        }
    }
};

/**
 * Main QUEUE callback example function
 *
 * @param argc Number of arguments
 * @param argv Arguments array
 *
 * @return
 */
int main(int argc, char** argv) {

    std::cout << "Teocque example ver " TCQUE_VERSION ", "
                 "based on teonet ver. " VERSION "\n";


    // Initialize teonet event manager and Read configuration
    auto *teo = new MyTeonet(argc, argv);

    // Set custom timer interval. See "case EV_K_TIMER" to continue this example
    teo->setCustomTimer(2.00);

    // Show Hello message
    std::cout << "Example started...\n\n";

    // Start teonet
    teo->run();

    delete(teo);

    return (EXIT_SUCCESS);
}
