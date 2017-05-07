#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#if defined(OS_LINUX) || defined(OS_MACOSX)
#include <sys/ioctl.h>
#include <termios.h>
#elif defined(OS_WINDOWS) || defined(WIN32) || defined(WIN64)
#include <conio.h>
#include <windows.h>
#endif

#define DEFAULT_POWERON     1
#define DEFAULT_PHASE256    0
#define DEFAULT_FREQUENCY   40000


#include "hid.h"

#define DEVICE_TIMEOUT_MS		10
#define RAWHID_PACKET_SIZE		64

bool emulateKeyerHW = false;

static char get_keystroke(void);


#define EEPROM_SIZE        2048


#pragma pack(push, 1) // exact fit - no padding


struct Config
{
	Config()
		: frequency(DEFAULT_FREQUENCY)
		, powerOn(DEFAULT_POWERON)
		, phase256(DEFAULT_PHASE256)
		, dummyA(0)
		, dummyB(0)
		, PIN_SER_DATA(16)  //  ==> DATA - yellow
		, PIN_W_CLK(17)  //  ==> W_CLK - blue
		, PIN_FQUPD(18)  //  ==> FQ_UPDATE - green
		, PIN_RESET(19)  //  ==> RESET  - red
	{
		begID[0] = 'A';
		begID[1] = 'D';
		begID[2] = '9';
		begID[3] = '8';

		endID[0] = '9';
		endID[1] = '8';
		endID[2] = '5';
		endID[3] = '0';
	}

	bool isValid() const {
		Config tmp;
		if (begID[0] != tmp.begID[0] || begID[1] != tmp.begID[1] || begID[2] != tmp.begID[2] || begID[3] != tmp.begID[3]
			|| endID[0] != tmp.endID[0] || endID[1] != tmp.endID[1] || endID[2] != tmp.endID[2] || endID[3] != tmp.endID[3])
			return false;
		return true;
	}

	void Reset() {
		Config tmp;
		*this = tmp;
	}


	// off: 0
	char begID[4];

	// off: 4
	uint32_t frequency;

	// off: 8
	uint8_t  powerOn;  // != 0 to activate power
	// off: 9
	uint8_t  phase256; // usually 0

	// off: 10
	uint8_t dummyA;
	uint8_t dummyB;

	// off: 12
	uint8_t PIN_SER_DATA; //  ==> DATA - yellow
	uint8_t PIN_W_CLK;    //  ==> W_CLK - blue
	uint8_t PIN_FQUPD;    //  ==> FQ_UPDATE - green
	uint8_t PIN_RESET;    //  ==> RESET  - red

	// off: 12
	char endID[4];
	// off: 16
};

#pragma pack(pop) //back to whatever the previous packing mode was

Config c;


bool readConfFromDev()
{
	if (emulateKeyerHW)
		return true;
	for (int tryNo = 0; tryNo < 5; ++tryNo)
	{
		char buf[RAWHID_PACKET_SIZE];
		int ret;

		for (int i = 0; i < RAWHID_PACKET_SIZE; ++i)	buf[i] = 0;
		buf[0] = 'C';
		buf[1] = 'F';
		buf[2] = 'G';
		buf[3] = 'R';

		ret = rawhid_send(0, buf, RAWHID_PACKET_SIZE, DEVICE_TIMEOUT_MS);
		printf("sent CFG-Request with 4 bytes. rawhid_send returned %d bytes sent\n", ret);
		if (ret < 0)
			continue;

		ret = rawhid_recv(0, buf, RAWHID_PACKET_SIZE, DEVICE_TIMEOUT_MS);
		if (ret < 0)
			continue;

		if (buf[0] != 'C' || buf[1] != 'F' || buf[2] != 'G' || buf[3] != 'U')
			continue;

		Config localC;
		memcpy(&localC, &buf[4], sizeof(Config));
		if (!localC.isValid())
			continue;

		memcpy(&c, &buf[4], sizeof(Config));
		return true;
	}
	return false;
}

bool writeConfToDev(bool writeToEEPROM = false)
{
	char buf[RAWHID_PACKET_SIZE];
	int ret;

	if (!c.isValid())
		return false;

	if (emulateKeyerHW)
		return true;

	if (writeToEEPROM)
	{
		for (int i = 0; i < RAWHID_PACKET_SIZE; ++i)	buf[i] = 0;
		buf[0] = 'C';
		buf[1] = 'F';
		buf[2] = 'G';
		buf[3] = 'W';

		ret = rawhid_send(0, buf, RAWHID_PACKET_SIZE, DEVICE_TIMEOUT_MS);
		printf("sent CFG-Write. rawhid_send returned %d bytes sent\n", ret);
		if (ret < 0)
			return false;
	}
	else
	{
		for (int i = 0; i < RAWHID_PACKET_SIZE; ++i)	buf[i] = 0;
		buf[0] = 'C';
		buf[1] = 'F';
		buf[2] = 'G';
		buf[3] = 'U';
		memcpy(&buf[4], &c, sizeof(Config));

		ret = rawhid_send(0, buf, RAWHID_PACKET_SIZE, DEVICE_TIMEOUT_MS);
		printf("sent CFG-Update. rawhid_send returned %d bytes sent\n", ret);
		if (ret < 0)
			return false;
	}

	return true;
}

