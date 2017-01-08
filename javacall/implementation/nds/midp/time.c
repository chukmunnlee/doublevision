/*
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <nds.h>
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>

#include "javacall_time.h"

static javacall_bool system_timer_initialized=JAVACALL_FALSE;

//Call out table 
struct timer_info {
    struct timer_info *prev;
    struct timer_info *next;
    javacall_bool cyclic;
    int wakeupInMillis; /* actual */
    int delta; /* adjusted to callout table */
    javacall_callback_func func;
};

int count_nodes(struct timer_info *node) {
    int c = 0;
    struct timer_info *curr = node;
    while (curr != NULL) {
        c++;
        curr = curr->next;
    }
    return (c);
}

//Root node
static struct timer_info *callout = NULL;
static volatile long long msec_base = 0;
static volatile long long msec_tick = 0;

void add_timer(struct timer_info *node) {

    struct timer_info *curr;

    //Initialize it first
    node->delta = node->wakeupInMillis;
    node->prev = NULL;
    node->next = NULL;

    //stopTimer();

    if (callout == NULL) {
        callout = node;
        //startTimer();
        return;
    }

    curr = callout;
    //Add to the root of the list
    if (node->delta < curr->delta) {
        curr->delta -= node->delta;
        callout = node;
        node->next = curr;
        curr->prev = node;
        //startTimer();
        return;
    }

    while (curr != NULL) {
        /* we have a potential node that is in the same delta slot
            as the prev node */
        if (node->delta <= 0) {
            node->delta = 0;
            node->next = curr->next;
            if (curr->next != NULL)
                curr->next->prev = node;
            curr->next = node;
            node->prev = curr;
            break;

        } else if (node->delta == curr->delta) {
            node->delta = 0;
            node->next = curr->next;
            if (curr->next != NULL)
                curr->next->prev = node;
            curr->next = node;
            node->prev = curr;
            break;

        } else if (node->delta < curr->delta) { 
            curr->delta -= node->delta;
            if (curr->prev != NULL) {
                curr->prev->next = node;
                node->prev = curr->prev;
                node->next = curr;
                curr->prev = node;
            } else { //We are at the root when prev == NULL
                node->next = curr;
                curr->prev = node;
                callout = node;
            }
            break;

        } else { /* node->delta > curr->delta */
            node->delta -= curr->delta;
            if (curr->next != NULL)
                curr = curr->next;
            else {
                //We have reached to the end of the chain
                curr->next = node;
                node->prev = curr;
                break;
            }
            continue;
        }
    }

    //startTimer();
    return;
}

/* Important - caller is responsible for freeing node */
void remove_timer(struct timer_info *node) {

    struct timer_info *curr;

    //stopTimer();

    if (callout == NULL) {
        //startTimer();
        return;
    }

    //if the node we are looking for is at the root
    if (callout == node) {
        callout = callout->next;
/*
        if (callout != NULL && callout->delta <= 0)
            callout->delta = node->delta;
*/
        if (callout != NULL)
            callout->delta += node->delta;
        //startTimer();
        return;
    }

    curr = callout;
    while (curr != NULL) {
        if (curr == node) {
            /* if the curr delta > 0, then need to add delta to next node
               if next node is == 0 */
/*
            if (curr->delta > 0) {
                if ((curr->next != NULL) && (curr->next->delta <= 0))
                    curr->next->delta = curr->delta;
            }
*/
            /* adjust the prev, next to bypass curr node */
            if (curr->prev != NULL)
                curr->prev->next = curr->next;
            if (curr->next != NULL) {
                curr->next->delta += curr->delta;
                curr->next->prev = curr->prev;
			}
            //Do not free, might be a cyclic, we leave this responsibility
            //to the caller
            //free(node);
            break;
        } else if (curr->next != NULL)
            curr = curr->next;
        else 
            break;
    }

    //startTimer();
    return;
}

void tick_tock(void) {

    struct timer_info *curr, *tmp;

	//Increment timer since time starts
    ++msec_tick;

    if (callout == NULL)
        return;

    /* timer resolution is 1ms */
    callout->delta -= 1;
    if (callout->delta > 0)
        return;

    /* Fire the events */
    curr = callout;
    while (curr != NULL) {

        //As per advise from Liang
        curr->func(NULL);

        //If next is not NULL and in the same timer bucket,
        //fire it.
        if ((curr->next != NULL) && (curr->next->delta <= 0))
            tmp = curr->next;
        else
            tmp = NULL;

        /* if it is cyclic, then add it again */
        remove_timer(curr);
        if (curr->cyclic)
            add_timer(curr);
        else
            free(curr);

        curr = tmp;
    }
}

/* IMPORTANT: Must call to install timer interupt */
void javacall_initialize_system_timer(void) {

    if (system_timer_initialized) {
        return;
    }
    //Set tick_tock to 1ms resolution as required my VM
    TIMER0_CR = 0;

    TIMER0_DATA = (u16)TIMER_FREQ(1000);
    TIMER0_CR = TIMER_ENABLE | TIMER_DIV_1 | TIMER_IRQ_REQ;
    irqSet(IRQ_TIMER0, tick_tock);

    //Advise from Liang
    time_t seconds = 0;
    while (!seconds)
        seconds = time(NULL);
    msec_base = seconds * 1000ll;

    system_timer_initialized = JAVACALL_TRUE;

	startTimer();
}

void startTimer(void) {
    irqEnable(IRQ_TIMER0);
}

void stopTimer(void) {
    irqDisable(IRQ_TIMER0);
}

