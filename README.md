以826x_module为源码来进行解读。

如果要编译826x_module的代码，需要把这部分代码设置成active.

设置方法：Properties->C/C++ Build -> Manage Configurations,将对应的模块设置成active.

#### 初始化

vendor/826x_module/main.c

```
int main (void) {
	blc_pm_select_internal_32k_crystal();

	cpu_wakeup_init(CRYSTAL_TYPE);

	set_tick_per_us (CLOCK_SYS_CLOCK_HZ/1000000);
	clock_init();

	gpio_init();

	rf_drv_init(CRYSTAL_TYPE);

	user_init ();

    irq_enable();

	while (1) {
#if (MODULE_WATCHDOG_ENABLE)
		wd_clear(); //clear watch dog
#endif
		main_loop ();
	}
}
```

vendor/826x_module/app.c

```
void user_init()
{
	blc_app_loadCustomizedParameters();  //load customized freq_offset cap value and tp value

	REG_ADDR8(0x74) = 0x53;
	REG_ADDR16(0x7e) = 0x08d1;
	REG_ADDR8(0x74) = 0x00;
	usb_log_init ();
	usb_dp_pullup_en (1);  //open USB enum

	led_init();

	// MAC地址计算
	u8  tbl_mac [] = {0xe1, 0xe1, 0xe2, 0xe3, 0xe4, 0xc7};
	u32 *pmac = (u32 *) CFG_ADR_MAC;
	// 读取MAC地址
	if (*pmac != 0xffffffff)
	{
	    memcpy (tbl_mac, pmac, 6);
	}
    else
    {
    	// 随机生成mac地址，写入flash中
        //TODO : should write mac to flash after pair OK
        tbl_mac[0] = (u8)rand();
        flash_write_page (CFG_ADR_MAC, 6, tbl_mac);
    }


///////////// BLE stack Initialization ////////////////
	////// Controller Initialization  //////////
	// 初始化几个状态
	// 初始状态，init
	blc_ll_initBasicMCU(tbl_mac);   //mandatory
	// ADV状态
	blc_ll_initAdvertising_module(tbl_mac); 	//adv module: 		 mandatory for BLE slave,
	// slave conn状态
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,
	// powermanage使能
	blc_ll_initPowerManagement_module();        //pm module:      	 optional


	////// Host Initialization  //////////
	extern void my_att_init ();
	// 属性表格初始化。
	my_att_init (); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization
#if SIG_PROC_ENABLE
	blc_l2cap_reg_att_sig_hander(att_sig_proc_handler);         //register sig process handler
#endif
	//smp initialization
#if ( SMP_DISABLE )
	//bls_smp_enableParing (SMP_PARING_DISABLE_TRRIGER );
#elif ( SMP_JUST_WORK || SMP_PASSKEY_ENTRY )
	//Just work encryption: TK default is 0, that is, pin code defaults to 0, without setting
	//Passkey entry encryption: generate random numbers, or set the default pin code, processed in the event_handler function
	bls_smp_enableParing (SMP_PARING_CONN_TRRIGER );
	#if (SMP_PASSKEY_ENTRY )
		blc_smp_enableAuthMITM (1, 123456);
	#endif
#endif

	///////////////////// USER application initialization ///////////////////
	// 设置广播名称
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	// 设置scan response名称
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));




	////////////////// config adv packet /////////////////////
	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS + 16,	// 广播时间间隔，在最小和最大时间间隔之间变化
								 ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,	// 广播类型
								 0,  NULL,
								 MY_APP_ADV_CHANNEL,	// 广播通道
								 ADV_FP_NONE);		// 白名单

	if(status != BLE_SUCCESS)
	{
		write_reg8(0x8000, 0x11);
		while(1);
	}  //debug: adv setting err


    printf("\n\rAdv parameters setting success!\n\r");
	bls_ll_setAdvEnable(1);  //adv enable		// 广播使能
	printf("Enable ble adv!\n\r");
	rf_set_power_level_index (RF_POWER_8dBm);		// RF的功率设置
	// SUSPEND_DISABLE:禁用SUSPEND模式，不会进入低功耗
	// SUSPEND_ADV: 广播的时候可以进入低功耗
	// SUSPEND_CONN: 连接状态下可以进入低功耗
	bls_pm_setSuspendMask (SUSPEND_DISABLE);//(SUSPEND_ADV | SUSPEND_CONN)


	////////////////// SPP initialization ///////////////////////////////////
/*	#if (HCI_ACCESS==HCI_USE_USB)
		blt_set_bluetooth_version (BLUETOOTH_VER_4_2);
		bls_ll_setAdvChannelMap (BLT_ENABLE_ADV_37);
		usb_bulk_drv_init (0);
		blc_register_hci_handler (blc_hci_rx_from_usb, blc_hci_tx_to_usb);
		bls_smp_enableParing (SMP_PARING_CONN_TRRIGER );
	#else	//uart */
		//one gpio should be configured to act as the wakeup pin if in power saving mode; pending
		//todo:uart init here
#if __PROJECT_8266_MODULE__
		gpio_set_func(GPIO_UTX, AS_UART);
		gpio_set_func(GPIO_URX, AS_UART);
		gpio_set_input_en(GPIO_UTX, 1);
		gpio_set_input_en(GPIO_URX, 1);
		gpio_write (GPIO_UTX, 1);			//pull-high RX to avoid mis-trig by floating signal
		gpio_write (GPIO_URX, 1);			//pull-high RX to avoid mis-trig by floating signal
#else
		// 自己修改了一下，使用PA6和PA7作为串口使用
		gpio_set_input_en(GPIO_PA6, 1);
		gpio_set_input_en(GPIO_PA7, 1);
		gpio_setup_up_down_resistor(GPIO_PA6, PM_PIN_PULLUP_1M);
		gpio_setup_up_down_resistor(GPIO_PA7, PM_PIN_PULLUP_1M);
		uart_io_init(UART_GPIO_8267_PA6_PA7);
#endif
		// 清中断标志位
		reg_dma_rx_rdy0 = FLD_DMA_UART_RX | FLD_DMA_UART_TX; //clear uart rx/tx status
		CLK16M_UART115200;		// 波特率
		// 接受和发送缓存初始化
		uart_BuffInit(hci_rx_fifo_b, hci_rx_fifo.size, hci_tx_fifo_b);
		extern int rx_from_uart_cb (void);
		extern int tx_to_uart_cb (void);
		// 中断处理函数，回调函数，在DMA中断接受完之后，调用
		blc_register_hci_handler(rx_from_uart_cb,tx_to_uart_cb);				//customized uart handler
//	#endif

	extern int event_handler(u32 h, u8 *para, int n);
	// 事件处理函数
	blc_hci_registerControllerEventHandler(event_handler);		//register event callback
	bls_hci_mod_setEventMask_cmd(0xffff);			//enable all 15 events,event list see ble_ll.h

	// OTA init
	bls_ota_clearNewFwDataArea(); //must
	bls_ota_registerStartCmdCb(entry_ota_mode);
	bls_ota_registerResultIndicateCb(show_ota_result);



#if (BLE_MODULE_PM_ENABLE)
	//mcu can wake up module from suspend or deepsleep by pulling up GPIO_WAKEUP_MODULE
	gpio_set_wakeup		(GPIO_WAKEUP_MODULE, 1, 1);  // core(gpio) high wakeup suspend
	cpu_set_gpio_wakeup (GPIO_WAKEUP_MODULE, 1, 1);  // pad high wakeup deepsleep

	GPIO_WAKEUP_MODULE_LOW;

	bls_pm_registerFuncBeforeSuspend( &app_suspend_enter );
#endif

#if (BATT_CHECK_ENABLE)
	#if((MCU_CORE_TYPE == MCU_CORE_8261)||(MCU_CORE_TYPE == MCU_CORE_8267)||(MCU_CORE_TYPE == MCU_CORE_8269))
		adc_BatteryCheckInit(ADC_CLK_4M, 1, Battery_Chn_VCC, 0, SINGLEEND, RV_1P428, RES14, S_3);
	#elif(MCU_CORE_TYPE == MCU_CORE_8266)
		adc_Init(ADC_CLK_4M, ADC_CHN_D2, SINGLEEND, ADC_REF_VOL_1V3, ADC_SAMPLING_RES_14BIT, ADC_SAMPLING_CYCLE_6);
	#endif
#endif
}
```

