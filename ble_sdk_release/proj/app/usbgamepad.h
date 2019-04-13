/*
 * usbgamepad.h
 *
 *  Created on: 2017-3-8
 *      Author: Telink
 */

#ifndef USBGAMEPAD_H_
#define USBGAMEPAD_H_

typedef struct {
    u8 z;
    u8 Rz;
    u8 x ;
    u8 y;
    u16 gamekey;   // include 16 keys
} usbgamepad_hid_report_t;

int usbgamepad_report(u8 *data);


#endif /* USBGAMEPAD_H_ */
