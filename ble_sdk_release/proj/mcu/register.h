#pragma once
#include "../config/user_config.h"

#if(__TL_LIB_8266__ || MCU_CORE_TYPE == MCU_CORE_8266)
#include "../mcu_spec/register_8266.h"
#elif(__TL_LIB_8267__ || MCU_CORE_TYPE == MCU_CORE_8267 || \
	  __TL_LIB_8261__ || MCU_CORE_TYPE == MCU_CORE_8261 || \
	  __TL_LIB_8269__ || MCU_CORE_TYPE == MCU_CORE_8269 )
#include "../mcu_spec/register_8267.h"
#elif(__TL_LIB_8366__ || MCU_CORE_TYPE == MCU_CORE_8366)
#include "../mcu_spec/register_8366.h"
#elif(__TL_LIB_8263__ || MCU_CORE_TYPE == MCU_CORE_8263)
#include "../mcu_spec/register_8263.h"
#endif

