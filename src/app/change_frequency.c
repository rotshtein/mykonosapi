#include "change_frequency.h"
#include "stdbool.h"

mykonosErr_t small_change_frequency(mykonosDevice_t *mykDevice, uint64_t new_rx_freq, uint64_t new_tx_freq);
mykonosErr_t large_change_frequency(mykonosDevice_t *mykDevice, uint64_t new_rx_freq, uint64_t new_tx_freq);
char *errorString;


char* chnage_frequenct_last_error()
{
	return errorString;
}

mykonosErr_t change_frequency(mykonosDevice_t *mykDevice, 
								uint64_t current_rx_freq, uint64_t new_rx_freq,
								uint64_t current_tx_freq, uint64_t new_tx_freq)
{
	if (((current_rx_freq == new_rx_freq) || (new_rx_freq == 0)) &&
		((current_tx_freq == new_rx_freq) || (new_tx_freq == 0)))
	{
		errorString = "Nothing to do\n";
		return MYKONOS_ERR_OK;
	}

	bool large_change = new_rx_freq > 0 ? (new_rx_freq > 1500000000) || abs(current_rx_freq - new_rx_freq) > 100000000 : false;
	large_change |= new_tx_freq > 0 ? (new_tx_freq > 1500000000) || abs(current_tx_freq - new_tx_freq) > 100000000 : false;

	errorString = "";
	if (large_change)
	{
		printf("\t\tLarge change\n");
		return large_change_frequency(mykDevice, new_rx_freq, new_tx_freq);
	}
	
	printf("\t\tSmall change\n");
	return small_change_frequency(mykDevice, new_rx_freq, new_tx_freq);
}


