/*
 * blt_pa.h
 *
 *  Created on: 2017-8-14
 *      Author: Administrator
 */

#ifndef BLT_PA_H_
#define BLT_PA_H_

#include "../tl_common.h"


#ifndef PA_ENABLE
#define PA_ENABLE                           0
#endif



#ifndef PA_TXEN_PIN
#define PA_TXEN_PIN                         GPIO_PB2
#endif

#ifndef PA_RXEN_PIN
#define PA_RXEN_PIN                         GPIO_PB3
#endif



#define PA_TYPE_OFF							0
#define PA_TYPE_TX_ON						1
#define PA_TYPE_RX_ON						2


typedef void (*rf_pa_callback_t)(int type);
extern rf_pa_callback_t  blc_rf_pa_cb;



void rf_pa_init(void);


#endif /* BLT_PA_H_ */
