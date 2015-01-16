#include "stdio.h"


class ThermistorLookupTable
{

	

public:
	int temptable[17][2];
	ThermistorLookupTable()
	{
		//temptable[0][0] = 266;
		//temptable[0][1] = 15;
		//temptable[1][0] = 319;
		//temptable[1][1] = 34;
		//temptable[2][0] = 372;
		//temptable[2][1] = 53;
		//temptable[3][0] = 425;
		//temptable[3][1] = 71;
		//temptable[4][0] = 478;
		//temptable[4][1] = 88;
		//temptable[5][0] = 531;
		//temptable[5][1] = 106;
		//temptable[6][0] = 584;
		//temptable[6][1] = 125;
		//temptable[7][0] = 637;
		//temptable[7][1] = 145;
		//temptable[8][0] = 690;
		//temptable[8][1] = 167;
		//temptable[9][0] = 743;
		//temptable[9][1] = 191;
		//temptable[10][0] = 796;
		//temptable[10][1] = 220;
		//temptable[11][0] = 849;
		//temptable[11][1] = 255;
		//temptable[12][0] = 902;
		//temptable[12][1] = 304;
		//temptable[13][0] = 955;
		//temptable[13][1] = 383;
		//temptable[14][0] = 1008;
		//temptable[14][1] = 616;
		//temptable[15][0] = 1023;
		//temptable[15][1] = 1319;

		temptable[0][0] = 1;
		temptable[0][1] = 2838;
		temptable[1][0] = 64;
		temptable[1][1] = 859;
		temptable[2][0] = 127;
		temptable[2][1] = 698;
		temptable[3][0] = 190;
		temptable[3][1] = 609;
		temptable[4][0] = 253;
		temptable[4][1] = 547;
		temptable[5][0] = 316;
		temptable[5][1] = 498;
		temptable[6][0] = 379;
		temptable[6][1] = 457;
		temptable[7][0] = 442;
		temptable[7][1] = 420;
		temptable[8][0] = 505;
		temptable[8][1] = 386;
		temptable[9][0] = 568;
		temptable[9][1] = 354;
		temptable[10][0] = 631;
		temptable[10][1] = 322;
		temptable[11][0] = 694;
		temptable[11][1] = 290;
		temptable[12][0] = 757;
		temptable[12][1] = 256;
		temptable[13][0] = 820;
		temptable[13][1] = 218;
		temptable[14][0] = 883;
		temptable[14][1] = 173;
		temptable[15][0] = 946;
		temptable[15][1] = 111;
		temptable[16][0] = 1003;
		temptable[16][1] = 1;
	}
};
