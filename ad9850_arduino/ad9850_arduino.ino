
/* (c)2017 Hayati Ayguen <h_ayguen@web.de>
 * License: MIT
 *
 * program for microController Teensy 3.1 with IDE Teensyduino:
 * settings for Arduino IDE:
 *   Board:           Teensy 3.1
 *   USB Type:        Raw HID
 *   CPU Speed:       72 MHz
 *   Keyboard Layout: US English
 *
 * you might want to change default Config settings:
 * see constructors initializer list at lines 53ff
 */

#include <EEPROM.h>
#include <WProgram.h>
#include "usb_desc.h"
#include "AH_AD9850.h"

#define DEFAULT_POWERON     1
#define DEFAULT_PHASE256    0
#define DEFAULT_FREQUENCY   40000
#define USE_DEFAULT_CONFIG  false

#define USE_SERIAL    1
#define USE_HID       1

#ifndef __cplusplus
  #error Expected C++ compiler!
#endif


// forward declarations
struct Config;
void readConfFromEEprom( Config * pConf );
void writeConfToEEprom( const Config * pConf );

AH_AD9850 *pAD9850 = NULL;

#if USE_SERIAL
  char acSerialMsg[256];
#endif

#pragma pack(push, 1) // exact fit - no padding

struct Config
{
  Config()
    : frequency( DEFAULT_FREQUENCY )
    , powerOn( DEFAULT_POWERON )
    , phase256( DEFAULT_PHASE256 )
    , dummyA( 0 )
    , dummyB( 0 )
    , PIN_SER_DATA(16)  //  ==> DATA - yellow
    , PIN_W_CLK   (17)  //  ==> W_CLK - blue
    , PIN_FQUPD   (18)  //  ==> FQ_UPDATE - green
    , PIN_RESET   (19)  //  ==> RESET  - red
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
    if ( begID[0] != tmp.begID[0] || begID[1] != tmp.begID[1] || begID[2] != tmp.begID[2] || begID[3] != tmp.begID[3]
      || endID[0] != tmp.endID[0] || endID[1] != tmp.endID[1] || endID[2] != tmp.endID[2] || endID[3] != tmp.endID[3] )
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

void readConfFromEEprom( Config * pConf )
{
  const int confSize = sizeof(Config);
  uint8_t aConf[ sizeof(Config) ];
  for ( int a = 0; a < confSize; ++a )
    aConf[a] = EEPROM.read(a);
  memcpy( pConf, aConf, confSize );
#if USE_SERIAL
  Serial.println( "read config from EEPROM:");
  sprintf(acSerialMsg, "config has %s config with frequency %ld Hz:",
    (pConf->isValid() ? "valid" : "invalid"), (long int)pConf->frequency );
  Serial.println( acSerialMsg );
#endif
  if ( ! pConf->isValid() )
    pConf->Reset();
}

void writeConfToEEprom( const Config * pConf )
{
  const int confSize = sizeof(Config);
  uint8_t aConf[ sizeof(Config) ];

  if ( pConf->isValid() )
  {
    Config cFromEEprom;
    uint8_t prevConf[ sizeof(Config) ];
    uint8_t verifyConf[ sizeof(Config) ];
    int a;
    bool allEq = true;

    readConfFromEEprom( &cFromEEprom );
    memcpy( &prevConf, &cFromEEprom, confSize );

    memcpy( aConf, pConf, confSize );
    for ( a = 0; a < confSize; ++a )
    {
      if ( true || prevConf[a] != aConf[a] )
      {
        EEPROM.write( a, aConf[a] );
        verifyConf[a] = EEPROM.read(a);
#if USE_SERIAL
        if ( verifyConf[a] != aConf[a] )
        {
          sprintf(acSerialMsg, "EEPROM write verification at byte %d of %d failed!", a, confSize );
          Serial.println( acSerialMsg );
        }
        else
        {
          sprintf(acSerialMsg, "EEPROM write verification at byte %d of %d successful.", a, confSize );
          Serial.println( acSerialMsg );
        }
#endif
        allEq = allEq && ( verifyConf[a] == aConf[a] );
      }
    }

#if USE_SERIAL
    sprintf(acSerialMsg, "EEPROM write verification %s", (allEq ? "successful" : "failed!") );
    Serial.println( acSerialMsg );
#endif
  }
}


// 5ms == 5 * 4 delays a 250 us = 20
// x^20 = 0.5
// 20*log10(x) = log10(0.5)
// log10(x) = log10(0.5) / 20
// x = 10^( log10(0.5)/20 )
// x = 0.96

Config c;

uint8_t txBufC[64];
uint8_t rxBuf[64];


void setFreq()
{
  if (!pAD9850)
  {
    pAD9850 = new AH_AD9850( c.PIN_W_CLK, c.PIN_FQUPD, c.PIN_SER_DATA, c.PIN_RESET );

#if USE_SERIAL
    sprintf(acSerialMsg, "setting up AH_AD9850() with pin config");
    Serial.println( acSerialMsg );
#endif
  }

  pAD9850->set_frequency( (c.powerOn == 0), c.phase256, double(c.frequency) );

#if USE_SERIAL
  sprintf(acSerialMsg, "setting frequency %ld Hz", (long int)c.frequency );
  Serial.println( acSerialMsg );
#endif
}


void setupAD()
{
#if USE_SERIAL
  Serial.println( "setupAD");
#endif

  if (pAD9850)
    delete pAD9850;

  setFreq();

  delay(10);
  if ( c.powerOn )
    c.powerOn = 0;
  else
    c.powerOn = 1;
  setFreq();

  delay(10);
  if ( c.powerOn )
    c.powerOn = 0;
  else
    c.powerOn = 1;
  setFreq();
}


void setup()
{
#if USE_SERIAL
  Serial.begin(38400);
#endif

  {
    for ( int i = 0; i < 64; ++i )
      txBufC[i] = 0;

    txBufC[0] = 'C';
    txBufC[1] = 'F';
    txBufC[2] = 'G';
    txBufC[3] = 'U';
  }

  readConfFromEEprom( &c );

  delay(1000);

  setupAD();

  delay(2000);

#if USE_SERIAL
 #ifdef USB_RAWHID
  Serial.println( "USB_RAWHID set");
 #else
  Serial.println( "USB_RAWHID not set");
 #endif
 #ifdef VENDOR_ID
  sprintf(acSerialMsg, "VENDOR_ID         0x%0x", VENDOR_ID);
  Serial.println( acSerialMsg );
  sprintf(acSerialMsg, "PRODUCT_ID        0x%0x", PRODUCT_ID);
  Serial.println( acSerialMsg );
  sprintf(acSerialMsg, "RAWHID_USAGE_PAGE 0x%0x", RAWHID_USAGE_PAGE);
  Serial.println( acSerialMsg );
  sprintf(acSerialMsg, "RAWHID_USAGE      0x%0x", RAWHID_USAGE);
  Serial.println( acSerialMsg );
 #endif
#endif
}


void loop()
{
#if USE_SERIAL == 0
  while (1)
#endif
  {
#if USE_HID
    int nRx = RawHID.recv(rxBuf, 0); // 0 timeout = do not wait
#else
    int nRx = 0;
#endif

    if ( nRx == 64 && rxBuf[0]=='C' && rxBuf[1]=='F' && rxBuf[2]=='G' )
    {
      if ( rxBuf[3]=='U' || rxBuf[3]=='F' )  // CFG Update: 'U': use / update to attached configuration
                                             //   'F': update frequency only
      {
        Config nCfg;
        memcpy( &nCfg, &rxBuf[4], sizeof(Config) );
        if ( nCfg.isValid() )
        {
          if ( rxBuf[3]=='F' )
          {
            memcpy( &c, &rxBuf[4], sizeof(Config) );
            setFreq();
          }
          else if ( nCfg.PIN_SER_DATA != c.PIN_SER_DATA
                 || nCfg.PIN_W_CLK    != c.PIN_W_CLK
                 || nCfg.PIN_FQUPD    != c.PIN_FQUPD
                 || nCfg.PIN_RESET    != c.PIN_RESET )
          {
            memcpy( &c, &rxBuf[4], sizeof(Config) );
            setupAD();
          }
          else
          {
            memcpy( &c, &rxBuf[4], sizeof(Config) );
            setFreq();
          }
        }
      }

      else if ( rxBuf[3]=='F' )  // CFG Update: use / update to attached frequency
      {
        Config nCfg;
        memcpy( &nCfg, &rxBuf[4], sizeof(Config) );
        memcpy( &c, &rxBuf[4], sizeof(Config) );
        setFreq();
      }

      else if ( rxBuf[3]=='W' )  // CFG Write: write current configuration (no configuration attached)
      {
        writeConfToEEprom( &c );
#if USE_SERIAL
        Serial.println( "write current config to EEPROM." );
#endif
      }

      else if ( rxBuf[3]=='R' )  // CFG Request: transmit current configuration to host/PC
      {
        memcpy( &txBufC[4], &c, sizeof(Config) );
        RawHID.send(txBufC, 0);
      }
      nRx = 0;
    }

#if USE_SERIAL
    delayMicroseconds( 250 );  // 1/4 ms = 250 us
#endif
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////


