/** 
 * \file   teocque.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * \example teocque.c
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
 * Created on September 2, 2015, 6:04 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ev_mgr.h"

#define TCQUE_VERSION "0.0.1"    

/**
 * Callback Queue callback (the same as callback queue event). 
 * 
 * This function calls at timeout or after ksnCQueExec calls
 * 
 * @param id Calls ID
 * @param type Type: 0 - timeout callback; 1 - successful callback 
 * @param data User data
 */
void kq_cb(uint32_t id, int type, void *data) {
    
    printf("Got Callback Queue callback with id: %d, type: %d => %s\n", 
            id, type, type ? "success" : "timeout");
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
void event_cb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data,
              size_t data_len, void *user_data) {

    // Check teonet event
    switch(event) {
        
        // Send by timer
        case EV_K_TIMER:
        {
            static int num = 0, // Timer event counter
                   success_id = -1; // Success ID number
            
            printf("\nTimer event %d...\n", ++num);
            
            switch(num) {
                
                // After 5 sec
                case 1: {
                    
                    // Add callback to queue and wait timeout after 5 sec ...
                    ksnCQueData *cq = ksnCQueAdd(ke->kq, kq_cb, 5.000, NULL);
                    printf("Register callback id %d\n", cq->id);
                } 
                break;
                    
                // After 25 sec
                case 5: {
                    
                    // Add callback to queue and wait success result ...
                    ksnCQueData *cq = ksnCQueAdd(ke->kq, kq_cb, 5.000, NULL);
                    printf("Register callback id %d\n", cq->id);
                    success_id = cq->id;
                }
                break;
                    
                // After 35 sec
                case 7: {
                    
                    // Execute callback queue to make success result
                    ksnCQueExec(ke->kq, success_id);
                }
                break;
                
                // After 50 sec
                case 10: 
                    
                    // Stop teonet to finish this Example
                    printf("\nExample stopped...\n\n");
                    ksnetEvMgrStop(ke);
                    break;
                    
                default: break;
            }
        }
        break;      
        
        // Callback QUEUE event (the same as callback queue callback). This 
        // event send at timeout or after ksnCQueExec calls
        case EV_K_CQUE_CALLBACK: 
        {
            int type = *(int*)user_data; // Callback type: 1- success; 0 - timeout
            ksnCQueData *cq = data; // Pointer to Callback QUEUE data

            printf("Got Callback Queue event with id: %d, type: %d => %s\n", 
            cq->id, type, type ? "success" : "timeout");
        }    
        break;
        
        // Other events
        default: break;
    }
}

/**
 * Main QUEUE callback example function
 *
 * @param argc Number of arguments
 * @param argv Arguments array
 * 
 * @return
 */
int main(int argc, char** argv) {
    
    printf("Teocque example ver " TCQUE_VERSION ", "
           "based on teonet ver. " VERSION "\n");
    
   
    // Initialize teonet event manager and Read configuration
    ksnetEvMgrClass *ke = ksnetEvMgrInit(argc, argv, event_cb,
            READ_OPTIONS|READ_CONFIGURATION);
    
    // Set custom timer interval. See "case EV_K_TIMER" to continue this example
    ksnetEvMgrSetCustomTimer(ke, 2.00);
    
    // Show Hello message
    ksnet_printf(&ke->ksn_cfg, MESSAGE, "Example started...\n\n");

    // Start teonet
    ksnetEvMgrRun(ke);    
    
    return (EXIT_SUCCESS);
}
