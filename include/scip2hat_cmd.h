/****************************************************************/
/**
  @file   libscip2hat_cmd.h
  @brief  Library for Sokuiki-Sensor "URG"
  @author HATTORI Kohei <hattori[at]team-lab.com>
 */
/****************************************************************/

#ifndef __LIBSCIP2HAT_CMD_H__
#define __LIBSCIP2HAT_CMD_H__

#ifdef __cplusplus
extern "C"
{
#endif



#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#include "scip2hat.h"
#include "scip2hat_dbuffer.h"



/** Structure of VV command */
typedef struct SCIP2_SENSOR_VERSION
{
    char vender[SCIP2_MAX_LENGTH];
    char product[SCIP2_MAX_LENGTH];
    char firmware[SCIP2_MAX_LENGTH];
    char protocol[SCIP2_MAX_LENGTH];
    char serialno[SCIP2_MAX_LENGTH];
} S2Ver_t;



/** Structure of PP command */
typedef struct SCIP2_SENSOR_PARAM
{
    char model[SCIP2_MAX_LENGTH];
    int dist_min;
    int dist_max;
    int step_resolution;
    int step_min;
    int step_max;
    int step_front;
    int revolution;
} S2Param_t;



int Scip2CMD_SCIP2( S2Port * apPort );
int Scip2CMD_SS( S2Port * apPort, const speed_t acBitrate );
int Scip2CMD_BM( S2Port * apPort );
int Scip2CMD_QT( S2Port * apPort );
int Scip2CMD_GS( S2Port * apPort, int aStart, int aEnd, int aGroup, S2Sdd_t * aData, const S2EncType acEnc );
int Scip2CMD_StopGS( S2Port * apPort, S2Sdd_t * aData );
int Scip2CMD_TM_GetStartTime( S2Port * apPort, struct timeval *apTime );
int Scip2CMD_TM_GetSyncTime( S2Port * apPort, unsigned long *adTime, struct timeval *apTime );
int Scip2CMD_RS( S2Port * apPort );
int Scip2CMD_CR( S2Port * apPort, const int acDeboost );
int Scip2CMD_StopMS( S2Port * apPort, S2Sdd_t * aData );
int Scip2CMD_StartMS( S2Port * apPort, int aStart, int aEnd, int aGroup,
int aCull, int aNum, S2Sdd_t * aData, const S2EncType acEnc );
int Scip2CMD_StopND( S2Port * apPort, S2Sdd_t * aData );
int Scip2CMD_StartND( S2Port * apPort, int aStart, int aEnd, int aGroup,
int aCull, int aNum, S2Sdd_t * aData, const S2EncType acEnc );
int Scip2CMD_VV( S2Port * apPort, S2Ver_t * apVer );
int Scip2CMD_PP( S2Port * apPort, S2Param_t * apParam );



/** alternative of Scip2CMD_TM for compatibility */
#define Scip2CMD_TM Scip2CMD_TM_GetStartTime



#ifdef __cplusplus
}
#endif

#endif	/* __LIBSCIP2HAT_CMD_H__ */