mykonosErr_t small_change_frequency(mykonosDevice_t *mykDevice, uint64_t new_rx_freq, uint64_t new_tx_freq)
{
	mykonosErr_t mykError;

	//Move the device into the radio off state
	if ((mykError =	MYKONOS_radioOff(mykDevice)) != MYKONOS_ERR_OK)
	{
		errorString = getMykonosErrorMessage(mykError);
	}

	/* Program the new local oscillator (LO) frequency. For example, set the Rx LO to 2550 MHz and the Tx LO to
	2500 MHz by executing the following commands :
	*/

	if (new_rx_freq > 0)
	{
		if ((mykError = MYKONOS_setRfPllFrequency(mykDevice, RX_PLL, new_rx_freq)) != MYKONOS_ERR_OK)
		{
			errorString = getMykonosErrorMessage(mykError);
		}
	}

	if (new_tx_freq > 0)
	{
		if ((mykError = MYKONOS_setRfPllFrequency(mykDevice, TX_PLL, new_tx_freq)) != MYKONOS_ERR_OK)
		{
			errorString = getMykonosErrorMessage(mykError);
		}
	}
	/*** < Action: wait 200ms for PLLs	to lock > ***/
	mdelay(200);

	uint8_t pllLockStatus;
	if ((mykError = MYKONOS_checkPllsLockStatus(mykDevice, &pllLockStatus)) != MYKONOS_ERR_OK)
	{
		errorString = getMykonosErrorMessage(mykError);
	}

	if ((pllLockStatus & 0x0F) == 0x07)
	{
		/*** < Info: Clock, Rx and Tx PLLs locked > ***/
	}
	else
	{
		errorString = "PLL not locked after 200 mSec";
		return MYKONOS_ERR_FAILED;
	}

	

	//Move the device back into the radio on state
	if ((mykError =	MYKONOS_radioOn(mykDevice)) !=	MYKONOS_ERR_OK)
	{
		errorString = getMykonosErrorMessage(mykError);
		return mykError;
	}

	/*	Some tracking calibrations are active only when the ORx path is set to the internal calibrations input. The user can
		reenable tracking calibrations by selecting the internal calibrations ORx path by executing the following command
		when using application programming interface (API) commands to control the ORx input:
	*/

	if ((mykError =	MYKONOS_setObsRxPathSource(mykDevice, OBS_INTERNALCALS)) != MYKONOS_ERR_OK)
	{
		errorString = getMykonosErrorMessage(mykError);
		return mykError;
	}

	return MYKONOS_ERR_OK;
}
mykonosErr_t large_change_frequency(mykonosDevice_t *mykDevice, uint64_t new_rx_freq, uint64_t new_tx_freq)
{
	mykonosErr_t mykError;

	//Move the device into the radio off state
	if ((mykError = MYKONOS_radioOff(mykDevice)) != MYKONOS_ERR_OK)
	{
		errorString = getMykonosErrorMessage(mykError);
	}


	/* Program the new local oscillator (LO) frequency. For example, set the Rx LO to 2550 MHz and the Tx LO to
	2500 MHz by executing the following commands :
	*/

	if (new_rx_freq > 0)
	{
		if ((mykError = MYKONOS_setRfPllFrequency(mykDevice, RX_PLL, new_rx_freq)) != MYKONOS_ERR_OK)
		{
			errorString = getMykonosErrorMessage(mykError);
		}
	}

	if (new_tx_freq > 0)
	{
		if ((mykError = MYKONOS_setRfPllFrequency(mykDevice, TX_PLL, new_tx_freq)) != MYKONOS_ERR_OK)
		{
			errorString = getMykonosErrorMessage(mykError);
		}
	}
	/*** < Action: wait 200ms for PLLs	to lock > ***/
	mdelay(200);

	uint8_t pllLockStatus;
	if ((mykError = MYKONOS_checkPllsLockStatus(mykDevice, &pllLockStatus)) != MYKONOS_ERR_OK)
	{
		errorString = getMykonosErrorMessage(mykError);
	}

	if ((pllLockStatus & 0x0F) == 0x07)
	{
		/*** < Info: Clock, Rx and Tx PLLs locked > ***/
	}
	else
	{
		errorString = "PLL not locked after 200 mSec";
		return MYKONOS_ERR_FAILED;
	}

	//Rerun the initialization calibrations

	uint8_t errorFlag, errorCode;
	uint32_t initCalMask = TX_QEC_INIT | LOOPBACK_RX_LO_DELAY |	LOOPBACK_RX_RX_QEC_INIT | RX_LO_DELAY | RX_QEC_INIT;
	if ((mykError =	MYKONOS_runInitCals(mykDevice, initCalMask)) != MYKONOS_ERR_OK)
	{
		errorString = getMykonosErrorMessage(mykError);
		return MYKONOS_ERR_FAILED;
	}
	if ((mykError = MYKONOS_waitInitCals(mykDevice, 60000, &errorFlag, &errorCode)) != MYKONOS_ERR_OK)
	{
		errorString = getMykonosErrorMessage(mykError);
		return MYKONOS_ERR_FAILED;
	}
	if ((errorFlag != 0) || (errorCode != 0))
	{
		uint8_t initCalStatus;
		if ((mykError =	MYKONOS_getInitCalStatus(mykDevice, &initCalStatus)) != MYKONOS_ERR_OK)
		{
			errorString = getMykonosErrorMessage(mykError);
			return MYKONOS_ERR_FAILED;
		}
		/*** < Info: abort init	cals > ***/
		uint8_t initCalsCompleted;
		if ((mykError = MYKONOS_abortInitCals(mykDevice, &initCalsCompleted)) != MYKONOS_ERR_OK)
		{
			errorString = getMykonosErrorMessage(mykError);
			return MYKONOS_ERR_FAILED;
		}
		if (initCalsCompleted)
		{
			printf("Info: which calls had completed, per the mask\n");
		}
		uint16_t errorWord, statusWord;
		if ((mykError =	MYKONOS_readArmCmdStatus(mykDevice, &errorWord, &statusWord)) != MYKONOS_ERR_OK)
		{
			errorString = getMykonosErrorMessage(mykError);
			return MYKONOS_ERR_FAILED;
		}

		uint8_t status;
		if ((mykError = MYKONOS_readArmCmdStatusByte(mykDevice, 2, &status)) != MYKONOS_ERR_OK)
		{
			errorString = getMykonosErrorMessage(mykError);
			return MYKONOS_ERR_FAILED;
		}
		if (status != 0)
		{
			/*** < Info: Arm Mailbox Status Error errorWord > ***/
			/*** < Info: Pending Flag per opcode statusWord, this follows the mask > ***/
			errorString = "Arm Mailbox Status Error errorWord, Pending Flag per opcode statusWord, this follows the mask\n";
			return MYKONOS_ERR_FAILED;
		}
	}
	else
	{
		/*** < Info: Calibrations completed successfully > ***/
	}
		
	//Move the device back into the radio on state

	if ((mykError =	MYKONOS_radioOn(mykDevice)) != MYKONOS_ERR_OK)
	{
		errorString = getMykonosErrorMessage(mykError);
		return MYKONOS_ERR_FAILED;

	}

	if ((mykError = MYKONOS_setObsRxPathSource(mykDevice, OBS_INTERNALCALS)) != MYKONOS_ERR_OK)
	{
		errorString = getMykonosErrorMessage(mykError);
		return MYKONOS_ERR_FAILED;
	}

	return MYKONOS_ERR_OK;
}