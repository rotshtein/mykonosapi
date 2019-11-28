#include "dvbs2x.h"
#include "platform_drivers.h"
#include "system.h"
#include "parameters.h"

#define DVBS_2x_MOD_BASE		0x6000
void dvbs2x_init()
{
	//0x6008 <= X"00000233";
	IOWR_32DIRECT(DVBS_2x_MOD_BASE, 8, 0x233);
	
	//0x600C <= X"29AAAAAB";
	IOWR_32DIRECT(DVBS_2x_MOD_BASE, 0xc, 0x29AAAAAB);
	
	//0x6018 <= X"00000002";
	IOWR_32DIRECT(DVBS_2x_MOD_BASE, 0x18, 0x2);

	//0x603C <= X"00000101";
	IOWR_32DIRECT(DVBS_2x_MOD_BASE, 0x3c, 0x101);

	//0x6008 <= X"00000232";
	IOWR_32DIRECT(DVBS_2x_MOD_BASE, 0x8, 0x232);
}