#include "../../proj/tl_common.h"
#include "../../proj_lib/ble/ll/ll.h"
#include "../../proj_lib/ble/blt_config.h"
#include "../../proj_lib/ble/service/ble_ll_ota.h"

#if(__PROJECT_8261_MODULE__ || __PROJECT_8266_MODULE__ || __PROJECT_8267_MODULE__ || __PROJECT_8269_MODULE__)

typedef struct
{
  /** Minimum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms) */
  u16 intervalMin;
  /** Maximum value for the connection event (interval. 0x0006 - 0x0C80 * 1.25 ms) */
  u16 intervalMax;
  /** Number of LL latency connection events (0x0000 - 0x03e8) */
  u16 latency;
  /** Connection Timeout (0x000A - 0x0C80 * 10 ms) */
  u16 timeout;
} gap_periConnectParams_t;

const u16 clientCharacterCfgUUID = GATT_UUID_CLIENT_CHAR_CFG;

const u16 extReportRefUUID = GATT_UUID_EXT_REPORT_REF;

const u16 reportRefUUID = GATT_UUID_REPORT_REF;

const u16 characterPresentFormatUUID = GATT_UUID_CHAR_PRESENT_FORMAT;

const u16 my_primaryServiceUUID = GATT_UUID_PRIMARY_SERVICE;

static const u16 my_characterUUID = GATT_UUID_CHARACTER;

const u16 my_devServiceUUID = SERVICE_UUID_DEVICE_INFORMATION;

const u16 my_PnPUUID = CHARACTERISTIC_UUID_PNP_ID;

const u16 my_devNameUUID = GATT_UUID_DEVICE_NAME;

//device information
const u16 my_gapServiceUUID = SERVICE_UUID_GENERIC_ACCESS;
// Device Name Characteristic Properties
static u8 my_devNameCharacter = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
// Appearance Characteristic Properties
const u16 my_appearanceUIID = 0x2a01;
const u16 my_periConnParamUUID = 0x2a04;
static u8 my_appearanceCharacter = CHAR_PROP_READ;
// Peripheral Preferred Connection Parameters Characteristic Properties
static u8 my_periConnParamChar = CHAR_PROP_READ;
u16 my_appearance = GAP_APPEARE_UNKNOWN;
gap_periConnectParams_t my_periConnParameters = {20, 40, 0, 1000};


const u16 my_gattServiceUUID = SERVICE_UUID_GENERIC_ATTRIBUTE;
const u8  serviceChangedProp = CHAR_PROP_INDICATE;
const u16 serviceChangeUIID = GATT_UUID_SERVICE_CHANGE;
u16 serviceChangeVal[2] = {0};
static u8 serviceChangeCCC[2]={0,0};



#define DEV_NAME                        "tModule"
extern u8  ble_devName[];

// Device Name Characteristic Properties
static u8 my_PnPCharacter = CHAR_PROP_READ;


const u8	my_PnPtrs [] = {0x02, 0x8a, 0x24, 0x66, 0x82, 0x01, 0x00};

//////////////////////// Battery /////////////////////////////////////////////////
const u16 my_batServiceUUID       			= SERVICE_UUID_BATTERY;
static u8 my_batProp 						= CHAR_PROP_READ | CHAR_PROP_NOTIFY;
const u16 my_batCharUUID       				= CHARACTERISTIC_UUID_BATTERY_LEVEL;
u8 		  my_batVal[1] 						= {99};

/////////////////////////////////////////////////////////
const u16 userdesc_UUID		= GATT_UUID_CHAR_USER_DESC;

/////////////////////////////////////////spp/////////////////////////////////////
#if (TELIK_SPP_SERVICE_ENABLE)
u8 TelinkSppServiceUUID[16]	      = TELINK_SPP_UUID_SERVICE;
u8 TelinkSppDataServer2ClientUUID[16]    = TELINK_SPP_DATA_SERVER2CLIENT;
u8 TelinkSppDataClient2ServerUUID[16]    = TELINK_SPP_DATA_CLIENT2SERVER;

// Spp data from Server to Client characteristic variables
static u8 SppDataServer2ClientProp = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
u8  SppDataServer2ClientData[ATT_MTU_SIZE - 3];
static u8 SppDataServer2ClientDataCCC[2] = {0};

// Spp data from Client to Server characteristic variables
//CHAR_PROP_WRITE: Need response from slave, low transmission speed
static u8 SppDataClient2ServerProp = CHAR_PROP_READ | CHAR_PROP_WRITE_WITHOUT_RSP; //CHAR_PROP_WRITE;
u8  SppDataClient2ServerData[ATT_MTU_SIZE - 3];

//SPP data descriptor
const u8 TelinkSPPS2CDescriptor[] = "Telink SPP: Module->Phone";
const u8 TelinkSPPC2SDescriptor[] = "Telink SPP: Phone->Module";