### 串口数据发送过程

telink收到串口数据，再把串口数据通过蓝牙发送出去。

串口中断函数

vendor/826x_module/main.c
```
_attribute_ram_code_ void irq_handler(void)
{
	irq_blt_sdk_handler ();

#if (HCI_ACCESS==HCI_USE_UART)
	unsigned char irqS = reg_dma_rx_rdy0;
    // 刚开始接受数据就会进入中断
	if(irqS & FLD_DMA_UART_RX)	//rx
    {
    	// 清中断
    	reg_dma_rx_rdy0 = FLD_DMA_UART_RX;
    	// 获取fifo的地址
    	u8* w = hci_rx_fifo.p + (hci_rx_fifo.wptr & (hci_rx_fifo.num-1)) * hci_rx_fifo.size;
    	// 判断fifo中数据是否为空
    	if(w[0]!=0)
    	{
    		my_fifo_next(&hci_rx_fifo);
    		u8* p = hci_rx_fifo.p + (hci_rx_fifo.wptr & (hci_rx_fifo.num-1)) * hci_rx_fifo.size;
    		// 把fifo中数据的地址放到DMA里面，接收的数据从外设保存到DMA指定的FIFO中
    		// 后面接受的数据就会保存在fifo里面
    		reg_dma0_addr = (u16)((u32)p);
    	}
    }

    if(irqS & FLD_DMA_UART_TX)	//tx
    {
    	// 清中断
    	reg_dma_rx_rdy0 = FLD_DMA_UART_TX;
#if __PROJECT_8266_MODULE__
		uart_clr_tx_busy_flag();
#endif
    }
#endif
}

```

