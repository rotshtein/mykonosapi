/***************************************************************************//**
 *   @file   headless.c
 *   @brief  Implementation of Main Function.
********************************************************************************
 * Copyright 2017(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "common.h"
#include "ad9528.h"
#include "mykonos.h"
#include "Mykonos_M3.h"
#include "mykonos_gpio.h"
#include "platform_drivers.h"
#include "parameters.h"
#include "util.h"
#ifdef ALTERA_PLATFORM
#include "clk_altera_a10_fpll.h"
#include "altera_adxcvr.h"
#else
#include "clk_axi_clkgen.h"
#include "axi_adxcvr.h"
#endif
#include "axi_jesd204_rx.h"
#include "axi_jesd204_tx.h"
#include "axi_dac_core.h"
#include "axi_adc_core.h"
#include "axi_dmac.h"

#include "sin.h"
#include "dvbs2x.h"

#include <locale.h>
#include <string.h>
#include <unistd.h> 

#include "profile_wide_double.h"
#include "profile_narrow_double.h"
#include "change_frequency.h"


//for knhit
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
int kbhit(void);
/******************************************************************************/
/************************ Variables Definitions *******************************/
/******************************************************************************/
extern ad9528Device_t clockAD9528_;
extern ad9528Device_t clockAD9528_705wd;
extern ad9528Device_t clockAD9528_705nd;
extern mykonosDevice_t mykDevice;
#define VERSION "1.2"


void print_help()
{
	printf("\nmykonosapi version %s\n", VERSION);
	printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	printf("   -r <address> \n");
	printf("   -w <address:value> \n");
	printf("           * addresses and vlue can be in decimal or hex with leading 0x\n");
	printf("\n");
	printf("\n");
	printf("   -o transmir content: <0 - FF burst, 1 - LO, 2 = Sin>             (0)\n");
	printf("   -p <profile 0= narrow, 1=wide                                    (0)\n");
	printf("   -t <tx frequency in Hz>                                        (1e9)\n");
	printf("   -f <rx frequency in Hz>                                        (1e9)\n");
	printf("   -a <attenuation [milli db]>                                      (0)\n");
	printf("   -c <change attenuation [milli db]>                               (0)\n");
	printf("   -T <set new tx frequency in Hz>                                  (0)\n");
	printf("   -F <set new rx frequency in Hz>                                  (0)\n");
}