void printMenuAndConf()
{
	unsigned tu;
	printf("\n\n\n\nMENU:\n");
	printf("1)  read config from device!\n");
	printf("2)  write config (on device) to EEPROM!\n");
	printf("3)  quit!\n");
	printf("\n");
	tu = c.frequency;				printf("10) frequency /Hz         %u\n", tu);
	tu = c.powerOn;					printf("11) power On? [0 / 1]     %u\n", tu);
	tu = c.phase256;				printf("12) phase [ 0 .. 255 ]    %u\n", tu);
	tu = c.PIN_SER_DATA;			printf("13) pin# SERIAL DATA      %u\n", tu);
	tu = c.PIN_W_CLK;				printf("14) pin# W_CLK            %u\n", tu);
	tu = c.PIN_FQUPD;				printf("15) pin# FQ UPD           %u\n", tu);
	tu = c.PIN_RESET;				printf("16) pin# RESET            %u\n", tu);
	printf("\n");
	printf("Enter command value: ");
	fflush(stdout);
}

int main(int argc, char * argv[])
{
	for (int k = 1; k < argc; ++k)
	{
		if (!strcmp(argv[k], "-e") || !strncmp(argv[k], "em", 2))
			emulateKeyerHW = true;
	}

	bool bReOpen = false;
	bool bQuit = false;

	while (!bQuit)
	{
		int r;

		if (!emulateKeyerHW)
		{
			::_sleep(1000);

			// C-based example is 16C0:0480:FFAB:0200
			r = rawhid_open(1, 0x16C0, 0x0480, 0xFFAB, 0x0200);
			if (r <= 0) {
				// Arduino-based example is 16C0:0486:FFAB:0200
				r = rawhid_open(1, 0x16C0, 0x0486, 0xFFAB, 0x0200);
				if (r <= 0) {
					printf("no rawhid device found - connect to USB port!\n");
					//return -1;
					::_sleep(1000);
					continue;
				}
			}
			printf("found rawhid device - requesting current config ..\n\n");

			if (!readConfFromDev())
			{
				printf("error reading config, possibly device went offline\n");
				rawhid_close(0);
				::_sleep(1000);
				continue;
			}
		}

		bReOpen = false;
		bQuit = false;
		while (!bReOpen && !bQuit)
		{
			unsigned menuVal;
			unsigned confVal;
			printMenuAndConf();
			scanf("%u", &menuVal);
			switch (menuVal)
			{
			default:	break;
			case 1:		if (!readConfFromDev())	bReOpen = true;		break;
			case 2:		if (!writeConfToDev(true))	bReOpen = true;	break;
			case 3:		bQuit = true;	break;
			}
			if (menuVal >= 10)
			{
				printf("Enter new config value: ");
				fflush(stdout);
				int tmp;
				scanf("%d", &tmp);
				//confVal = (tmp < 0) ? 0xffff : (tmp & 0xffff);	// limit to 16 bit
				confVal = tmp;
				switch (menuVal)
				{
				default:	break;
				case 10:	c.frequency = confVal;			break;
				case 11:	c.powerOn = confVal;			break;
				case 12:	c.phase256 = confVal;			break;
				case 13:	c.PIN_SER_DATA = confVal;		break;
				case 14:	c.PIN_W_CLK = confVal;			break;
				case 15:	c.PIN_FQUPD = confVal;			break;
				case 16:	c.PIN_RESET = confVal;			break;
				}
				if (!writeConfToDev(false))
					bReOpen = true;;
			}
		} // end while (!bReOpen && !bQuit)

		if (bReOpen)
			printf("error reading config, possibly device went offline\n");
		if (!emulateKeyerHW && (bReOpen || bQuit))
			rawhid_close(0);
	} // end while (!bQuit)
	return 0;
}

#if defined(OS_LINUX) || defined(OS_MACOSX)
// Linux (POSIX) implementation of _kbhit().
// Morgan McGuire, morgan@cs.brown.edu
static int _kbhit() {
	static const int STDIN = 0;
	static int initialized = 0;
	int bytesWaiting;

	if (!initialized) {
		// Use termios to turn off line buffering
		struct termios term;
		tcgetattr(STDIN, &term);
		term.c_lflag &= ~ICANON;
		tcsetattr(STDIN, TCSANOW, &term);
		setbuf(stdin, NULL);
		initialized = 1;
	}
	ioctl(STDIN, FIONREAD, &bytesWaiting);
	return bytesWaiting;
}
static char _getch(void) {
	char c;
	if (fread(&c, 1, 1, stdin) < 1) return 0;
	return c;
}
#endif


static char get_keystroke(void)
{
	if (_kbhit()) {
		char c = _getch();
		if (c >= 32) return c;
	}
	return 0;
}


