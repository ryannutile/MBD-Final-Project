/**
 *@file gpio_pwm.h
 *
 *@brief
 *	- GPIO pwm code
 *
 * Target:   Zedboard
 *
 *@author    Rohan Kangralkar, ECE, Northeastern University
 *@date      07/08/2010
 *
 * LastChange:
 * $Id$
 *
 *******************************************************************************/
#ifndef GPIO_TTC_H_
#define GPIO_TTC_H_
/** gpio_init
 *
 * Initialization of PORTFIO. This PORT is used as GPIO.
 * The output and input direction can be set using the MACROS
 *
 * Parameters:
 *
 * @return void
 */
void gpio_init(void);

/** gpio_init
 *
 * The main command loop. Write all the control commands in this function
 *
 * Parameters:
 *
 * @return void
 */
void gpio_run(void);


void ttc_init(void);
void ttc_start(void);

/************************** Constant Definitions *****************************/

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are only defined here such that a user can easily
 * change all the needed parameters in one place.
 */
 
// Definitions of Triple Timer Counter MMR, Refer Pg 1769 in Zynq TRM.

// /** Base address for TTC0 peripherals*/
#define TTC0_T0_BASE 0xF8001000
#define TTC0_T0_INT_STATUS  TTC0_T0_BASE + 0x54
#define TTC0_T0_INT_EN		TTC0_T0_BASE + 0x60

/******** Set Clock Control Definition ********************/
#define SET_CLK_CNTRL_VAL (TTC0_T0_BASE + 0x00000000U)  /**< Clock Control Register*/


/******** Set Counter Control Definition ********************/
#define SET_CNT_CNTRL_VAL (TTC0_T0_BASE + 0x0C)  /**< Counter Control Register*/


/*If interval is enabled, this is the maximum value
that the counter will count up to or down from.
*/
#define SET_INTERVAL_VAL (TTC0_T0_BASE + 0x24)  /**< Interval Count Value */

#endif /* GPIO_PWM_H_ */


