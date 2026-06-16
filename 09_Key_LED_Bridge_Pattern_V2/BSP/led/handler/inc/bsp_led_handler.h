/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bsp_led_driver.h
 * 
 * @par dependencies 
 * - stdio.h
 * - stdint.h
 * 
 * @author Jack | R&D Dept. | EternalChip ??????????
 * 
 * @brief Provide the HAL APIs of LEDs and corresponding operations.
 * 
 * Processing flow:
 * 
 * call directly.
 * 
 * @version V1.0 2024-10-18
 *
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/
#ifndef __BSP_LED_HANDLER_H__
#define __BSP_LED_HANDLER_H__

//******************************** Includes *********************************//
#include "bsp_led_driver.h"
#include <stdio.h>
#include <stdint.h>

//******************************** Includes *********************************//


//******************************** Defines **********************************//

#define OS_SUPPORTING            /* OS_SUPPORTING depending on OS avaliable  */
#define DEBUG                    /* Enable DEBUG                             */
#define DEBUG_OUT(X)   printf(X) /* DEBUG output infoto indicate statues     */

typedef struct bsp_led_driver bsp_led_driver_t;

typedef enum
{
	HANDLER_NOT_INITED     = 1,      /* LED Run-time error without cas       */ 
    HANDLER_INITED         = 0,      /* LED Operation completed succes       */
} led_handler_init_t;

typedef enum
{
	HANDLER_OK             = 0,      /* LED Operation completed succes       */
	HANDLER_ERROR          = 1,      /* LED Run-time error without cas       */
	HANDLER_ERRORTIMEOUT   = 2,      /* LED Operation failed with time       */
	HANDLER_ERRORRESOURCE  = 3,      /* LED Resource not available.          */
	HANDLER_ERRORPARAMETER = 4,      /* LED Parameter error.                 */
	HANDLER_ERRORNOMEMORY  = 5,      /* LED Out of memory.                   */
	HANDLER_ERRORISR       = 6,      /* LED Not allowed in ISR context       */
	HANDLER_RESERVED       = 0xFF,   /* LED Reserved                         */
} led_handler_status_t;

typedef struct
{
    led_handler_status_t (*pf_get_time_ms)  ( uint32_t * const );/* LED Op   */
} handler_time_base_ms_t;

#ifdef OS_SUPPORTING
typedef struct
{
    led_handler_status_t (*pf_os_delay_ms)  ( const   uint32_t );/* LED Op   */
} handler_os_delay_t;
#endif //OS_SUPPORTING
typedef struct bsp_led_handler bsp_led_handler_t;

typedef led_handler_status_t (*pf_handler_led_control_t)(
                                         bsp_led_driver_t * const         self,
                                         uint32_t ,       //     Cycle_time[ms]
                                         uint32_t ,       // blink_times[times]
                                         proportion_t     //  proportion_on_off
                                        );

typedef led_handler_status_t (*pf_handler_led_register_t)(
                                         bsp_led_handler_t * const        self,
                                         bsp_led_driver_t  * const  led_driver
                                        );
                                               
typedef struct bsp_led_handler
{
    /**********Target of Internal Status**********/
    uint8_t                              is_inited;
    
    /**********Internal registered instances******/
    bsp_led_driver_t *      led_instance_group[10];

    /*************Target of Features**************/
    //Maybe have feature later...

    /*************Target of IOs needed************/
    /* The interface from Core layer             */
    time_base_ms_t                 *p_time_base_ms;
    /* The interface from OS layer               */
#ifdef OS_SUPPORTING
    os_delay_t                    *p_os_time_delay;
#endif //OS_SUPPORTING

    /*************Target of APIs******************/
    /* For APP                                   */
    pf_led_control_t             pf_led_countroler;
    /* For Internal driver                       */
    pf_handler_led_register_t      pf_led_register;

}bsp_led_handler_t;

//******************************** Defines **********************************//

//******************************** Declaring ********************************//
/**
 * @brief instantiate the target of bsp_led_handler_t.
 * 
 * Steps:
 *  1, Adding the Core interfaces into target of bsp_led_driver instance.
 *  2, Adding the OS interfaces into target of bsp_led_driver instance.
 *  3, Adding the timebase interfaces into target of bsp_led_driver instance.
 *  
 * 
 * @param[in] self      : Pointer to the input data.
 * @param[in] led_ops   : Length of the input data.
 * @param[in] os_delay  : Pointer to the input data.
 * @param[in] time_base : Pointer to the input data.
 * 
 * @return led_handler_status_t : The status of running
 * 
 * */
led_handler_status_t led_handler_inst (
                                      bsp_led_handler_t * const     self, 
#ifdef OS_SUPPORTING
                                      os_delay_t       * const  os_delay,
#endif //OS_SUPPORTING
                                      time_base_ms_t   * const  time_base    );

/**
 * @brief Regiter the target of led_driver_instance.
 * 
 * Steps:
 *  1, .
 * 
 * @param[in] self      : Pointer to the input data.
 * 
 * @return led_handler_status_t : The status of running
 * 
 * */
led_handler_status_t led_regiter_inst (bsp_led_driver_t * const  self);

//******************************** Declaring ********************************//

#endif // End of __BSP_LED_DRIVER_H__

