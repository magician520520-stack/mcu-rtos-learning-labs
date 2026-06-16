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
 * @author Jack | R&D Dept. | EternalChip ¡¢–æ«∂»Î Ω
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
#ifndef __BSP_LED_DRIVER_H__
#define __BSP_LED_DRIVER_H__

//******************************** Includes *********************************//
#include <stdio.h>
#include <stdint.h>

//******************************** Includes *********************************//


//******************************** Defines **********************************//
#define INITED           1       /* LED is inited                            */
#define NOT_INITED       0       /* LED is not inited                        */

#define OS_SUPPORTING            /* OS_SUPPORTING depending on OS avaliable  */
#define DEBUG                    /* Enable DEBUG                             */
#define DEBUG_OUT(X)   printf(X) /* DEBUG output infoto indicate statues     */

typedef struct bsp_led_driver bsp_led_driver_t;

typedef enum
{
	LED_OK             = 0,      /* LED Operation completed successfully.    */
	LED_ERROR          = 1,      /* LED Run-time error without case matched  */
	LED_ERRORTIMEOUT   = 2,      /* LED Operation failed with timeout        */
	LED_ERRORRESOURCE  = 3,      /* LED Resource not available.              */
	LED_ERRORPARAMETER = 4,      /* LED Parameter error.                     */
	LED_ERRORNOMEMORY  = 5,      /* LED Out of memory.                       */
	LED_ERRORISR       = 6,      /* LED Not allowed in ISR context           */
	LED_RESERVED       = 0xFF,   /* LED Reserved                             */
} led_status_t;

typedef enum
{
    PROPORTIONN_1_3    = 0,      /* LED Operation completed successfully.    */
    PROPORTIONN_1_2    = 1,      /* LED Operation completed successfully.    */
    PROPORTIONN_1_1    = 2,      /* LED Operation completed successfully.    */
    PROPORTIONN_x_x    = 0xFF,   /* LED Operation completed successfully.    */
} proportion_t;

typedef struct
{
    led_status_t (*pf_led_on)  (void);/* LED Operation completed successfu   */
    led_status_t (*pf_led_off) (void);/* LED Operation completed successfu   */
} led_operations_t;

typedef struct
{
    led_status_t (*pf_get_time_ms)  ( uint32_t * const );/* LED Op           */
} time_base_ms_t;

#ifdef OS_SUPPORTING
typedef struct
{
    led_status_t (*pf_os_delay_ms)  ( const   uint32_t );/* LED Op           */
} os_delay_t;
#endif //OS_SUPPORTING

typedef led_status_t (*pf_led_control_t)(
                                     bsp_led_driver_t * const       self,
                                     uint32_t ,     //     Cycle_time[ms]
                                     uint32_t ,     // blink_times[times]
                                     proportion_t   //  proportion_on_off
                                        );
                                               
typedef struct bsp_led_driver
{
    /**********Target of Internal Status**********/
    uint8_t                              is_inited;

    /*************Target of Features**************/
    /* The whole time of                   blink */
    uint32_t                         cycle_time_ms;
    /* The times of blink                        */
    uint32_t                           blink_times;
    /* The relationship of light on and off time */
    proportion_t                 proportion_on_off;

    /*************Target of IOs needed************/
    /* The interface from Core layer             */
    led_operations_t              *p_led_opes_inst;
    time_base_ms_t                 *p_time_base_ms;
    /* The interface from OS layer               */
#ifdef OS_SUPPORTING
    os_delay_t                    *p_os_time_delay;
#endif //OS_SUPPORTING
    /*************Target of APIs******************/
    pf_led_control_t             pf_led_countroler;

}bsp_led_driver_t;


//******************************** Defines **********************************//

//******************************** Declaring ********************************//
/**
 * @brief instantiate the target of bsp_led_driver_t.
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
 * @return led_status_t : The status of running
 * 
 * */
led_status_t led_driver_inst (
                                      bsp_led_driver_t * const      self, 
                                      led_operations_t * const   led_ops,
#ifdef OS_SUPPORTING
                                      os_delay_t       * const  os_delay,
#endif //OS_SUPPORTING
                                      time_base_ms_t   * const time_base    );
                
//******************************** Declaring ********************************//



#endif // End of __BSP_LED_DRIVER_H__