u32 	spp_err = 0;
void	spp_test_read (u8 *p, int n)
{
	static u32 spp_err_st;
	static u32 spp_read = 0;
	u32 seq;
	memcpy (&seq, p, 4);
	if (spp_read != seq)
	{
		spp_err++;
	}
	else
	{
		for (int i=4; i<n; i++)
		{
			if ((u8)(p[0] + i) != p[i])
			{
				spp_err++;
				spp_err_st += BIT(16);
				break;
			}
		}
	}
	spp_read = seq - 1;

	if (0 && spp_err > 1)
	{
#if __PROJECT_8266_MODULE__
		gpio_set_input_en(GPIO_URX, 0);
#else
		gpio_set_input_en(GPIO_URXB3, 0);
#endif
		irq_disable ();
		while (1);
	}
}

int module_onReceiveData(rf_packet_att_write_t *p)
{
	u32 n;
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
		header = 0x07a0;		//data received event
		header |= (3 << 16) | (1<<24);
		spp_test_read (&p->value, len);
		extern int hci_send_data (u32 h, u8 *para, int n);
		hci_send_data(header, &p->opcode, len + 3);		//HCI_FLAG_EVENT_TLK_MODULE
	}


	return 0;
}
#endif
// Include attribute (Battery service)
static u16 include[3] = {0x0026, 0x0028, SERVICE_UUID_BATTERY};


const u8 my_OtaServiceUUID[16]		= TELINK_OTA_UUID_SERVICE;
const u8 my_OtaUUID[16]		= TELINK_SPP_DATA_OTA;

static u8 my_OtaProp		= CHAR_PROP_READ | CHAR_PROP_WRITE_WITHOUT_RSP;
const u8  my_OtaName[] = {'O', 'T', 'A'};
u8	 	my_OtaData 		= 0x00;


// TM : to modify
const attribute_t my_Attributes[] = {
#if (TELIK_SPP_SERVICE_ENABLE)
	{26,0,0,0,0,0},	// total num of attribute
#else
	{18,0,0,0,0,0},	// total num of attribute
#endif

	// 0001 - 0007  gap
	{7,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_gapServiceUUID), 0},
	{0,ATT_PERMISSIONS_READ,2,1,(u8*)(&my_characterUUID), 		(u8*)(&my_devNameCharacter), 0},
	{0,ATT_PERMISSIONS_READ,2,MAX_DEV_NAME_LEN, (u8*)(&my_devNameUUID), (u8*)(&ble_devName), 0},
	{0,ATT_PERMISSIONS_READ,2,1,(u8*)(&my_characterUUID), 		(u8*)(&my_appearanceCharacter), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_appearance), (u8*)(&my_appearanceUIID), 	(u8*)(&my_appearance), 0},
	{0,ATT_PERMISSIONS_READ,2,1,(u8*)(&my_characterUUID), 		(u8*)(&my_periConnParamChar), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_periConnParameters),(u8*)(&my_periConnParamUUID), 	(u8*)(&my_periConnParameters), 0},


	// 0008 - 000b gatt
	{4,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_gattServiceUUID), 0},
	{0,ATT_PERMISSIONS_READ,2,1,(u8*)(&my_characterUUID), 		(u8*)(&serviceChangedProp), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (serviceChangeVal), (u8*)(&serviceChangeUIID), 	(u8*)(&serviceChangeVal), 0},
	{0,ATT_PERMISSIONS_RDWR,2,sizeof (serviceChangeCCC),(u8*)(&clientCharacterCfgUUID), (u8*)(serviceChangeCCC), 0},



	// 000c - 000e  device Information Service
	{3,ATT_PERMISSIONS_READ,2,2,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_devServiceUUID), 0},
	{0,ATT_PERMISSIONS_READ,2,1,(u8*)(&my_characterUUID), 		(u8*)(&my_PnPCharacter), 0},
	{0,ATT_PERMISSIONS_READ,2,sizeof (my_PnPtrs),(u8*)(&my_PnPUUID), (u8*)(my_PnPtrs), 0},

////////////////////////////////////// SPP Service /////////////////////////////////////////////////////
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

	// OTA
	{4,ATT_PERMISSIONS_READ, 2,16,(u8*)(&my_primaryServiceUUID), 	(u8*)(&my_OtaServiceUUID), 0},
	{0,ATT_PERMISSIONS_READ, 2, 1,(u8*)(&my_characterUUID), 		(u8*)(&my_OtaProp), 0},				//prop
	{0,ATT_PERMISSIONS_RDWR,16,sizeof(my_OtaData),(u8*)(&my_OtaUUID),	(&my_OtaData), &otaWrite, &otaRead},			//value
	{0,ATT_PERMISSIONS_READ, 2,sizeof (my_OtaName),(u8*)(&userdesc_UUID), (u8*)(my_OtaName), 0},

};

void	my_att_init ()
{
	bls_att_setAttributeTable ((u8 *)my_Attributes);

	u8 device_name[] = DEV_NAME;
	bls_att_setDeviceName(device_name, sizeof(DEV_NAME));
}

#endif
