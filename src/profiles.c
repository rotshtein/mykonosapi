#include "profiles.h"
#include <stddef.h>
#include <memory.h>


/*384 - Profile 0*/
/*************************************************************************************************************************/
static int16_t rxFirCoefs_384[] = { -2,-2,-1,6,14,14,-5,-36,-58,-37,36,127,158,62,-146,-336,-328,-32,428,726,537,-181,-1036,-1364,-688,821,2242,2378,597,-2473,-4971,-4698,-454,6982,14933,20053,20053,14933,6982,-454,-4698,-4971,-2473,597,2378,2242,821,-688,-1364,-1036,-181,537,726,428,-32,-328,-336,-146,62,158,127,36,-37,-58,-36,-5,14,14,6,-1,-2,-2 };

static mykonosFir_t rxFir_384 =
{
	-6,             /* Filter gain in dB*/
	72,             /* Number of coefficients in the FIR filter*/
	&rxFirCoefs_384[0]  /* A pointer to an array of filter coefficients*/
};


static uint16_t rxAdcCustom_384[] = { 479,285,190,98,1280,112,1505,53,1574,25,1026,40,48,48,29,186 };

static mykonosRxProfile_t rxProfile_384 =
{/* RX 20MHz, Iqrate 38.4MSPS, DEC5 */
	1,              /* The divider used to generate the ADC clock*/
	&rxFir_384,     /* Pointer to Rx FIR filter structure*/
	4,              /* Rx FIR decimation (1,2,4)*/
	5,              /* Decimation of Dec5 or Dec4 filter (5,4)*/
	1,              /* If set, and DEC5 filter used, will use a higher rejection DEC5 FIR filter (1=Enabled, 0=Disabled)*/
	2,              /* RX Half band 1 decimation (1 or 2)*/
	38400,          /* Rx IQ data rate in kHz*/
	20000000,       /* The Rx RF passband bandwidth for the profile*/
	20000,          /* Rx BBF 3dB corner in kHz*/
	&rxAdcCustom_384[0] /* pointer to custom ADC profile*/
};

/*1536 Profile 1*/
/*************************************************************************************************************************/
static int16_t rxFirCoefs_1536[] = { -12,9,33,49,-11,-119,-175,-36,270,478,245,-442,-1053,-821,482,1990,2133,-11,-3429,-5241,-2546,5144,14845,21583,21583,14845,5144,-2546,-5241,-3429,-11,2133,1990,482,-821,-1053,-442,245,478,270,-36,-175,-119,-11,49,33,9,-12 };

static mykonosFir_t rxFir_1536 =
{
	-6,             /* Filter gain in dB*/
	48,             /* Number of coefficients in the FIR filter*/
	&rxFirCoefs_1536[0]  /* A pointer to an array of filter coefficients*/
};


static uint16_t rxAdcCustom_1536[] = { 476,286,190,98,1280,132,1509,63,1575,29,1024,39,48,48,29,186 };

static mykonosRxProfile_t rxProfile_1536 =
{/* RX 62MHz, Iqrate 153.6MSPS, DEC5 */
	1,              /* The divider used to generate the ADC clock*/
	&rxFir_1536,         /* Pointer to Rx FIR filter structure*/
	2,              /* Rx FIR decimation (1,2,4)*/
	5,              /* Decimation of Dec5 or Dec4 filter (5,4)*/
	1,              /* If set, and DEC5 filter used, will use a higher rejection DEC5 FIR filter (1=Enabled, 0=Disabled)*/
	1,              /* RX Half band 1 decimation (1 or 2)*/
	153600,         /* Rx IQ data rate in kHz*/
	62000000,       /* The Rx RF passband bandwidth for the profile*/
	62000,          /* Rx BBF 3dB corner in kHz*/
	&rxAdcCustom_1536[0] /* pointer to custom ADC profile*/
};


void change_rx_profile(int p, mykonosRxProfile_t *rxProfile)
{
	switch (p)
	{
	case 0:
		memcpy(rxProfile, &rxProfile_384, sizeof(rxProfile));
		break;

	case 1:
		memcpy(rxProfile, &rxProfile_1536, sizeof(rxProfile));
		break;
	}
}
