/*
 * app_pair.h
 *
 *  Created on: 2017-6-1
 *      Author: Administrator
 */

#ifndef APP_PAIR_H_
#define APP_PAIR_H_


int user_tbl_slave_mac_add(u8 adr_type, u8 *adr);
int user_tbl_slave_mac_search(u8 adr_type, u8 * adr);

int user_tbl_slave_mac_delete_by_adr(u8 adr_type, u8 *adr);
void user_tbl_salve_mac_unpair_proc(void);

void user_master_host_pairing_management_init(void);


#endif /* APP_PAIR_H_ */
