
/**
  @file   libscip2hat.h
  @brief  Library for Sokuiki-Sensor "URG"
  @author HATTORI Kohei <hattori[at]team-lab.com>
 */

#ifndef __LIBSCIP2HAT_BASE_H__
#define __LIBSCIP2HAT_BASE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>

#include "scip2hat.h"

/** SCIP2 encode type */
typedef enum SCIP2_ENCODE_TYPE_E
{

    SCIP2_ENC_2BYTE = 2,
    /**< 2 bytes encoding */

    SCIP2_ENC_3BYTE,/**< 3 bytes encoding */

    SCIP2_ENC_4BYTE, /**< 4 bytes encoding */

    SCIP2_ENC_3X2BYTE
    /**< 3 bytes encoding x2 */
} S2EncType;

/** SCIP2 handle */
typedef FILE S2Port;

S2Port *Scip2_Open( const char *acpDevice, const speed_t acBitrate );
S2Port *Scip2_OpenEthernet( const char *acpAddress, const int acPort );
int Scip2_Close( S2Port * apPort );
void Scip2_Flush( S2Port * apPort );
int bitrate2i( const speed_t acBitrate );
int Scip2_SendTerm( S2Port * apPort );
int Scip2_RecvTerm( S2Port * apPort );
int Scip2_Send( S2Port * apPort, const char *apcMes );
int Scip2_Recv( S2Port * apPort, char *apMes, int aNMes );
int Scip2_ChangeBitrate( S2Port * apPort, const speed_t acBitrate );
int Scip2_RecvStatus( S2Port * apPort );
int Scip2_RecvEncodedLine( S2Port * apPort, unsigned long *apBuf, int aNBuf,
const S2EncType acEnc, unsigned long *apRemains, int *apNRemains );

/** Maximum length par Line */
#define SCIP2_MAX_LENGTH 128

/** define to print out error message */
//	 #define SCIP2_DEBUG
//	 #define SCIP2_DEBUG_ALL
//	 #define SCIP2_OUTPUT_CONTDATA

#if defined(SCIP2_DEBUG_ALL) || defined(SCIP2_OUTPUT_CONTDATA)
extern char scip2_debuf[SCIP2_MAX_LENGTH];
#endif

#ifdef __cplusplus
}
#endif

#endif											/* __LIBSCIP2HAT_BASE_H__ */