中断处理回调函数

定义位置

```
user_init 
// 中断处理函数，回调函数，在DMA中断接受完之后，调用
		blc_register_hci_handler(rx_from_uart_cb,tx_to_uart_cb);				//customized uart handler
```

回调函数处理数据

vendor/826x_module/spp.c

```
int rx_from_uart_cb (void)//UART data send to Master,we will handler the data as CMD or DATA
{
	if(my_fifo_get(&hci_rx_fifo) == 0)
	{
		return 0;
	}

	u8* p = my_fifo_get(&hci_rx_fifo);
	// 获取数据长度接受数据的开始4个字节是数据长度
	u32 rx_len = p[0]; //usually <= 255 so 1 byte should be sufficient

	if (rx_len)
	{
		// 处理数据， 第一个参数是数据的首地址，第二个参数是除去开头四个字节的长度
		bls_uart_handler(&p[4], rx_len - 4);
		my_fifo_pop(&hci_rx_fifo);
	}

	return 0;
}

///////////////////////////////////////////the default bls_uart_handler///////////////////////////////
int bls_uart_handler (u8 *p, int n)
{
	u8  status = BLE_SUCCESS;
	int  cmdLen;
	u8 *cmdPara;
	// 这一部分的代码根据格式进行修改，如果是纯数据，这一部分可以不要
	// 直接把数据通过蓝牙发送出去就好，不需要判断那么多的cmd，header
	// 命令
	u16 cmd = p[0]|p[1]<<8;
	u32	header = 0;
	u8	para[16] = {0};
	u8 para_len = 1;
	cmdPara = p + 4;
	// 长度
	cmdLen = p[2] | p[3]<<8;
	header = ((p[0] + p[1] * 256) & 0x3ff) | 0x400;		//event complete
	header |= (3 << 16) | HCI_FLAG_EVENT_TLK_MODULE;
	// set advertising interval: 01 ff 02 00 50 00: 80 *0.625ms
	if (cmd == SPP_CMD_SET_ADV_INTV)
	{
		u8 interval = cmdPara[0] ;
		status = bls_ll_setAdvInterval(interval, interval);
	}
	// set advertising data: 02 ff 06 00 01 02 03 04 05 06
	else if (cmd == SPP_CMD_SET_ADV_DATA)
	{
		status = (u8)bls_ll_setAdvData(cmdPara, p[2]);
	}
	// enable/disable advertising: 0a ff 01 00  01
	else if (cmd == SPP_CMD_SET_ADV_ENABLE)
	{
		status = (u8)bls_ll_setAdvEnable(cmdPara[0]);
		para[0] = status;
	}
	// send data: 0b ff 05 00  01 02 03 04 05
	//change format to 0b ff 07 handle(2bytes) 00 01 02 03 04 05
	else if (cmd == 0xFF0B)
	{

	}
	// get module available data buffer: 0c ff 00  00
	else if (cmd == SPP_CMD_GET_BUF_SIZE)
	{
		u8 r[4];
		para[0] = (u8)blc_hci_le_readBufferSize_cmd( (u8 *)(r) );
		para[1] = r[2];
		para_len = 2;
	}
	// set advertising type: 0d ff 01 00  00
	else if (cmd == SPP_CMD_SET_ADV_TYPE)
	{
		status = bls_ll_setAdvType(cmdPara[0]);
	}
	// set advertising addr type: 0e ff 01 00  00
	else if (cmd == SPP_CMD_SET_ADV_ADDR_TYPE)
	{
		status = blt_set_adv_addrtype(cmdPara[0]);
	}
	// set advertising direct initiator address & addr type: 0e ff 07 00  00(public; 1 for random) 01 02 03 04 05 06
	else if (cmd == SPP_CMD_SET_ADV_DIRECT_ADDR)
	{
		status = blt_set_adv_direct_init_addrtype(cmdPara);
	}
	// add white list entry: 0f ff 07 00 01 02 03 04 05 06
	else if (cmd == SPP_CMD_ADD_WHITE_LST_ENTRY)
	{
		status = (u8)ll_whiteList_add(cmdPara[0], cmdPara + 1);
	}
	// delete white list entry: 10  ff 07 00 01 02 03 04 05 06
	else if (cmd == SPP_CMD_DEL_WHITE_LST_ENTRY)
	{
		status = (u8)ll_whiteList_delete(cmdPara[0], cmdPara + 1);
	}
	// reset white list entry: 11 ff 00 00
	else if (cmd == SPP_CMD_RST_WHITE_LST)
	{
		status = (u8)ll_whiteList_reset();
	}
	// set filter policy: 12 ff 10 00 00(bit0: scan WL enable; bit1: connect WL enable)
	else if (cmd == SPP_CMD_SET_FLT_POLICY)
	{
		status = bls_ll_setAdvFilterPolicy(cmdPara[0]);
	}
	// set device name: 13 ff 0a 00  01 02 03 04 05 06 07 08 09 0a
	else if (cmd == SPP_CMD_SET_DEV_NAME)
	{
		status = bls_att_setDeviceName(cmdPara,p[2]);
	}
	// get connection parameter: 14 ff 00 00
	else if (cmd == SPP_CMD_GET_CONN_PARA)
	{
		blt_get_conn_para(para+1);
		para_len = 11;
	}
	// set connection parameter: 15 ff 08 00 a0 00 a2 00 00 00 2c 01 (min, max, latency, timeout)
	else if (cmd == SPP_CMD_SET_CONN_PARA)
	{
		bls_l2cap_requestConnParamUpdate(cmdPara[0]|cmdPara[1]<<8,cmdPara[2]|cmdPara[3]<<8,cmdPara[4]|cmdPara[5]<<8,cmdPara[6]|cmdPara[7]<<8);
	}
	// get module current work state: 16 ff 00 00
	else if (cmd == SPP_CMD_GET_CUR_STATE)
	{
		para[1] = blc_ll_getCurrentState();
		para_len = 2;
	}
	// terminate connection: 17 ff 00 00
	else if (cmd == SPP_CMD_TERMINATE)
	{
		bls_ll_terminateConnection(HCI_ERR_REMOTE_USER_TERM_CONN);
	}
	// restart module: 18 ff 00 00
	else if (cmd == SPP_CMD_RESTART_MOD)
	{
		cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_TIMER, clock_time() + 10000 * sys_tick_per_us);
	}
	// enable/disable MAC binding function: 19 ff 01 00 00(disable, 01 enable)
	else if (cmd == 0x19)
	{

	}
	// add MAC address to binding table: 1a ff 06 00 01 02 03 04 05 06
	else if (cmd == 0x1a)
	{

	}
	// delete MAC address from binding table: 1b ff 06 00 01 02 03 04 05 06
	else if (cmd == 0x1b)
	{

	}
	// 发送数据，通过NOTIFY发出
	//change format to 1c ff 07 00 11 00 01 02 03 04 05
	else if (cmd == SPP_CMD_SEND_NOTIFY_DATA)
	{
		// 判断数据长度
		if (cmdLen > 42)
		{
			status = 2;			//data too long
		}
		else
		{
			// 第一个参数Handler,att_table定义的，第二个参数数据首地址，第三个参数，数据长度
			status = bls_att_pushNotifyData( cmdPara[0] | (cmdPara[1]<<8), cmdPara + 2,  cmdLen - 2);
		}
	}
	para[0] = status;
	hci_send_data (header, para, para_len);
	return 0;
}
```

