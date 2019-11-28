#include "sin.h"


#define gettheAD9371ErrorMessage(a) "Error"

void sin_rx(mykonosDevice_t *mykDevice, uint8_t pllLockStatus)
{
	mykonosErr_t mykError;
	char *errorString;

	if ((mykError = MYKONOS_radioOff(mykDevice)) != MYKONOS_ERR_OK)
	{
		/*** < Info: errorString will contain log error string in order to debug failure > ***/
		//errorString = gettheAD9371ErrorMessage(mykError);
	}


	if ((mykError = MYKONOS_setRfPllFrequency(mykDevice, RX_PLL, 1234000000)) != MYKONOS_ERR_OK)
	{
		/*** < Info: errorString will
		contain log error string in order to
		debug failure > ***/
		errorString = gettheAD9371ErrorMessage(mykError);
	}

	if ((mykError = MYKONOS_setRfPllFrequency(mykDevice, TX_PLL, 1234000000)) != MYKONOS_ERR_OK)
	{
		/*** < Info: errorString will contain log error string in order to debug failure > ***/
		errorString = gettheAD9371ErrorMessage(mykError);
	}

	/*** < Action: wait 200ms for PLLs to lock > ***/
	if ((mykError = MYKONOS_checkPllsLockStatus(mykDevice, &pllLockStatus)) != MYKONOS_ERR_OK)
	{
		/*** < Info: errorString will contain log error string in order to debug failure > ***/
		errorString = gettheAD9371ErrorMessage(mykError);
	}

	if ((pllLockStatus & 0x0F) == 0x07)
	{
		/*** < Info: Clock, Rx and Tx PLLs locked > ***/
	}
	else
	{
		/*** < Info: Clock, Rx and Tx PLLs not locked > ***/
		/*** < Action: Ensure lock before proceeding - User code here > ***/
	}

	uint32_t initCalMask = TX_QEC_INIT | LOOPBACK_RX_LO_DELAY | LOOPBACK_RX_RX_QEC_INIT | RX_LO_DELAY | RX_QEC_INIT;
	if ((mykError = MYKONOS_runInitCals(mykDevice, initCalMask)) != MYKONOS_ERR_OK)
	{
		/*** < Info: errorString will contain log error string in order to debug failure > ***/
		errorString = gettheAD9371ErrorMessage(mykError);
	}
	uint8_t errorFlag, errorCode;

	if ((mykError = MYKONOS_waitInitCals(mykDevice, 60000, &errorFlag, &errorCode)) != MYKONOS_ERR_OK)
	{
		/*** < Info: errorString will contain log error string in order to debug failure > ***/
		errorString = gettheAD9371ErrorMessage(mykError);
	}
	if ((errorFlag != 0) || (errorCode != 0))
	{
		mykonosInitCalStatus_t initCalStatus;
		if ((mykError = MYKONOS_getInitCalStatus(mykDevice, &initCalStatus)) != MYKONOS_ERR_OK)
		{
			/*** < Info: errorString will contain log error string in order to debug failure > ***/
			errorString = gettheAD9371ErrorMessage(mykError);
		}
		/*** < Info: abort init cals > ***/
		uint32_t initCalsCompleted;
		if ((mykError = MYKONOS_abortInitCals(mykDevice, &initCalsCompleted)) != MYKONOS_ERR_OK)
		{
			/*** < Info: errorString will contain log error string in order to debug failure > ***/
			errorString = gettheAD9371ErrorMessage(mykError);
		}
		if (initCalsCompleted)
		{
			/*** < Info: which calls had completed, per the mask > ***/
		}
		uint16_t errorWord, uint16_t, statusWord;
		uint8_t status;
		if ((mykError = MYKONOS_readArmCmdStatus(mykDevice, &errorWord, &statusWord)) != MYKONOS_ERR_OK)
		{
			/*** < Info: errorString will contain log error string in order to debug failure > ***/
			errorString = gettheAD9371ErrorMessage(mykError);
		}
		if ((mykError = MYKONOS_readArmCmdStatusByte(mykDevice, 2, &status)) != MYKONOS_ERR_OK)
		{
			/*** < Info: errorString will contain log error string in order to debug why failed > ***/
			errorString = gettheAD9371ErrorMessage(mykError);
		}

		if (status != 0)
		{
			/*** < Info: Arm Mailbox Status Error errorWord > ***/
			/*** < Info: Pending Flag per opcode statusWord, this follows the mask > ***/
		}
	}
	else
	{
		/*** < Info: Calibrations completed successfully > ***/
	}

	if ((mykError = MYKONOS_radioOn(mykDevice)) != MYKONOS_ERR_OK)
	{
		/*** < Info: errorString will contain log error string in order to debug failure > ***/
		errorString = gettheAD9371ErrorMessage(mykError);
	}



}