/***************************************************************************//**
 * @brief main
*******************************************************************************/
int main(int argc, char * argv[])
{
	unsigned int write_addr = 0xFFFFFFFF, read_addr = 0xFFFFFFFF;
	int value = 0;
	int profile = 0;
	uint64_t tx_frequency_Hz = 1e9;
	uint64_t rx_frequency_Hz = 1e9;
	uint64_t new_tx_frequency_Hz = 0;
	uint64_t new_rx_frequency_Hz = 0;
	int output_waveform = -1;
	char *output_type[3] = { "FF bursts", "LO", "Sin" };
	int attenuation = 0, change_attenuation = 50000;
    

	if (argc > 1)
	{
		
		int opt;


		// put ':' in the starting of the 
		// string so that program can  
		//distinguish between '?' and ':'  
		while ((opt = getopt(argc, argv, ":o:F:T:c:p:t:f:r:w:a:hH")) != -1)
		{
			extern char *optarg;
			extern int optopt;

			switch (opt)
			{
			case 'r':
				printf("Reading from address %s\n", optarg);
				read_addr = (uint32_t)optarg[1] == 'x' ? (unsigned int)strtoul(optarg, NULL, 16) : (unsigned int)strtoul(optarg,0,10);
				break;

			case 'w':
				{
					char * addr = strtok(optarg, ":");
					if (addr == NULL)
					{
						print_help();
						return 0;
					}
					
					write_addr = (uint32_t)addr[1] == 'x' ? (unsigned int)strtoul(addr, NULL, 16) : (unsigned int)strtoul(addr, 0, 10);;
					printf("Writing to address 0x%X\n", write_addr);
					char * val = strtok(NULL, ":"); //optarg;
					if (val == NULL)
					{
						print_help();
						return 0;
					}
					value = (uint32_t)val[1] == 'x' ? (unsigned int)strtoul(val, NULL, 16) : (unsigned int)strtoul(val, NULL, 10);
					printf("Value = 0x%X\n", value);
					break;
				}
			case 'c':
				change_attenuation = (uint32_t)(optarg[1] == 'x' ? (int)strtol(optarg, NULL, 16) : atoi(optarg));
				break;

			case 'o':
				output_waveform = (uint32_t)(optarg[1] == 'x' ? (int)strtol(optarg, NULL, 16) : atoi(optarg));
				break;

			case 'p':
				profile = atoi(optarg);
				/*if ((profile < 0) || (profile > 1))
				{
					profile = 0;
				}*/
				break;

			case 't':
				setlocale(LC_NUMERIC, "");
				tx_frequency_Hz = (uint64_t)(optarg[1] == 'x' ? (int)strtol(optarg, NULL, 16) : atoi(optarg));
				break;

			case 'T':
				setlocale(LC_NUMERIC, "");
				new_tx_frequency_Hz = (uint64_t)(optarg[1] == 'x' ? (int)strtol(optarg, NULL, 16) : atoi(optarg));
				break;

			case 'f':
				setlocale(LC_NUMERIC, "");
				rx_frequency_Hz = (uint64_t)(optarg[1] == 'x' ? (int)strtol(optarg, NULL, 16) : atoi(optarg));
				break;

			case 'F':
				setlocale(LC_NUMERIC, "");
				new_rx_frequency_Hz = (uint64_t)(optarg[1] == 'x' ? (int)strtol(optarg, NULL, 16) : atoi(optarg));
				break;

			case 'a':
				attenuation = (uint32_t)(optarg[1] == 'x' ? (int)strtol(optarg, NULL, 16) : atoi(optarg));
				
				break;

			case 'h':
			case 'H':
				print_help();
				return 0;
				break;

			case ':':
				printf("option needs a value\n");
				break;
			case '?':
				printf("unknown option : %c\n", optopt);
				break;
			}
		}

		
		if(write_addr != 0xFFFFFFFF)	//Write data to address
		{
			IOWR_32DIRECT(0, write_addr, value);
			printf("Value=0x%X ==> Address 0x%X\n", value, write_addr);
		}

		if (read_addr != 0xFFFFFFFF)	// Read address
		{
			uint32_t reg_val = IORD_32DIRECT(0, read_addr);
			printf("Address 0x%X = Value=0x%X\n", read_addr, reg_val);
		}

		if ((write_addr != 0xFFFFFFFF) | (read_addr != 0xFFFFFFFF))
		{
			return 0;
		}
	}

	ad9528Device_t *clockAD9528_device = &clockAD9528_;
#if(1)
	printf("***************************\n");
	switch (profile)
	{
	
	case 0:
		memcpy(&mykDevice, &mykDevice_705nd, sizeof(mykDevice));
		clockAD9528_device = &clockAD9528_705nd; // clockAD9528_wide_0705;
		printf("       Profile: Wide Double 245.76 Tx 61.44 Rx\n");
		break;
	case 2:
		memcpy(&mykDevice, &mykDevice_705wd, sizeof(mykDevice));
		clockAD9528_device = &clockAD9528_705wd; // clockAD9528_wide_0705 double;
		printf("      Profile: Wide Double 245.76 Tx 122.88 Rx  \n");
		break;
	default:
		printf("      Profile: default (myc.c)\n");
		break;
	}
	/*
	
	uint8_t mykError1;
	printf("Close radio");
	if ((mykError1 = MYKONOS_radioOff(&mykDevice)) != MYKONOS_ERR_OK)
	{
		//errorString = getMykonosErrorMessage(mykError);
		printf("%s", getMykonosErrorMessage(mykError1));
	}
	return 0;
	*/
	printf("***************************\n");
	printf("receive profile number: \t%d\n", profile);
#endif
	int ret_platfor = platform_init();
	if (ret_platfor != SUCCESS)
	{
		printf("error: platform_init() failed\n");
	}
	
	//set transmit content
	if (output_waveform != (-1))
	{
		printf("Setting output content to: %s\n\n", output_type[output_waveform]);
		IOWR_32DIRECT(0, 0x2F010, output_waveform);
	}

	if ((new_rx_frequency_Hz > 0) || (new_tx_frequency_Hz > 0))
	{
		printf("Changing frequncy:\n");
		if (new_rx_frequency_Hz > 0)
			printf("\tRx\t%llu => %llu\n", rx_frequency_Hz, new_rx_frequency_Hz);
		if (new_tx_frequency_Hz > 0)
			printf("\tTx\t%llu => %llu\n", tx_frequency_Hz, new_tx_frequency_Hz);
		int ret = change_frequency(&mykDevice, new_rx_frequency_Hz, new_tx_frequency_Hz);
		uint64_t freq = 0;

		MYKONOS_getRfPllFrequency(&mykDevice, RX_PLL, &freq);
		printf("\t\tRx frequency is: %llu\n", freq);

		MYKONOS_getRfPllFrequency(&mykDevice, TX_PLL, &freq);
		printf("\t\tTx frequency is: %llu\n", freq);


		if (ret != MYKONOS_ERR_OK)
			printf("Error");
		return(0);
	}



	if (change_attenuation < 50000)
	{
		uint64_t rfPllLoFrequency_Hz;
		MYKONOS_getRfPllFrequency(&mykDevice, RX_PLL, &rfPllLoFrequency_Hz);
		printf("Current Rx frequency %lld [Hz]\n", rfPllLoFrequency_Hz);

		unsigned short current_attenuation = 0xFFFF;
		MYKONOS_getTx1Attenuation(&mykDevice, &current_attenuation);
		printf("Current attenuation is %d [mili DB]\n", current_attenuation);

		current_attenuation = (unsigned short)change_attenuation;
		printf("Setting attenuation to %d [mili DB]\n", current_attenuation);
		MYKONOS_setTx1Attenuation(&mykDevice, current_attenuation);
			
		current_attenuation = 0xFFFF;
		MYKONOS_getTx1Attenuation(&mykDevice, &current_attenuation);
		printf("New attenuation is %d [mili DB]\n", current_attenuation);
		return(0);
	}

	// Set the Tx Frequency
	mykDevice.tx->txPllUseExternalLo = 0; // Use internal LO
	mykDevice.tx->txPllLoFrequency_Hz = tx_frequency_Hz;
	printf("transmit: \t\t\t%llu [Hz]\n", mykDevice.tx->txPllLoFrequency_Hz);

	// Set the Rx Frequency
	mykDevice.rx->rxPllUseExternalLo = 0; // Use internal LO
	mykDevice.rx->rxPllLoFrequency_Hz = rx_frequency_Hz;
	printf("receive: \t\t\t%llu [Hz]\n", mykDevice.rx->rxPllLoFrequency_Hz);

	

	ADI_ERR error;
	
	mykonosErr_t mykError;
	const char *errorString;
	uint8_t pllLockStatus;
	uint8_t mcsStatus;
	uint8_t arm_major;
	uint8_t arm_minor;
	uint8_t arm_release;
	mykonosGpioErr_t mykGpioErr;
	uint32_t initCalMask = TX_BB_FILTER | ADC_TUNER | TIA_3DB_CORNER | DC_OFFSET |
		TX_ATTENUATION_DELAY | RX_GAIN_DELAY | FLASH_CAL |
		PATH_DELAY | TX_LO_LEAKAGE_INTERNAL | TX_QEC_INIT |
		LOOPBACK_RX_LO_DELAY | LOOPBACK_RX_RX_QEC_INIT |
		RX_LO_DELAY | RX_QEC_INIT;
	uint8_t errorFlag = 0;
	uint8_t errorCode = 0;
	uint32_t initCalsCompleted;
	uint8_t framerStatus;
	uint8_t obsFramerStatus;
	uint8_t deframerStatus;
	uint32_t trackingCalMask = TRACK_ORX1_QEC | TRACK_ORX2_QEC | TRACK_RX1_QEC |
		TRACK_RX2_QEC | TRACK_TX1_QEC | TRACK_TX2_QEC;
	uint32_t status;
	int32_t ret;
#ifdef ALTERA_PLATFORM
	struct altera_a10_fpll_init rx_device_clk_pll_init = {
		"rx_device_clk_pll",
		RX_A10_FPLL_BASEADDR,
		clockAD9528_device->outputSettings->outFrequency_Hz[1]
	};
	struct altera_a10_fpll_init tx_device_clk_pll_init = {
		"tx_device_clk_pll",
		TX_A10_FPLL_BASEADDR,
		clockAD9528_device->outputSettings->outFrequency_Hz[1]
	};
	struct altera_a10_fpll_init rx_os_device_clk_pll_init = {
		"rx_os_device_clk_pll",
		RX_OS_A10_FPLL_BASEADDR,
		clockAD9528_device->outputSettings->outFrequency_Hz[1]
	};
	struct altera_a10_fpll *rx_device_clk_pll;
	struct altera_a10_fpll *tx_device_clk_pll;
	struct altera_a10_fpll *rx_os_device_clk_pll;
#else
	struct axi_clkgen_init rx_clkgen_init = {
		"rx_clkgen",
		RX_CLKGEN_BASEADDR,
		clockAD9528_device->outputSettings->outFrequency_Hz[1]
	};
	struct axi_clkgen_init tx_clkgen_init = {
		"tx_clkgen",
		TX_CLKGEN_BASEADDR,
		clockAD9528_device->outputSettings->outFrequency_Hz[1]
	};
	struct axi_clkgen_init rx_os_clkgen_init = {
		"rx_os_clkgen",
		RX_OS_CLKGEN_BASEADDR,
		clockAD9528_device->outputSettings->outFrequency_Hz[1]
	};
	struct axi_clkgen *rx_clkgen;
	struct axi_clkgen *tx_clkgen;
	struct axi_clkgen *rx_os_clkgen;
#endif
	uint32_t rx_lane_rate_khz = mykDevice.rx->rxProfile->iqRate_kHz *
		mykDevice.rx->framer->M * (20 /
			hweight8(mykDevice.rx->framer->serializerLanesEnabled));
	uint32_t rx_div40_rate_hz = rx_lane_rate_khz * (1000 / 40);
	uint32_t tx_lane_rate_khz = mykDevice.tx->txProfile->iqRate_kHz *
		mykDevice.tx->deframer->M * (20 /
			hweight8(mykDevice.tx->deframer->deserializerLanesEnabled));
	uint32_t tx_div40_rate_hz = tx_lane_rate_khz * (1000 / 40);
	uint32_t rx_os_lane_rate_khz = mykDevice.obsRx->orxProfile->iqRate_kHz *
		mykDevice.obsRx->framer->M * (20 /
			hweight8(mykDevice.obsRx->framer->serializerLanesEnabled));
	uint32_t rx_os_div40_rate_hz = rx_os_lane_rate_khz * (1000 / 40);
	struct jesd204_rx_init rx_jesd_init = {
		"rx_jesd",
		RX_JESD_BASEADDR,
		4,
		32,
		1,
		rx_div40_rate_hz / 1000,
		rx_lane_rate_khz,
	};
	struct jesd204_tx_init tx_jesd_init = {
		"tx_jesd",
		TX_JESD_BASEADDR,
		2,
		32,
		4,
		14,
		16,
		false,
		2,
		1,
		tx_div40_rate_hz / 1000,
		tx_lane_rate_khz,
	};

	struct jesd204_rx_init rx_os_jesd_init = {
		"rx_os_jesd",
		RX_OS_JESD_BASEADDR,
		2,
		32,
		1,
		rx_os_div40_rate_hz / 1000,
		rx_os_lane_rate_khz,
	};
	struct axi_jesd204_rx *rx_jesd;
	struct axi_jesd204_tx *tx_jesd;
	struct axi_jesd204_rx *rx_os_jesd;
#ifdef ALTERA_PLATFORM
	struct adxcvr_init rx_adxcvr_init = {
		"rx_adxcvr",
		RX_XCVR_BASEADDR,
		{RX_ADXCFG_0_BASEADDR, RX_ADXCFG_1_BASEADDR, 0, 0},
		0,
		rx_lane_rate_khz,
		mykDevice.clocks->deviceClock_kHz,
	};
	struct adxcvr_init tx_adxcvr_init = {
		"tx_adxcvr",
		TX_XCVR_BASEADDR,
		{TX_ADXCFG_0_BASEADDR, TX_ADXCFG_1_BASEADDR, TX_ADXCFG_2_BASEADDR, TX_ADXCFG_3_BASEADDR},
		TX_PLL_BASEADDR,
		tx_lane_rate_khz,
		mykDevice.clocks->deviceClock_kHz,
	};
	struct adxcvr_init rx_os_adxcvr_init = {
		"rx_os_adxcvr",
		RX_OS_XCVR_BASEADDR,
		{RX_OS_ADXCFG_0_BASEADDR, RX_OS_ADXCFG_1_BASEADDR, 0, 0},
		0,
		rx_os_lane_rate_khz,
		mykDevice.clocks->deviceClock_kHz,
	};
#else
	struct adxcvr_init rx_adxcvr_init = {
		"rx_adxcvr",
		RX_XCVR_BASEADDR,
		0,
		3,
		1,
		1,
		rx_lane_rate_khz,
		mykDevice.clocks->deviceClock_kHz,
	};
	struct adxcvr_init tx_adxcvr_init = {
		"tx_adxcvr",
		TX_XCVR_BASEADDR,
		3,
		3,
		0,
		0,
		tx_lane_rate_khz,
		mykDevice.clocks->deviceClock_kHz,
	};
	struct adxcvr_init rx_os_adxcvr_init = {
		"rx_os_adxcvr",
		RX_OS_XCVR_BASEADDR,
		0,
		3,
		1,
		1,
		rx_os_lane_rate_khz,
		mykDevice.clocks->deviceClock_kHz,
	};
#endif
#if (RX_CORE_BASEADDR != 0)
	struct adxcvr *rx_adxcvr;
	struct adxcvr *tx_adxcvr;
	struct adxcvr *rx_os_adxcvr;

	struct axi_dac_init tx_dac_init = {
		"tx_dac",
		TX_CORE_BASEADDR,
		4,
	};
	struct axi_dac *tx_dac;
	printf("OOOOOOOOOOOOOOOOOpsssssssss............RX_CORE_BASEADDR, = %d\n", RX_CORE_BASEADDR);
	struct axi_adc_init rx_adc_init = {
		"rx_adc",
		RX_CORE_BASEADDR,
		4,
	};
	struct axi_adc *rx_adc;
	printf("OOOOOOOOOOOOOOOOOpsssssssss............RX_DMA_BASEADDR, = %d\n", RX_DMA_BASEADDR);
	struct axi_dmac_init rx_dmac_init = {
		"rx_dmac",
		RX_DMA_BASEADDR,
		DMA_DEV_TO_MEM,
		0,
	};
	struct axi_dmac *rx_dmac;
	uint32_t i;

	/*mykonosTempSensorStatus_t tempStatus;
	mykonosGpioErr_t e = MYKONOS_readTempSensor(&mykDevice, &tempStatus);
	printf("Temp is: %d", tempStatus.tempCode);
	*/
	/* Allocating memory for the errorString */
	errorString = NULL;
#endif
	printf("Please wait...\n");
	/*
	ret = platform_init();
	if (ret != SUCCESS) {
		printf("error: platform_init() failed\n");
		goto error_0;
	}
	*/
	/**************************************************************************/
	/*****      System Clocks Initialization Initialization Sequence      *****/
	/**************************************************************************/

	/* Perform a hard reset on the AD9528 DUT */
	error = AD9528_resetDevice(clockAD9528_device);
	if (error != ADIERR_OK) {
		printf("AD9528_resetDevice() failed\n");
		error = ADIERR_FAILED;
		goto error_1;
	}

	error = AD9528_initDeviceDataStruct(clockAD9528_device,
		clockAD9528_device->pll1Settings->vcxo_Frequency_Hz,
		clockAD9528_device->pll1Settings->refA_Frequency_Hz,
		clockAD9528_device->outputSettings->outFrequency_Hz[1]);
	if (error != ADIERR_OK) {
		printf("AD9528_initDeviceDataStruct() failed\n");
		error = ADIERR_FAILED;
		goto error_1;
	}

	/* Initialize the AD9528 by writing all SPI registers */
	error = AD9528_initialize(clockAD9528_device);
	if (error != ADIERR_OK)
		printf("WARNING: AD9528_initialize() issues. Possible cause: REF_CLK not connected.\n");

#ifdef ALTERA_PLATFORM
	/* Initialize A10 FPLLs */
	status = altera_a10_fpll_init(&rx_device_clk_pll,
		&rx_device_clk_pll_init);
	if (status != SUCCESS) {
		printf("error: %s: altera_a10_fpll_init() failed\n",
			rx_os_device_clk_pll_init.name);
		goto error_1;
	}
	status = altera_a10_fpll_init(&tx_device_clk_pll,
		&tx_device_clk_pll_init);
	if (status != SUCCESS) {
		printf("error: %s: altera_a10_fpll_init() failed\n",
			rx_os_device_clk_pll_init.name);
		goto error_2;
	}
	status = altera_a10_fpll_init(&rx_os_device_clk_pll,
		&rx_os_device_clk_pll_init);
	if (status != SUCCESS) {
		printf("error: %s: altera_a10_fpll_init() failed\n",
			rx_os_device_clk_pll_init.name);
		goto error_3;
	}

	altera_a10_fpll_disable(rx_device_clk_pll);
	status = altera_a10_fpll_set_rate(rx_device_clk_pll,
		rx_div40_rate_hz);
	if (status != SUCCESS) {
		printf("error: %s: altera_a10_fpll_set_rate() failed\n",
			rx_device_clk_pll->name);
		goto error_4;
	}
	altera_a10_fpll_enable(rx_device_clk_pll);
	altera_a10_fpll_disable(tx_device_clk_pll);
	status = altera_a10_fpll_set_rate(tx_device_clk_pll,
		tx_div40_rate_hz);
	if (status != SUCCESS) {
		printf("error: %s: altera_a10_fpll_set_rate() failed\n",
			tx_device_clk_pll->name);
		goto error_4;
	}
	altera_a10_fpll_enable(tx_device_clk_pll);
	altera_a10_fpll_disable(rx_os_device_clk_pll);
	status = altera_a10_fpll_set_rate(rx_os_device_clk_pll,
		rx_os_div40_rate_hz);
	if (status != SUCCESS) {
		printf("error: %s: altera_a10_fpll_set_rate() failed\n",
			rx_os_device_clk_pll->name);
		goto error_4;
	}
	altera_a10_fpll_enable(rx_os_device_clk_pll);
#else
	/* Initialize CLKGEN */
	status = axi_clkgen_init(&rx_clkgen, &rx_clkgen_init);
	if (status != SUCCESS) {
		printf("error: %s: axi_clkgen_init() failed\n", rx_clkgen_init.name);
		goto error_1;
	}
	status = axi_clkgen_init(&tx_clkgen, &tx_clkgen_init);
	if (status != SUCCESS) {
		printf("error: %s: axi_clkgen_init() failed\n", tx_clkgen_init.name);
		goto error_2;
	}
	status = axi_clkgen_init(&rx_os_clkgen, &rx_os_clkgen_init);
	if (status != SUCCESS) {
		printf("error: %s: axi_clkgen_set_rate() failed\n", rx_os_clkgen_init.name);
		goto error_3;
	}

	status = axi_clkgen_set_rate(rx_clkgen, rx_div40_rate_hz);
	if (status != SUCCESS) {
		printf("error: %s: axi_clkgen_set_rate() failed\n", rx_clkgen->name);
		goto error_4;
	}
	status = axi_clkgen_set_rate(tx_clkgen, tx_div40_rate_hz);
	if (status != SUCCESS) {
		printf("error: %s: axi_clkgen_set_rate() failed\n", tx_clkgen->name);
		goto error_4;
	}
	status = axi_clkgen_set_rate(rx_os_clkgen, rx_os_div40_rate_hz);
	if (status != SUCCESS) {
		printf("error: %s: axi_clkgen_set_rate() failed\n", rx_os_clkgen->name);
		goto error_4;
	}
#endif

	

	/* Initialize JESDs */
	status = axi_jesd204_rx_init(&rx_jesd, &rx_jesd_init);
	
	

	if (status != SUCCESS) {
		printf("error: %s: axi_jesd204_rx_init() failed\n", rx_jesd_init.name);
		goto error_4;
	}


	status = axi_jesd204_tx_init(&tx_jesd, &tx_jesd_init);
	if (status != SUCCESS) {
		printf("error: %s: axi_jesd204_rx_init() failed\n", rx_jesd_init.name);
		goto error_5;
	}
	status = axi_jesd204_rx_init(&rx_os_jesd, &rx_os_jesd_init);
	if (status != SUCCESS) {
		printf("error: %s: axi_jesd204_rx_init() failed\n", rx_jesd_init.name);
		goto error_6;
	}

	/* Initialize ADXCVRs */
#if (RX_CORE_BASEADDR != 0) //$$$URI
	status = adxcvr_init(&rx_adxcvr, &rx_adxcvr_init);
	if (status != SUCCESS) {
		printf("error: %s: adxcvr_init() failed\n", rx_adxcvr_init.name);
		goto error_7;
	}
	status = adxcvr_init(&tx_adxcvr, &tx_adxcvr_init);
	if (status != SUCCESS) {
		printf("error: %s: adxcvr_init() failed\n", tx_adxcvr_init.name);
		goto error_8;
	}
	status = adxcvr_init(&rx_os_adxcvr, &rx_os_adxcvr_init);
	if (status != SUCCESS) {
		printf("error: %s: adxcvr_init() failed\n", rx_os_adxcvr_init.name);
		goto error_9;
	}
#endif
	/*************************************************************************/
	/*****                Mykonos Initialization Sequence                *****/
	/*************************************************************************/

	/* Perform a hard reset on the MYKONOS DUT (Toggle RESETB pin on device) */
	if ((mykError = MYKONOS_resetDevice(&mykDevice)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	if ((mykError = MYKONOS_initialize(&mykDevice)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	/*************************************************************************/
	/*****                Mykonos CLKPLL Status Check                    *****/
	/*************************************************************************/

	if ((mykError = MYKONOS_checkPllsLockStatus(&mykDevice,
		&pllLockStatus)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	/*************************************************************************/
	/*****                Mykonos Perform MultiChip Sync                 *****/
	/*************************************************************************/

	if ((mykError = MYKONOS_enableMultichipSync(&mykDevice, 1,
		&mcsStatus)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	/* Minimum 3 SYSREF pulses from Clock Device has to be produced for MulticChip Sync */

	AD9528_requestSysref(clockAD9528_device, 1);
	mdelay(1);
	AD9528_requestSysref(clockAD9528_device, 1);
	mdelay(1);
	AD9528_requestSysref(clockAD9528_device, 1);
	mdelay(1);
	AD9528_requestSysref(clockAD9528_device, 1);
	mdelay(1);

	/*************************************************************************/
	/*****                Mykonos Verify MultiChip Sync                 *****/
	/*************************************************************************/

	if ((mykError = MYKONOS_enableMultichipSync(&mykDevice, 0,
		&mcsStatus)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	if ((mcsStatus & 0x0B) == 0x0B)
		printf("MCS successful\n");
	else
		printf("MCS failed\n");

	/*************************************************************************/
	/*****                Mykonos Load ARM file                          *****/
	/*************************************************************************/

	if (pllLockStatus & 0x01) {
		printf("CLKPLL locked\n");
		if ((mykError = MYKONOS_initArm(&mykDevice)) != MYKONOS_ERR_OK) {
			errorString = getMykonosErrorMessage(mykError);
			goto error_11;
		}

		if ((mykError = MYKONOS_loadArmFromBinary(&mykDevice,
			&firmware_Mykonos_M3_bin[0], firmware_Mykonos_M3_bin_len)) != MYKONOS_ERR_OK) {
			errorString = getMykonosErrorMessage(mykError);
			goto error_11;
		}
	}
	else {
		printf("CLKPLL not locked (0x%x)\n", pllLockStatus);
		error = ADIERR_FAILED;
		goto error_2;
	}

	/* Read back the version of the ARM binary loaded into the Mykonos ARM memory */
	if ((mykError = MYKONOS_getArmVersion(&mykDevice, &arm_major, &arm_minor,
		&arm_release, NULL)) == MYKONOS_ERR_OK)
		printf("AD9371 ARM version %d.%d.%d\n", arm_major, arm_minor, arm_release);

	/*************************************************************************/
	/*****                Mykonos Set RF PLL Frequencies                 *****/
	/*************************************************************************/

	//mykDevice.rx->rxPllLoFrequency_Hz = 1234000000; //$$$IGAL

	if ((mykError = MYKONOS_setRfPllFrequency(&mykDevice, RX_PLL,
		mykDevice.rx->rxPllLoFrequency_Hz)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	if ((mykError = MYKONOS_setRfPllFrequency(&mykDevice, TX_PLL,
		mykDevice.tx->txPllLoFrequency_Hz)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	if ((mykError = MYKONOS_setRfPllFrequency(&mykDevice, SNIFFER_PLL,
		mykDevice.obsRx->snifferPllLoFrequency_Hz)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	/* Wait 200ms for PLLs to lock */
	mdelay(200);

	if ((mykError = MYKONOS_checkPllsLockStatus(&mykDevice,
		&pllLockStatus)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	if ((pllLockStatus & 0x0F) == 0x0F)
		printf("PLLs locked\n");
	else {
		printf("PLLs not locked (0x%x)\n", pllLockStatus);
		error = ADIERR_FAILED;
		goto error_2;
	}

	/*************************************************************************/
	/*****                Mykonos Set GPIOs                              *****/
	/*************************************************************************/
#if(0) //$$$URI
	if ((mykGpioErr = MYKONOS_setRx1GainCtrlPin(&mykDevice, 0, 0, 0, 0,
		0)) != MYKONOS_ERR_GPIO_OK) {
		errorString = getGpioMykonosErrorMessage(mykGpioErr);
		goto error_11;
	}

	if ((mykGpioErr = MYKONOS_setRx2GainCtrlPin(&mykDevice, 0, 0, 0, 0,
		0)) != MYKONOS_ERR_GPIO_OK) {
		errorString = getGpioMykonosErrorMessage(mykGpioErr);
		goto error_11;
	}

	if ((mykGpioErr = MYKONOS_setTx1AttenCtrlPin(&mykDevice, 0, 0, 0, 0,
		0)) != MYKONOS_ERR_GPIO_OK) {
		errorString = getGpioMykonosErrorMessage(mykGpioErr);
		goto error_11;
	}

	if ((mykGpioErr = MYKONOS_setTx2AttenCtrlPin(&mykDevice, 0, 0, 0,
		0)) != MYKONOS_ERR_GPIO_OK) {
		errorString = getGpioMykonosErrorMessage(mykGpioErr);
		goto error_11;
	}
#endif
	if ((mykGpioErr = MYKONOS_setupGpio(&mykDevice)) != MYKONOS_ERR_GPIO_OK) {
		errorString = getGpioMykonosErrorMessage(mykGpioErr);
		goto error_11;
	}

	/*************************************************************************/
	/*****                Mykonos Set manual gains values                *****/
	/*************************************************************************/

	if ((mykError = MYKONOS_setRx1ManualGain(&mykDevice, 255)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	if ((mykError = MYKONOS_setRx2ManualGain(&mykDevice, 255)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	if ((mykError = MYKONOS_setObsRxManualGain(&mykDevice, OBS_RX1_TXLO,
		255)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	if ((mykError = MYKONOS_setObsRxManualGain(&mykDevice, OBS_RX2_TXLO,
		255)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	if ((mykError = MYKONOS_setObsRxManualGain(&mykDevice, OBS_SNIFFER_A,
		255)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	if ((mykError = MYKONOS_setObsRxManualGain(&mykDevice, OBS_SNIFFER_B,
		255)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	if ((mykError = MYKONOS_setObsRxManualGain(&mykDevice, OBS_SNIFFER_C,
		255)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	/*************************************************************************/
	/*****                Mykonos Initialize attenuations                *****/
	/*************************************************************************/

	if ((mykError = MYKONOS_setTx1Attenuation(&mykDevice, 0)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	if ((mykError = MYKONOS_setTx2Attenuation(&mykDevice, 0)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	/*************************************************************************/
	/*****           Mykonos ARM Initialization Calibrations             *****/
	/*************************************************************************/

	if ((mykError = MYKONOS_runInitCals(&mykDevice,
		(initCalMask & ~TX_LO_LEAKAGE_EXTERNAL))) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	if ((mykError = MYKONOS_waitInitCals(&mykDevice, 60000, &errorFlag,
		&errorCode)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	if ((errorFlag != 0) || (errorCode != 0)) {
		/*** < Info: abort init cals > ***/
		if ((mykError = MYKONOS_abortInitCals(&mykDevice,
			&initCalsCompleted)) != MYKONOS_ERR_OK) {
			errorString = getMykonosErrorMessage(mykError);
			goto error_11;
		}
		if (initCalsCompleted)
			printf("Completed calibrations: %x\n", (unsigned int)initCalsCompleted);
	}
	else
		printf("Calibrations completed successfully\n");

	/*************************************************************************/
	/*****  Mykonos ARM Initialization External LOL Calibrations with PA *****/
	/*************************************************************************/

	/* Please ensure PA is enabled operational at this time */
	if (initCalMask & TX_LO_LEAKAGE_EXTERNAL) {
		if ((mykError = MYKONOS_runInitCals(&mykDevice,
			TX_LO_LEAKAGE_EXTERNAL)) != MYKONOS_ERR_OK) {
			errorString = getMykonosErrorMessage(mykError);
			goto error_11;
		}

		if ((mykError = MYKONOS_waitInitCals(&mykDevice, 60000, &errorFlag,
			&errorCode)) != MYKONOS_ERR_OK) {
			errorString = getMykonosErrorMessage(mykError);
			goto error_11;
		}

		if ((errorFlag != 0) || (errorCode != 0)) {
			/*** < Info: abort init cals > ***/
			if ((mykError = MYKONOS_abortInitCals(&mykDevice,
				&initCalsCompleted)) != MYKONOS_ERR_OK) {
				errorString = getMykonosErrorMessage(mykError);
				goto error_11;
			}
		}
		else
			printf("External LOL Calibrations completed successfully\n");
	}

	/*************************************************************************/
	/*****             SYSTEM JESD bring up procedure                    *****/
	/*************************************************************************/

	if ((mykError = MYKONOS_enableSysrefToRxFramer(&mykDevice,
		1)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}
	/*** < Info: Mykonos is waiting for sysref in order to start
	 * transmitting CGS from the RxFramer> ***/

	if ((mykError = MYKONOS_enableSysrefToObsRxFramer(&mykDevice,
		1)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}
	/*** < Info: Mykonos is waiting for sysref in order to start
	 * transmitting CGS from the ObsRxFramer> ***/

	if ((mykError = MYKONOS_enableSysrefToDeframer(&mykDevice,
		0)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	if ((mykError = MYKONOS_resetDeframer(&mykDevice)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

#ifndef ALTERA_PLATFORM
	status = adxcvr_clk_enable(tx_adxcvr);
	if (status != SUCCESS) {
		printf("error: %s: adxcvr_clk_enable() failed\n", tx_adxcvr->name);
		goto error_10;
	}
#endif
	axi_jesd204_tx_lane_clk_enable(tx_jesd);

	if ((mykError = MYKONOS_enableSysrefToDeframer(&mykDevice,
		1)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	/*************************************************************************/
	/*****            Enable SYSREF to Mykonos and BBIC                  *****/
	/*************************************************************************/

	/* Request a SYSREF from the AD9528 */
	AD9528_requestSysref(clockAD9528_device, 1);
	mdelay(1);

	/*** < Info: Mykonos is actively transmitting CGS from the RxFramer> ***/

	/*** < Info: Mykonos is actively transmitting CGS from the ObsRxFramer> ***/

#ifndef ALTERA_PLATFORM
	status = adxcvr_clk_enable(rx_adxcvr);
	if (status != SUCCESS) {
		printf("error: %s: adxcvr_clk_enable() failed\n", rx_adxcvr->name);
		goto error_10;
	}
#endif
	axi_jesd204_rx_lane_clk_enable(rx_jesd);
#ifndef ALTERA_PLATFORM
	status = adxcvr_clk_enable(rx_os_adxcvr);
	if (status != SUCCESS) {
		printf("error: %s: adxcvr_clk_enable() failed\n", rx_os_adxcvr->name);
		goto error_10;
	}
#endif
	axi_jesd204_rx_lane_clk_enable(rx_os_jesd);

	/* Request two SYSREFs from the AD9528 */
	AD9528_requestSysref(clockAD9528_device, 1);
	mdelay(1);
	AD9528_requestSysref(clockAD9528_device, 1);
	mdelay(5);

	/*************************************************************************/
	/*****               Check Mykonos Framer Status                     *****/
	/*************************************************************************/

	if ((mykError = MYKONOS_readRxFramerStatus(&mykDevice,
		&framerStatus)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}
	else if (framerStatus != 0x3E)
		printf("RxFramerStatus = 0x%x\n", framerStatus);

	if ((mykError = MYKONOS_readOrxFramerStatus(&mykDevice,
		&obsFramerStatus)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}
	else if (obsFramerStatus != 0x3E)
		printf("OrxFramerStatus = 0x%x\n", obsFramerStatus);

	/*************************************************************************/
	/*****               Check Mykonos Deframer Status                   *****/
	/*************************************************************************/

	if ((mykError = MYKONOS_readDeframerStatus(&mykDevice,
		&deframerStatus)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}
	else if (deframerStatus != 0x28)
		printf("DeframerStatus = 0x%x\n", deframerStatus);

	/*************************************************************************/
	/*****           Mykonos enable tracking calibrations                *****/
	/*************************************************************************/

	if ((mykError = MYKONOS_enableTrackingCals(&mykDevice,
		trackingCalMask)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	/*** < Info: Allow Rx1/2 QEC tracking and Tx1/2 QEC tracking to run when in the radioOn state
		 *  Tx calibrations will only run if radioOn and the obsRx path is set to OBS_INTERNAL_CALS > ***/

		 /*** < Info: Function to turn radio on, Enables transmitters and receivers
		  * that were setup during MYKONOS_initialize() > ***/
	if ((mykError = MYKONOS_radioOn(&mykDevice)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	/*** < Info: Allow TxQEC to run when User: is not actively using ORx receive path > ***/
	if ((mykError = MYKONOS_setObsRxPathSource(&mykDevice,
		OBS_RXOFF)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}
	if ((mykError = MYKONOS_setObsRxPathSource(&mykDevice,
		OBS_INTERNALCALS)) != MYKONOS_ERR_OK) {
		errorString = getMykonosErrorMessage(mykError);
		goto error_11;
	}

	axi_jesd204_rx_watchdog(rx_jesd);
	axi_jesd204_rx_watchdog(rx_os_jesd);
	mdelay(1000);

	/* Print JESD status */
	axi_jesd204_rx_status_read(rx_jesd);
	for (int i = 0; i < rx_jesd->num_lanes; i++)
		axi_jesd204_rx_laneinfo_read(rx_jesd, i);
	axi_jesd204_tx_status_read(tx_jesd);
	axi_jesd204_rx_status_read(rx_os_jesd);
	for (int i = 0; i < rx_os_jesd->num_lanes; i++)
		axi_jesd204_rx_laneinfo_read(rx_os_jesd, i);

	/* Initialize the DAC DDS */
	axi_dac_init(&tx_dac, &tx_dac_init);

	//axi_dac_set_datasel(tx_dac, -1, AXI_DAC_DATA_SEL_DMA); // ALL_CHANNELS
	axi_dac_set_datasel(tx_dac, 0, AXI_DAC_DATA_SEL_DMA);
	axi_dac_set_datasel(tx_dac, 1, AXI_DAC_DATA_SEL_DMA);

	/* Initialize the ADC core */
	axi_adc_init(&rx_adc, &rx_adc_init);

	mdelay(1000);
	

	/* Initialize the DMAC and transfer 16384 samples from ADC to MEM */
#if (AXI_AD9371_BASE != 0) //$$$URI
	axi_dmac_init(&rx_dmac, &rx_dmac_init);
#endif

#if (DDR_MEM_BASEADDR != 0)
	axi_dmac_transfer(rx_dmac,
		DDR_MEM_BASEADDR + 0x800000,
		16384 * 8);
#else
	printf("Skipping DMA initialaztion (no ddr on board)\n");
#endif
	/*************************************************************************/
	/*****           Sorin - check attenuation                           *****/
	/*************************************************************************/
	// Set the Tx Attenuation
	//mykDevice.tx->tx1Atten_mdB = attenuation;
	printf("Transmit attenuation: \t\t%d [dbM]\n", attenuation);



	uint16_t tx1Attenuation_mdB = attenuation;
	MYKONOS_setTx1Attenuation(&mykDevice, attenuation);
	MYKONOS_setTx2Attenuation(&mykDevice, 49500);


	printf("\nSorin [press enter to end test]\n");
	printf("\tCheck attenuation\n");
	tx1Attenuation_mdB = -100;
	
	MYKONOS_getTx1Attenuation(&mykDevice, &tx1Attenuation_mdB);
	printf("\t\tGot Attenuation = %hu\n", tx1Attenuation_mdB);
	/*************************************************************************/
	/*****           Sorin - protect DAC                                 *****/
	/*************************************************************************/
	printf("\tSet DAC protection\n");
	uint16_t	powerThreshold = 4095;
	uint8_t		attenStepSize = 0;
	uint8_t		avgDuration = 255;
	uint8_t		stickyFlagEnable = 0;
	uint8_t		txAttenControlEnable = 1;

	mykonosErr_t myError = MYKONOS_setupPaProtection(&mykDevice, powerThreshold, attenStepSize, avgDuration, stickyFlagEnable, txAttenControlEnable);
	
	printf("\tCheck Status after configuration:\n");
	uint8_t sframerStatus;
	MYKONOS_readRxFramerStatus(&mykDevice, &sframerStatus);
	printf("\t\tRx Framer Statu = %u\n", sframerStatus);

	uint8_t sdefframerStatus;
	MYKONOS_readDeframerStatus(&mykDevice, &sdefframerStatus);
	printf("\t\tDeframer Statu = %u\n", sdefframerStatus);

	uint8_t smismatch;
	MYKONOS_jesd204bIlasCheck(&mykDevice, &smismatch);
	printf("\t\tjesd204bIlasCheck = %u\n", smismatch);


	printf("\tContinues power meter\n");
	
	MYKONOS_enablePaProtection(&mykDevice, 1);
		
	if (mykError = MYKONOS_setRxGainControlMode(&mykDevice, AGC) != MYKONOS_ERR_OK)
	{
		printf("Error setting Rx AGC to auto: %s",getMykonosErrorMessage(mykError));
	}

	uint16_t maxPower = 0;

	while (true)
	{
		uint16_t channelPower;
		
		MYKONOS_getDacPower(&mykDevice, 1, &channelPower);
		maxPower = maxPower < channelPower ? channelPower : maxPower;
		printf("\t\tPower = %hu\r", maxPower);
		sleep(0.1);
		if (kbhit())
			break;
	}
	
	printf("\tRemove DAC protection\n");
	MYKONOS_enablePaProtection(&mykDevice, 0);

	printf("\nDone\n");
	//uint8_t pllLockStatus;
	//sin_rx(&mykDevice, &pllLockStatus);
	//dvbs2x_init();
	return 0;

error_11:
	printf("%s", errorString);
#ifndef ALTERA_PLATFORM
	error_10 :
#endif
#if (RX_CORE_BASEADDR != 0)
			 adxcvr_remove(rx_os_adxcvr);
		 error_9:
			 adxcvr_remove(tx_adxcvr);
		 error_8:
			 adxcvr_remove(rx_adxcvr);
#endif
		 error_7:
			 axi_jesd204_rx_remove(rx_os_jesd);
		 error_6:
			 axi_jesd204_tx_remove(tx_jesd);
		 error_5:
			 axi_jesd204_rx_remove(rx_jesd);
		 error_4:
#ifdef ALTERA_PLATFORM
			 altera_a10_fpll_remove(rx_os_device_clk_pll);
#else
			 axi_clkgen_remove(rx_os_clkgen);
#endif
		 error_3:
#ifdef ALTERA_PLATFORM
			 altera_a10_fpll_remove(tx_device_clk_pll);
#else
			 axi_clkgen_remove(tx_clkgen);
#endif
		 error_2:
#ifdef ALTERA_PLATFORM
			 altera_a10_fpll_remove(rx_device_clk_pll);
#else
			 axi_clkgen_remove(rx_clkgen);
#endif
		 error_1:
			 platform_remove();
		 error_0:
			 return FAILURE;
}

int kbhit(void)
{
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if (ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}

	return 0;
}