### 蓝牙接收数据过程

实验方法，蓝牙调试助手连接，通过向对应的UID中写数据，telink会把收到的数据通过串口发送。

调用过程，首先

```
const attribute_t my_Attributes[] = {
#if (TELIK_SPP_SERVICE_ENABLE)
	{8,ATT_PERMISSIONS_READ,2,16,(u8*)(&my_primaryServiceUUID), 	(u8*)(&TelinkSppServiceUUID), 0},

	{0,ATT_PERMISSIONS_READ,2,1,(u8*)(&my_characterUUID), 		(u8*)(&SppDataServer2ClientProp), 0},				//prop
	{0,ATT_PERMISSIONS_READ,16,sizeof(SppDataServer2ClientData),(u8*)(&TelinkSppDataServer2ClientUUID), (u8*)(SppDataServer2ClientData), 0},	//value
	{0,ATT_PERMISSIONS_RDWR,2,2,(u8*)&clientCharacterCfgUUID,(u8*)(&SppDataServer2ClientDataCCC)},
	{0,ATT_PERMISSIONS_READ,2,sizeof(TelinkSPPS2CDescriptor),(u8*)&userdesc_UUID,(u8*)(&TelinkSPPS2CDescriptor)},

	{0,ATT_PERMISSIONS_READ,2,1,(u8*)(&my_characterUUID), 		(u8*)(&SppDataClient2ServerProp), 0},				//prop
	{0,ATT_PERMISSIONS_RDWR,16,sizeof(SppDataClient2ServerData),(u8*)(&TelinkSppDataClient2ServerUUID), (u8*)(SppDataClient2ServerData), &module_onReceiveData},	//value
	{0,ATT_PERMISSIONS_READ,2,sizeof(TelinkSPPC2SDescriptor),(u8*)&userdesc_UUID,(u8*)(&TelinkSPPC2SDescriptor)},
#endif
```

