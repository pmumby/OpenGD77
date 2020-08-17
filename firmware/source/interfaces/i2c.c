/*
 * Copyright (C)2019 Kai Ludwig, DG4KLU
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <i2c.h>
#include "fsl_port.h"

#if defined(USE_SEGGER_RTT)
#include <SeggerRTT/RTT/SEGGER_RTT.h>
#endif

static void I2C0Setup(void);

#define I2C_DATA_LENGTH (4)  /* Maximum transfer size seems to be 3 bytes. So use 4 to pad to 32 byte boundary */
static uint8_t i2c_master_buff[I2C_DATA_LENGTH];
volatile int isI2cInUse = 0;

void I2C0aInit(void)
{
    // I2C0a to AT24C512 EEPROM & AT1846S
    const port_pin_config_t porte24_config = {/* Internal pull-up resistor is enabled */
                                              kPORT_PullUp,
                                              /* Fast slew rate is configured */
                                              kPORT_FastSlewRate,
                                              /* Passive filter is disabled */
                                              kPORT_PassiveFilterDisable,
                                              /* Open drain is enabled */
                                              kPORT_OpenDrainEnable,
                                              /* Low drive strength is configured */
                                              kPORT_LowDriveStrength,
                                              /* Pin is configured as I2C0_SCL */
                                              kPORT_MuxAlt5,
                                              /* Pin Control Register fields [15:0] are not locked */
                                              kPORT_UnlockRegister};
    PORT_SetPinConfig(Port_I2C0a_SCL, Pin_I2C0a_SCL, &porte24_config);

    const port_pin_config_t porte25_config = {/* Internal pull-up resistor is enabled */
                                              kPORT_PullUp,
                                              /* Fast slew rate is configured */
                                              kPORT_FastSlewRate,
                                              /* Passive filter is disabled */
                                              kPORT_PassiveFilterDisable,
                                              /* Open drain is enabled */
                                              kPORT_OpenDrainEnable,
                                              /* Low drive strength is configured */
                                              kPORT_LowDriveStrength,
                                              /* Pin is configured as I2C0_SDA */
                                              kPORT_MuxAlt5,
                                              /* Pin Control Register fields [15:0] are not locked */
                                              kPORT_UnlockRegister};
    PORT_SetPinConfig(Port_I2C0a_SDA, Pin_I2C0a_SDA, &porte25_config);

    NVIC_SetPriority(I2C0_IRQn, 3);
    isI2cInUse = 0;

	I2C0Setup();
}

void I2C0bInit(void)
{
	// I2C0b to ALPU-MP-1413
    const port_pin_config_t portb2_config = {/* Internal pull-up resistor is enabled */
                                             kPORT_PullUp,
                                             /* Fast slew rate is configured */
                                             kPORT_FastSlewRate,
                                             /* Passive filter is disabled */
                                             kPORT_PassiveFilterDisable,
                                             /* Open drain is enabled */
                                             kPORT_OpenDrainEnable,
                                             /* Low drive strength is configured */
                                             kPORT_LowDriveStrength,
                                             /* Pin is configured as I2C0_SCL */
                                             kPORT_MuxAlt2,
                                             /* Pin Control Register fields [15:0] are not locked */
                                             kPORT_UnlockRegister};
    PORT_SetPinConfig(Port_I2C0b_SCL, Pin_I2C0b_SCL, &portb2_config);

    const port_pin_config_t portb3_config = {/* Internal pull-up resistor is enabled */
                                             kPORT_PullUp,
                                             /* Fast slew rate is configured */
                                             kPORT_FastSlewRate,
                                             /* Passive filter is disabled */
                                             kPORT_PassiveFilterDisable,
                                             /* Open drain is enabled */
                                             kPORT_OpenDrainEnable,
                                             /* Low drive strength is configured */
                                             kPORT_LowDriveStrength,
                                             /* Pin is configured as I2C0_SDA */
                                             kPORT_MuxAlt2,
                                             /* Pin Control Register fields [15:0] are not locked */
                                             kPORT_UnlockRegister};
    PORT_SetPinConfig(Port_I2C0b_SDA, Pin_I2C0b_SDA, &portb3_config);

    NVIC_SetPriority(I2C0_IRQn, 3);
}

static void I2C0Setup(void)
{
    i2c_master_config_t masterConfig;

	/*
	 * masterConfig.baudRate_Bps = 100000U;
	 * masterConfig.enableStopHold = false;
	 * masterConfig.glitchFilterWidth = 0U;
	 * masterConfig.enableMaster = true;
	 */
	I2C_MasterGetDefaultConfig(&masterConfig);
	masterConfig.baudRate_Bps = I2C_BAUDRATE;

	I2C_MasterInit(I2C0, &masterConfig, CLOCK_GetFreq(I2C0_CLK_SRC));
}

int I2CReadReg2byte(uint8_t addr, uint8_t reg, uint8_t *val1, uint8_t *val2)
{
    i2c_master_transfer_t masterXfer;
    status_t status;

    if (isI2cInUse)
    {
#if defined(USE_SEGGER_RTT)
    	SEGGER_RTT_printf(0, "Clash in read_I2C_reg_2byte (4) with %d\n",isI2cInUse);
#endif
    	return 0;
    }
    isI2cInUse = 4;

	i2c_master_buff[0] = reg;

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = addr;
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = i2c_master_buff;
    masterXfer.dataSize = 1;
    masterXfer.flags = kI2C_TransferDefaultFlag;

    status = I2C_MasterTransferBlocking(I2C0, &masterXfer);
    if (status != kStatus_Success)
    {
    	isI2cInUse = 0;
    	return status;
    }

    masterXfer.slaveAddress = AT1846S_I2C_MASTER_SLAVE_ADDR_7BIT;
    masterXfer.direction = kI2C_Read;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = i2c_master_buff;
    masterXfer.dataSize = 2;
    masterXfer.flags = kI2C_TransferDefaultFlag;

    status = I2C_MasterTransferBlocking(I2C0, &masterXfer);
    if (status != kStatus_Success)
    {
    	isI2cInUse = 0;
    	return status;
    }

    *val1 = i2c_master_buff[0];
    *val2 = i2c_master_buff[1];

    isI2cInUse = 0;
	return kStatus_Success;
}

int I2CWriteReg2byte(uint8_t addr, uint8_t reg, uint8_t val1, uint8_t val2)
{
    i2c_master_transfer_t masterXfer;
    status_t status;

    if (isI2cInUse)
    {
#if defined(USE_SEGGER_RTT)
    	SEGGER_RTT_printf(0, "Clash in write_I2C_reg_2byte (3) with %d\n",isI2cInUse);
#endif
    	return 0;
    }
    isI2cInUse = 3;

	i2c_master_buff[0] = reg;
	i2c_master_buff[1] = val1;
	i2c_master_buff[2] = val2;

    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = addr;
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = i2c_master_buff;
    masterXfer.dataSize = 3;
    masterXfer.flags = kI2C_TransferDefaultFlag;

    status = I2C_MasterTransferBlocking(I2C0, &masterXfer);
    if (status != kStatus_Success)
    {
    	isI2cInUse = 0;
    	return status;
    }

    isI2cInUse = 0;
	return kStatus_Success;
}
