/** 
 * File:   cque.c
 * Author: Kirill Scherba <kirill@scherba.ru>
 * 
 * Module to manage callbacks QUEUE: ksnCQue
 * 
 * Main module functions:
 * 
 *  * Initialize / Destroy module (ksnCQueClass)
 *  * Add callback to QUEUE, get peers TR-UDP (or ARP) TRIP_TIME and start timeout timer
 *  * Execute callback when callback event or timeout event occurred and Remove record from queue
 *
 * Created on August 21, 2015, 12:10 PM
 */

#include "cque.h"