写操作的回调函数

```
// 蓝牙回调函数
int module_onReceiveData(rf_packet_att_write_t *p)
{
	u32 n;
	// 计算接受数据的长度
	u8 len = p->l2capLen - 3;
	if(len > 0)
	{
#if 0
		static u32 sn = 0;
		memcpy (&n, &p->value, 4);
		if (sn != n)
		{
			sn = 0;
			bls_ll_terminateConnection (0x13);
		}
		else
		{
			sn = n + 1;
		}
#endif
		u32 header;
		// 数据接收事件的header
		header = 0x07a0;		//data received event
		header |= (3 << 16) | (1<<24);
		spp_test_read (&p->value, len);
		extern int hci_send_data (u32 h, u8 *para, int n);
		// 发送数据，因为已经在中断处理函数中和DMA地址关联，所以直接改里面的参数就可以发送了
		hci_send_data(header, &p->opcode, len + 3);		//HCI_FLAG_EVENT_TLK_MODULE
	}


	return 0;
}
```

```
int hci_send_data (u32 h, u8 *para, int n)
{
	// 发完数据以后，重新更改指针的位置
	u8 *p = my_fifo_wptr (&hci_tx_fifo);
	if (!p || n >= hci_tx_fifo.size)
	{
		return -1;
	}

#if (BLE_MODULE_INDICATE_DATA_TO_MCU)
	if(!module_uart_data_flg){ //UART idle, new data is sent
		GPIO_WAKEUP_MCU_HIGH;  //Notify MCU that there is data here
		module_wakeup_module_tick = clock_time() | 1;
		module_uart_data_flg = 1;
	}
#endif

	int nl = n + 4;
	if (h & HCI_FLAG_EVENT_TLK_MODULE)
	{
		*p++ = nl;
		*p++ = nl >> 8;
		*p++ = 0xff;
		*p++ = n + 2;
		*p++ = h;
		*p++ = h>>8;
		memcpy (p, para, n);
		p += n;
	}
	my_fifo_next (&hci_tx_fifo);
	return 0;
}

uart_data_t T_txdata_buf;
// 发送数据
int tx_to_uart_cb (void)
{
	// 获取发送数据的首地址
	u8 *p = my_fifo_get (&hci_tx_fifo);
	if (p && !uart_tx_is_busy ())
	{
		// 拷贝数据到data中
		memcpy(&T_txdata_buf.data, p + 2, p[0]+p[1]*256);
		// 数据的长度，这里的长度和fifo里面定义的有些区别，fifo是前两字节保存长度，而接受的数据中是前4字节保存长度
		T_txdata_buf.len = p[0]+p[1]*256 ;


#if (BLE_MODULE_INDICATE_DATA_TO_MCU)
		//If the MCU side is designed to have low power consumption and the module has data to pull up
		//the GPIO_WAKEUP_MCU will only wake up the MCU, then you need to consider whether MCU needs a
		//reply time T from wakeup to a stable receive UART data. If you need a response time of T, ch-
		//ange the following 100US to the actual time required by user.
		if(module_wakeup_module_tick){
			while( !clock_time_exceed(module_wakeup_module_tick, 100) );
		}
#endif


#if __PROJECT_8266_MODULE__
		if (uart_Send_kma((u8 *)(&T_txdata_buf)))
#else
			// uart发送数据
		if (uart_Send((u8 *)(&T_txdata_buf)))
#endif
		{
			// 发送完数据以后，FIFO的数据就多一个可以使用
			my_fifo_pop (&hci_tx_fifo);
		}
	}
	return 0;
}
```