/**
 *
 * Create a native timer to expire in wakeupInSeconds or less seconds.
 *
 * @param wakeupInMilliSecondsFromNow time to wakeup in milli-seconds
 *                              relative to current time
 *                              if -1, then ignore the call
 * @param cyclic <tt>JAVACALL_TRUE</tt>  indicates that the timer should be
 *               repeated cuclically,
 *               <tt>JAVACALL_FALSE</tt> indicates that this is a one-shot
 *               timer that should call the callback function once
 * @param func callback function should be called in platform's context once the timer
 *             expires
 * @param handle A pointer to the returned handle that on success will be
 *               associated with this timer.
 *
 * @return on success returns <tt>JAVACALL_OK</tt>,
 *         or <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
javacall_result javacall_time_initialize_timer(
                    int                      wakeupInMilliSecondsFromNow,
                    javacall_bool            cyclic,
                    javacall_callback_func   func,
                    /*OUT*/ javacall_handle *handle){

    struct timer_info *timer;

    if (wakeupInMilliSecondsFromNow <= 0) {
        javacall_print("javacall_time_initialize_timer invalid param: %d\n"
                , wakeupInMilliSecondsFromNow);
        return (JAVACALL_FAIL);
    }

    timer = (struct timer_info *)malloc(sizeof(struct timer_info));
    timer->cyclic = cyclic;
    timer->wakeupInMillis = wakeupInMilliSecondsFromNow;
    timer->func = func;
    add_timer(timer);

    *handle = timer;

    return JAVACALL_OK;
}

/**
 *
 * Disable a set native timer
 * @param handle The handle of the timer to be finalized
 *
 * @return on success returns <tt>JAVACALL_OK</tt>,
 *         <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
javacall_result javacall_time_finalize_timer(javacall_handle handle){
    struct timer_info *timer = (struct timer_info*) handle;

    if ((timer == NULL) || 
        (timer->delta <= 0 && !(timer->cyclic))) { // expired noncyclic timer has been finalized
        return JAVACALL_FAIL;
    }

    remove_timer(timer);
    if (timer->cyclic || timer->delta > 0) {
        free(timer);
    }

    return JAVACALL_OK;
}

/*
 *
 * Temporarily disable timer interrupts. This is called when
 * the VM is about to sleep (when there's no Java thread to execute)
 *
 * @param handle timer handle to suspend
 *
 */
void javacall_time_suspend_ticks(javacall_handle handle){
    //TODO implement it later
    //stopTimer();
    return;
}

/*
 *
 * Enable  timer interrupts. This is called when the VM
 * wakes up and continues executing Java threads.
 *
 * @param handle timer handle to resume
 *
 */
void javacall_time_resume_ticks(javacall_handle handle){
    //TODO implement it later
    //startTimer();
    return;
}

/*
 * Suspend the current process sleep for ms milliseconds
 */
void javacall_time_sleep(javacall_uint64 ms){
    int i = 0;
    if (!system_timer_initialized) {
        javacall_initialize_system_timer();
    }
    for (i = 0; i < (int)ms; i++) {
        swiIntrWait(1, IRQ_TIMER0);
    }

    return;
}

/**
 *
 * Create a native timer to expire in wakeupInSeconds or less seconds.
 * At least one native timer can be used concurrently.
 * If a later timer exists, cancel it and create a new timer
 *
 * @param type type of alarm to set, either JAVACALL_TIMER_PUSH, JAVACALL_TIMER_EVENT
 *                              or JAVACALL_TIMER_WATCHDOG
 * @param wakeupInMilliSecondsFromNow time to wakeup in milli-seconds
 *                              relative to current time
 *                              if -1, then ignore the call
 *
 * @return <tt>JAVACALL_OK</tt> on success, <tt>JAVACALL_FAIL</tt> on failure
 */
////javacall_result javacall_time_create_timer(javacall_timer_type type, int wakeupInMilliSecondsFromNow){
//    return JAVACALL_FAIL;
//}

/**
 * Return local timezone ID string. This string is maintained by this
 * function internally. Caller must NOT try to free it.
 *
 * This function should handle daylight saving time properly. For example,
 * for time zone America/Los_Angeles, during summer time, this function
 * should return GMT-07:00 and GMT-08:00 during winter time.
 *
 * @return Local timezone ID string pointer. The ID string should be in the
 *         format of GMT+/-??:??. For example, GMT-08:00 for PST.
 */
char* javacall_time_get_local_timezone(void){
    //NDS has no TZ info. May have to read the TZ in from
    //a config file. Hardcoded for the time being
    return (char*) "GMT+08:00";
}

/**
 * returns number of milliseconds elapsed since midnight(00:00:00), January 1, 1970,
 *
 * @return milliseconds elapsed since midnight (00:00:00), January 1, 1970
 */
javacall_int64 /*OPTIONAL*/ javacall_time_get_milliseconds_since_1970(void){
    return ((javacall_int64)(msec_base + msec_tick));
}

/**
 * returns the number of seconds elapsed since midnight (00:00:00), January 1, 1970,
 *
 * @return seconds elapsed since midnight (00:00:00), January 1, 1970
 */
javacall_time_seconds /*OPTIONAL*/ javacall_time_get_seconds_since_1970(void){
    time_t unix_time = time(NULL);
    if (unix_time < 0)
        return ((javacall_time_seconds)0);
    return ((javacall_time_seconds)unix_time);
}

/**
 * returns the milliseconds elapsed time counter
 *
 * @return elapsed time in milliseconds
 */
javacall_time_milliseconds /*OPTIONAL*/ javacall_time_get_clock_milliseconds(void){
    return ((javacall_time_milliseconds)msec_tick);
}

#ifdef __cplusplus
}
#endif

