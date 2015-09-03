/**
 * \file   test_cque.c
 * \author Kirill Scherba <kirill@scherba.ru>
 * 
 * Callback QUEUE [module](@ref cque.c) tests suite
 * 
 * Test functions:
 * 
 * * Initialize/Destroy module class: test_4_1()
 * * Add callback to QUEUE: test_4_2()
 * * Execute callback to emulate callback event: test_4_3()
 * 
 * cUnit test suite code: \include test_cque.c
 *
 * Created on Aug 22, 2015, 3:35:32 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#include "ev_mgr.h"
#include "modules/cque.h"

extern CU_pSuite pSuite; // Test global variable

/**
 * Emulate ksnetEvMgrClass
 */
#define kc_emul() \
  ksnetEvMgrClass ke_obj; \
  ksnetEvMgrClass *ke = &ke_obj; \
  ke->ev_loop = ev_loop_new (0)

//! Initialize and Destroy module class
void test_4_1() {
    
    // Emulate ksnCoreClass
    kc_emul();

    // Initialize module
    ksnCQueClass *kq = ksnCQueInit(ke);
    CU_ASSERT_PTR_NOT_NULL_FATAL(kq);
    CU_ASSERT_PTR_NOT_NULL_FATAL(kq->cque_map);
    
    // Destroy module
    ksnCQueDestroy(kq);
    CU_PASS("Destroy ksnPblKfClass done");
}

/**
 * Callback, this time for a time-out
 * 
 * @param loop Event manager loop
 * @param w Watcher
 * @param revents Reserved (not used)
 */
static void timeout_cb (EV_P_ ev_timer *w, int revents) {

    //puts ("timeout");
    // this causes the innermost ev_run to stop iterating
    ev_break (EV_A_ EVBREAK_ONE);
}

/**
 * Callback Queue callback
 * 
 * @param id Calls ID
 * @param type Type: 0 - timeout callback; 1 - successful callback 
 * @param data User data
 */
void kq_cb(uint32_t id, int type, void *data) {
    
    //printf("CQue callback id: %d, type: %d \n", id, type);
}

//! Add callback to QUEUE
void test_4_2() {
    
    // Emulate ksnCoreClass
    kc_emul();

    // Initialize module
    ksnCQueClass *kq = ksnCQueInit(ke);
    CU_ASSERT_PTR_NOT_NULL_FATAL(kq);
    CU_ASSERT_PTR_NOT_NULL_FATAL(kq->cque_map);
    
    // Add callback to queue
    ksnCQueData *cq = ksnCQueAdd(kq, kq_cb, 0.050, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cq);
    CU_ASSERT(pblMapSize(kq->cque_map) == 1);
    cq = ksnCQueAdd(kq, kq_cb, 0.050, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cq);
    CU_ASSERT(pblMapSize(kq->cque_map) == 2);
    
    // Define timer to stop event loop after 1.0 sec
    ev_timer timeout_watcher;
    ev_timer_init (&timeout_watcher, timeout_cb, 0.250, 0.0);
    ev_timer_start (ke->ev_loop, &timeout_watcher);     
    // Start event loop
    ev_run (ke->ev_loop, 0);
    CU_ASSERT(pblMapSize(kq->cque_map) == 0);
    
    // Destroy module
    ksnCQueDestroy(kq);
    CU_PASS("Destroy ksnPblKfClass done");
}

//! Execute callback to emulate callback event
void test_4_3() {
    
    // Emulate ksnCoreClass
    kc_emul();

    // Initialize module
    ksnCQueClass *kq = ksnCQueInit(ke);
    CU_ASSERT_PTR_NOT_NULL_FATAL(kq);
    CU_ASSERT_PTR_NOT_NULL_FATAL(kq->cque_map);
    
    // Add callback to queue
    ksnCQueData *cq = ksnCQueAdd(kq, kq_cb, 0.050, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cq);
    CU_ASSERT(pblMapSize(kq->cque_map) == 1);
    cq = ksnCQueAdd(kq, kq_cb, 0.050, NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(cq);
    CU_ASSERT(pblMapSize(kq->cque_map) == 2);
    
    // Execute callback queue record 
    int rv = ksnCQueExec(kq, 1);
    CU_ASSERT(rv == 0)
    CU_ASSERT(pblMapSize(kq->cque_map) == 1);
    rv = ksnCQueExec(kq, 0);
    CU_ASSERT(rv == 0)
    CU_ASSERT(pblMapSize(kq->cque_map) == 0);
    
    // Destroy module
    ksnCQueDestroy(kq);
    CU_PASS("Destroy ksnPblKfClass done");
}

/**
 * Add Callback QUEUE module tests
 * 
 * @return 
 */
int add_suite_4_tests(void) {
    
    // Add the tests to the suite 
    if ((NULL == CU_add_test(pSuite, "Initialize/Destroy module class", test_4_1)) ||
        (NULL == CU_add_test(pSuite, "Add callback to QUEUE", test_4_2)) ||
        (NULL == CU_add_test(pSuite, "Execute callback to emulate callback event", test_4_3))) {
        
        CU_cleanup_registry();
        return CU_get_error();
    }
    
    return 0;
}
