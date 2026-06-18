/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bsp_led_handler.c
 * 
 * @par dependencies 
 * - bsp_handler_driver.h
 * 
 * 
 * @author Jack | R&D Dept. | EternalChip ??????????
 * 
 * @brief Provide the APIs of LEDs and corresponding operations.
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

//******************************** Includes *********************************//
#include "bsp_led_handler.h"


//******************************** Includes *********************************//

//******************************** Defines **********************************//

static led_handler_status_t __array_init(bsp_led_driver_t * array[],
                                         uint32_t array_size        )
{
    for(int i = 0; i < array_size; i++)
    {
        array[i] = (bsp_led_driver_t *)INIT_PATTERN;
    }
    //TBD valid the memory to check if can access;
    return HANDLER_OK;
}

/**
 * @brief Regiter the target of led_driver_instance.
 * 
 * Steps:
 *  1, .
 * 
 * @param[in] led_driver      : Pointer to the input data.
 * 
 * @return led_handler_status_t : The status of running
 * 
 * */
led_handler_status_t led_register ( bsp_led_handler_t  * const        self,
                                    bsp_led_driver_t   * const  led_driver, 
                                    led_index_t        * const       index)
{
#ifdef DEBUG
        DEBUG_OUT("Start_led_register \r\n");
#endif // DEBUG
    led_handler_status_t ret = HANDLER_OK;
    /*************0.Checing the target statues  ******************************/
    //1.check if the target has been instantiated
    if( NULL       == led_driver          ||
        NULL       == index               ||
        NOT_INITED == self->is_inited      ) 
    {
        //2.if not instantiated, return error to caller 
        //TBD:3.option - mutex to upgrade low priority task to init target ASAP
#ifdef DEBUG
        DEBUG_OUT("LED_ERRORPARAMETER \r\n");
#endif // DEBUG
        ret = HANDLER_ERRORPARAMETER;
        return ret;
    }

    /*************1.Checing the input parameters******************************/
    if ( INITED != led_driver->is_inited )
    {
        //2.if instantiated, return error to caller
        //TBD:3.option - mutex to upgrade low priority task to init target ASAP
#ifdef DEBUG
        DEBUG_OUT("HANDLER_ERRORRESOURCE \r\n");
#endif // DEBUG
        ret = HANDLER_ERRORRESOURCE;
        return ret;
    }


    /*************2.Adding the instance in target array **********************/
    if( ( MAX_INSTANCE_NUMBER - self->instances.led_instance_num ) == 0 )
    {
        ret = HANDLER_ERRORRESOURCE;
        return ret;
    }
    



#ifdef OS_SUPPORTING
    self->p_os_critical->pf_os_critical_enter();
#endif //OS_SUPPORTING

    if( (MAX_INSTANCE_NUMBER - self->instances.led_instance_num) > 0  )
    {
        self->instances.led_instance_group[self->instances.led_instance_num] \
                                                                = led_driver;
        *index = self->instances.led_instance_num;
        
        self->instances.led_instance_num++;
    } 

#ifdef OS_SUPPORTING
    self->p_os_critical->pf_os_critical_exit();
#endif //OS_SUPPORTING

    

#ifdef DEBUG
        DEBUG_OUT("led_register Succees!\r\n");
#endif // DEBUG
    return ret;
}


/**
 * @brief Control the bsp_led_driver_t target.
 * 
 * Steps:
 *  1, set the internal values of target, and do the action.
 *  
 * 
 * @param[in] self              : TBD:AI to add comments
 * @param[in] Cycle_time        : TBD:AI to add comments
 * @param[in] blink_times       : TBD:AI to add comments
 * @param[in] proportion_on_off : TBD:AI to add comments
 * 
 * @return led_status_t : The status of running
 * 
 * */
static led_status_t led_control   (
                                bsp_led_driver_t * const       self, // target
                                uint32_t      cycle_time,//     Cycle_time[ms]
                                uint32_t      blink_times,//blink_times[times]
                                proportion_t   proportion//  proportion_on_off
                                )
{
    /*************0.Checing the target statues  **********/
    led_status_t ret = LED_OK;
    //1.check if the target has been instantiated
    if( NULL       == self               || 
        NOT_INITED == self->is_inited      ) 
    {
        //2.if not instantiated, return error to caller 
        //TBD:3.option - mutex to upgrade low priority task to init target ASAP
#ifdef DEBUG
        DEBUG_OUT("LED_ERRORPARAMETER \r\n");
#endif // DEBUG
        ret = LED_ERRORPARAMETER;
        return ret;
    }

    /*************1.Checing the input parameters**********/
    //1.cycle time checking

    /*************2.Adding the data in target*************/

    /************3. run the operations of led*************/
    //1.call the fuction to blink


    return ret;
}


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
 * @param[in] os_delay  : Pointer to the input data.
 * @param[in] time_base : Pointer to the input data.
 * 
 * @return led_status_t : The status of running
 * 
 * */
led_handler_status_t led_handler_inst (
                                  bsp_led_handler_t      * const        self, 
#ifdef OS_SUPPORTING 
                                  os_delay_t             * const    os_delay,
                                  handler_os_queue_t     * const    os_queue,
                                  handler_os_critical_t  * const os_critical,
#endif //OS_SUPPORTING 
                                  handler_time_base_ms_t * const  time_base  )
{
    led_handler_status_t ret = HANDLER_OK;
#ifdef DEBUG
    DEBUG_OUT("Handler inst Kick-off \r\n");
#endif // DEBUG

/*************1.Checing the input parameters**************/
    if( NULL == self      ||
#ifdef OS_SUPPORTING
        NULL == os_delay  ||
        NULL == os_queue  ||
#endif //OS_SUPPORTING
        NULL == time_base
                              )
    {
#ifdef DEBUG
        DEBUG_OUT("HANDLER_ERRORPARAMETER\r\n");
#endif // DEBUG
        ret = HANDLER_ERRORPARAMETER;
        return ret;
    }

/*************2.Checing the Resources*********************/
    if( INITED == self->is_inited )
    {
#ifdef DEBUG
        DEBUG_OUT("HANDLER_ERRORRESOURCE\r\n");
#endif // DEBUG
        ret = HANDLER_ERRORPARAMETER;
        return ret;
    }

#ifdef DEBUG
    DEBUG_OUT("Handler inst start\r\n");
#endif  // DEBUG

/*************3.Adding the interface**********************/

    //3.1 mount external interface 
    self->p_time_base_ms         =    time_base;
    self->p_os_time_delay        =     os_delay;
    self->p_os_queue_interface   =     os_queue;
    self->p_os_critical          =  os_critical;
 
    //3.2 mount internal interface  
    self->pf_led_countroler      =  led_control;
    self->pf_led_register        = led_register;
    

/*************4.Init the target     **********************/
    //4.1 init the variables will be used
    self->instances.led_instance_num  =                 0;
    ret = __array_init(self->instances.led_instance_group,
                                     MAX_INSTANCE_NUMBER);
if( HANDLER_OK != ret )
    {
#ifdef DEBUG
        DEBUG_OUT("HANDLER_ERRORNOMEMORY\r\n");
#endif // DEBUG
        return ret;
    }

    
    self->is_inited = INITED;

#ifdef DEBUG
    DEBUG_OUT("LED handler init finished\r\n");
#endif  // DEBUG
    
    return ret;
}                     

//******************************** Defines **********************************//

//******************************** Declaring ********************************//
//******************************** Declaring ********************************//

