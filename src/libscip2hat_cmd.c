/****************************************************************/
/**
  @file   libscip2hat_cmd.c
  @brief  Library for Sokuiki-Sensor "URG"
  @author HATTORI Kohei <hattori[at]team-lab.com>
 */
/****************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "scip2hat.h"



/*--------------------------------------------------------------*/
/**
 * @brief Switch to SCIP2.0 mode
 * @param *apPort Pointer to SCIP2.0 Device Port
 * @return false: failed, true: succeeded
 */
/*--------------------------------------------------------------*/
int Scip2CMD_SCIP2( S2Port * apPort )
{
    //! return value of function
    int ret;

    ret = Scip2_Send( apPort, "SCIP2.0" );
    if( ret == -1 )
        return -1;
    if( !Scip2_RecvTerm( apPort ) )
        return -1;
    return ret;
}



/*--------------------------------------------------------------*/
/**
 * @brief Change Device's Bitrate
 * @param *apPort Pointer to SCIP2.0 Device Port
 * @param acBitrate Bitrate
 * @return failed: false, succeeded: true
 */
/*--------------------------------------------------------------*/
int Scip2CMD_SS( S2Port * apPort, const speed_t acBitrate )
{
    //! Send & Recive Buffer
    char buf[SCIP2_MAX_LENGTH];
    //! return value of function
    int ret;

    sprintf( buf, "SS%06d", bitrate2i( acBitrate ) );
    ret = Scip2_Send( apPort, buf );
    if( !Scip2_RecvTerm( apPort ) )
        return 0;
    if( ret == 0 || ret == 3 )
    {
        if( !Scip2_ChangeBitrate( apPort, acBitrate ) )
            return 0;
        return 1;
    }
    return 0;
}



/*--------------------------------------------------------------*/
/**
 * @brief Laser ON
 * @param *apPort Pointer to SCIP2.0 Device Port
 * @return failed: false, succeeded: true
 */
/*--------------------------------------------------------------*/
int Scip2CMD_BM( S2Port * apPort )
{
    //! return value of function
    int ret;
    //! count trying
    int ntry;

    //! BM command fail, if URG is not ready
    for ( ntry = 0; ntry < 3; ntry++ )
    {
        ret = Scip2_Send( apPort, "BM" );
        if( !Scip2_RecvTerm( apPort ) )
            return 0;

        if( ret == 0 || ret == 2 )
            return 1;

        Scip2_Flush( apPort );
        usleep( 100000 );
        Scip2_SendTerm( apPort );
        Scip2_Flush( apPort );
        usleep( 100000 );
    }
    return 0;
}



/*--------------------------------------------------------------*/
/**
 * @brief Laser OFF
 * @param *apPort Pointer to SCIP2.0 Device Port
 * @return failed: false, succeeded: true
 */
/*--------------------------------------------------------------*/
int Scip2CMD_QT( S2Port * apPort )
{
    //! Status Value
    int status;
    //! Buffer
    char buf[SCIP2_MAX_LENGTH];
    //! Return value of function
    int ret;

    strcpy( buf, "QT" );
    ret = fwrite( buf, strlen( buf ), sizeof ( char ), apPort );
    if( ret == 0 || Scip2_SendTerm( apPort ) == 0 )
    {
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Failed to send message.\n" );
#endif											/* SCIP2_DEBUG */
        return 0;
    }

    do{
        //! Strtok save ptr
        char *ptr;

        if( fgets( buf, SCIP2_MAX_LENGTH, apPort ) == NULL )
        {
            return 0;
        }
        strtok_r( buf, "\n", &ptr );
    }
    while( strcmp( buf, "QT" ) != 0 );

    status = Scip2_RecvStatus( apPort );
    if( !Scip2_RecvTerm( apPort ) )
        return 0;

    if( status == 0 || status == 2 )
        return 1;
    return 0;
}



/*--------------------------------------------------------------*/
/**
 * @brief Start getting scanned data
 * @param *apPort Pointer to SCIP2.0 Device Port
 * @param aStart Start step
 * @param aEnd End step
 * @param aGroup Number of group
 * @param *aData Pointer to buffer structure
 * @param acEnc Encode type
 * @return failed: false, succeeded: true
 * @attention Scip2CMD_StopGS must be called before calling another Scip2CMD function,
 *            if the device remains sending data to PC!!
 */
/*--------------------------------------------------------------*/
int Scip2CMD_GS( S2Port * apPort, int aStart, int aEnd, int aGroup, S2Sdd_t * aData, const S2EncType acEnc )
{
    //! Buffer to write
    S2Scan_t *scan;

    switch ( acEnc )
    {
    case SCIP2_ENC_2BYTE:
    case SCIP2_ENC_3BYTE:
    case SCIP2_ENC_3X2BYTE:
        break;
    default:
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Unsupported encording type selected.\n" );
#endif											/* SCIP2_DEBUG */
        return 0;
        break;
    }

    if( aGroup == 0 )
        aGroup = 1;
    pthread_mutex_lock( &( aData->mutexw ) );
    scan = aData->sec;
    aData->nbuf = 2;
    pthread_mutex_unlock( &( aData->mutexw ) );
    pthread_mutex_lock( &( scan->mutex ) );
    scan->start = aStart;
    scan->end = aEnd;
    scan->group = aGroup;
    scan->port = apPort;
    scan->enc = acEnc;
    pthread_mutex_unlock( &( scan->mutex ) );

    pthread_create( &( aData->thread ), NULL, S2Sdd_RecvData, ( void * )aData );

    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief Stop GS scanning
 * @param *apPort Pointer to SCIP2.0 Device Port
 * @param *aData Pointer to buffer structure
 * @return failed: false, succeeded: true
 */
/*--------------------------------------------------------------*/
int Scip2CMD_StopGS( S2Port * apPort, S2Sdd_t * aData )
{
    if( aData->thread )
    {
        pthread_join( aData->thread, NULL );
        aData->thread = 0;
    }
    return 1;
}



/*--------------------------------------------------------------*/
/**
  @brief Start getting scanned data
  @param *apPort Pointer to SCIP2.0 Device Port
  @param aStart Start step
  @param aEnd End step
  @param aGroup Number of group
  @param aCull Culling clearance
  @param aNum Number of scan
  @param *aData Pointer to buffer structure
  @param acEnc Encode type
  @return failed: false, succeeded: true
  @attention Scip2CMD_StopMS must be called before calling another Scip2CMD function,
             if the device remains sending data to PC!!
 */
/*--------------------------------------------------------------*/
int
Scip2CMD_StartMS( S2Port * apPort, int aStart, int aEnd, int aGroup,
                  int aCull, int aNum, S2Sdd_t * aData, const S2EncType acEnc )
{
    //! Command Buffer
    char mes[SCIP2_MAX_LENGTH];
    //! return value of function
    int ret;

    switch ( acEnc )
    {
    case SCIP2_ENC_2BYTE:
    case SCIP2_ENC_3BYTE:
    case SCIP2_ENC_3X2BYTE:
        break;
    default:
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Unsupported encording type selected.\n" );
#endif											/* SCIP2_DEBUG */
        return 0;
        break;
    }

    if( aGroup == 0 )
        aGroup = 1;
    aData->nbuf = 3;
    aData->thr->start = aData->sec->start = aData->pri->start = aStart;
    aData->thr->end = aData->sec->end = aData->pri->end = aEnd;
    aData->thr->group = aData->sec->group = aData->pri->group = aGroup;
    aData->thr->cull = aData->sec->cull = aData->pri->cull = aCull;
    aData->thr->enc = aData->sec->enc = aData->pri->enc = acEnc;
    aData->thr->port = aData->sec->port = aData->pri->port = apPort;
    aData->thr->num = aData->sec->num = aData->pri->num = aNum;
    aData->thr->memsize = aData->sec->memsize = aData->pri->memsize = 0;

    switch ( acEnc )
    {
    case SCIP2_ENC_2BYTE:
        sprintf( mes, "MS%04d%04d%02d%d%02d", aStart, aEnd, aGroup, aCull, aNum );
        break;
    case SCIP2_ENC_3BYTE:
        sprintf( mes, "MD%04d%04d%02d%d%02d", aStart, aEnd, aGroup, aCull, aNum );
        break;
    case SCIP2_ENC_3X2BYTE:
        sprintf( mes, "ME%04d%04d%02d%d%02d", aStart, aEnd, aGroup, aCull, aNum );
        break;
    default:
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Unsupported encording type selected.\n" );
#endif											/* SCIP2_DEBUG */
        return 0;
    }
    ret = Scip2_Send( apPort, mes );
    if( !Scip2_RecvTerm( apPort ) )
        return 0;
    if( ret != 0 )
        return 0;

    pthread_create( &( aData->thread ), NULL, S2Sdd_RecvDataCont, ( void * )aData );

    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief Stop MS scanning
 * @param *apPort Pointer to SCIP2.0 Device Port
 * @param *aData Pointer to buffer structure
 * @return failed: false, succeeded: true
 */
/*--------------------------------------------------------------*/
int Scip2CMD_StopMS( S2Port * apPort, S2Sdd_t * aData )
{
    S2Sdd_StopThread( aData );
    return Scip2CMD_QT( apPort );
}



/*--------------------------------------------------------------*/
/**
 * @brief Get time of device
 * @param *apPort Pointer to SCIP2.0 Device Port
 * @param *apTime Start time of device
 * @return failed: 0, succeeded: 1
 */
/*--------------------------------------------------------------*/
int Scip2CMD_TM_GetStartTime( S2Port * apPort, struct timeval *apTime )
{
    //! Return value of function
    int ret;
    //! time of device
    unsigned long dtime;
    //! Calculation of delay
    struct timeval tms, tme;
    //! Remains value of line
    unsigned long value;
    //! Number of remains value of line
    int nrem;

    ret = Scip2_Send( apPort, "TM0" );
    if( !Scip2_RecvTerm( apPort ) )
        return 0;
    if( ret != 0 && ret != 2 )
    {
        return 0;
    }

    gettimeofday( &tms, NULL );
    ret = Scip2_Send( apPort, "TM1" );
    if( ret != 0 )
    {
        Scip2_RecvTerm( apPort );
        return 0;
    }
    value = 0;
    nrem = 0;
    ret = Scip2_RecvEncodedLine( apPort, &dtime, 1, SCIP2_ENC_4BYTE, &value, &nrem );
    if( !Scip2_RecvTerm( apPort ) )
        return 0;

    tme.tv_sec = dtime / 1000;
    tme.tv_usec = ( dtime * 1000 ) % 1000000;
    timersub( &tms, &tme, apTime );

    ret = Scip2_Send( apPort, "TM2" );
    if( !Scip2_RecvTerm( apPort ) )
        return 0;
    if( ret != 0 && ret != 3 )
    {
        return 0;
    }

    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief Get time of device
 * @param *apPort Pointer to SCIP2.0 Device Port
 * @param *adClock device clock
 * @param *ahTime pc clock time
 * @return failed: 0, succeeded: 1
 */
/*--------------------------------------------------------------*/
int Scip2CMD_TM_GetSyncTime( S2Port * apPort, unsigned long *adClock, struct timeval *apTime )
{
    //! Return value of function
    int ret;
    //! time of device
    unsigned long dtime;
    //! Time of send command
    struct timeval tms;
    //! Time of receive command
    struct timeval tmr;
    //! Subtraction send time from recive time
    struct timeval sub;
    //! (double)Subtraction send time from recive time
    double dsub;
    //! Subtraction send time from recive time (second)
    long sub_sec;
    //! Remains value of line
    unsigned long value;
    //! Number of remains value of line
    int nrem;

    ret = Scip2_Send( apPort, "TM0" );
    if( !Scip2_RecvTerm( apPort ) )
        return 0;
    if( ret != 0 && ret != 2 )
    {
        return 0;
    }

    gettimeofday( &tms, NULL );
    ret = Scip2_Send( apPort, "TM1" );
    if( ret != 0 )
    {
        Scip2_RecvTerm( apPort );
        return 0;
    }
    value = 0;
    nrem = 0;

    ret = Scip2_RecvEncodedLine( apPort, &dtime, 1, SCIP2_ENC_4BYTE, &value, &nrem );
    if( !Scip2_RecvTerm( apPort ) )
        return 0;
    gettimeofday( &tmr, NULL );

    timersub( &tmr, &tms, &sub );
    dsub = (sub.tv_sec / 2) +  ( (double)sub.tv_usec / (2 * 1000000) );
    sub_sec = (long int)dsub;
    apTime->tv_sec = tms.tv_sec + sub_sec;

    for(apTime->tv_usec = tms.tv_usec + (dsub - sub_sec) * 1000000;
        apTime->tv_usec >= 1000000;
        apTime->tv_sec++, apTime->tv_usec -= 1000000);

    *adClock = dtime;

    ret = Scip2_Send( apPort, "TM2" );
    if( !Scip2_RecvTerm( apPort ) )
        return 0;
    if( ret != 0 && ret != 3 )
    {
        return 0;
    }

    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief Set rotating speed
 * @param *apPort Pointer to SCIP2.0 Device Port
 * @param acDeboost Deboost rate
 * @return failed: 0, succeeded: time of device
 */
/*--------------------------------------------------------------*/
int Scip2CMD_CR( S2Port * apPort, const int acDeboost )
{
    //! Return value of function
    int ret;
    //! Send & Recive Buffer
    char buf[SCIP2_MAX_LENGTH];

    sprintf( buf, "CR%02d", acDeboost );
    ret = Scip2_Send( apPort, buf );
    if( !Scip2_RecvTerm( apPort ) )
        return 0;
    if( ret == 0 || ret == 3 )
    {
        return 1;
    }
    return 0;
}



/*--------------------------------------------------------------*/
/**
 * @brief Get version info
 * @param *apPort Pointer to SCIP2.0 Device Port
 * @param *apVer Pointer to version structure
 * @return failed: 0, succeeded: time of device
 */
/*--------------------------------------------------------------*/
int Scip2CMD_VV( S2Port * apPort, S2Ver_t * apVer )
{
    //! Return value of function
    int ret;
    //! Send & Recive Buffer
    char buf[SCIP2_MAX_LENGTH];

    ret = Scip2_Send( apPort, "VV" );
    while( 1 )
    {
        //! Return value of function
        void *ret;
        //! Temporary
        char *tmp;

        ret = fgets( buf, SCIP2_MAX_LENGTH, apPort );

        if( ret == NULL || buf[0] == '\n' )
            break;

        tmp = strchr( buf, ';' );
        if( tmp == NULL )
            continue;

        *tmp = 0;
        if( strstr( buf, "VEND:" ) == buf )
        {
            strcpy( apVer->vender, buf + 5 );
        }
        else if( strstr( buf, "PROD:" ) == buf )
        {
            strcpy( apVer->product, buf + 5 );
        }
        else if( strstr( buf, "FIRM:" ) == buf )
        {
            strcpy( apVer->firmware, buf + 5 );
        }
        else if( strstr( buf, "PROT:" ) == buf )
        {
            strcpy( apVer->protocol, buf + 5 );
        }
        else if( strstr( buf, "SERI:" ) == buf )
        {
            strcpy( apVer->serialno, buf + 5 );
        }
    }
    if( ret == 0 )
    {
        return 1;
    }
    return 0;
}



/*--------------------------------------------------------------*/
/**
 * @brief Get param info
 * @param *apPort Pointer to SCIP2.0 Device Port
 * @param *apParam Pointer to param structure
 * @return failed: 0, succeeded: time of device
 */
/*--------------------------------------------------------------*/
int Scip2CMD_PP( S2Port * apPort, S2Param_t * apParam )
{
    //! Return value of function
    int ret;
    //! Send & Recive Buffer
    char buf[SCIP2_MAX_LENGTH];

    ret = Scip2_Send( apPort, "PP" );
    while( 1 )
    {
        //! Return value of function
        void *ret;
        //! Temporary
        char *tmp;

        ret = fgets( buf, SCIP2_MAX_LENGTH, apPort );

        if( ret == NULL || buf[0] == '\n' )
            break;

        tmp = strchr( buf, ';' );
        if( tmp == NULL )
            continue;

        *tmp = 0;
        if( strstr( buf, "MODL:" ) == buf )
        {
            strcpy( apParam->model, buf + 5 );
        }
        else if( strstr( buf, "DMIN:" ) == buf )
        {
            apParam->dist_min = atoi( buf + 5 );
        }
        else if( strstr( buf, "DMAX:" ) == buf )
        {
            apParam->dist_max = atoi( buf + 5 );
        }
        else if( strstr( buf, "ARES:" ) == buf )
        {
            apParam->step_resolution = atoi( buf + 5 );
        }
        else if( strstr( buf, "AMIN:" ) == buf )
        {
            apParam->step_min = atoi( buf + 5 );
        }
        else if( strstr( buf, "AMAX:" ) == buf )
        {
            apParam->step_max = atoi( buf + 5 );
        }
        else if( strstr( buf, "AFRT:" ) == buf )
        {
            apParam->step_front = atoi( buf + 5 );
        }
        else if( strstr( buf, "SCAN:" ) == buf )
        {
            apParam->revolution = atoi( buf + 5 );
        }
    }
    if( ret == 0 )
    {
        return 1;
    }
    return 0;
}



/*--------------------------------------------------------------*/
/**
 * @brief Reset Device
 * @param *apPort Pointer to SCIP2.0 Device Port
 * @return failed: false, succeeded: true
 */
/*--------------------------------------------------------------*/
int Scip2CMD_RS( S2Port * apPort )
{
    //! Status Value
    int status;
    //! Buffer
    char buf[SCIP2_MAX_LENGTH];
    //! Return value of function
    int ret;

    strcpy( buf, "RS" );
    ret = fwrite( buf, strlen( buf ), sizeof ( char ), apPort );
    if( ret == 0 || Scip2_SendTerm( apPort ) == 0 )
    {
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Failed to send message.\n" );
#endif											/* SCIP2_DEBUG */
        return 0;
    }

    do
    {
        //! Strtok save ptr
        char *ptr;

        if( fgets( buf, SCIP2_MAX_LENGTH, apPort ) == NULL )
        {
            return 0;
        }
        strtok_r( buf, "\n", &ptr );
    }
    while( strcmp( buf, "RS" ) != 0 );

    status = Scip2_RecvStatus( apPort );
    if( !Scip2_RecvTerm( apPort ) )
        return 0;

    if( status == 0 || status == 2 )
        return 1;
    return 0;
}



/*--------------------------------------------------------------*/
/**
 * @brief Start getting scanned data
 * @param *apPort Pointer to SCIP2.0 Device Port
 * @param aStart Start step
 * @param aEnd End step
 * @param aGroup Number of group
 * @param aCull Culling clearance
 * @param aNum Number of scan
 * @param *aData Pointer to buffer structure
 * @param acEnc Encode type
 * @return failed: false, succeeded: true
 * @attention Scip2CMD_StopND must be called before calling another Scip2CMD function,
 *            if the device remains sending data to PC!!
 */
/*--------------------------------------------------------------*/
int
Scip2CMD_StartND( S2Port * apPort, int aStart, int aEnd, int aGroup,
                  int aCull, int aNum, S2Sdd_t * aData, const S2EncType acEnc )
{
    //! Command Buffer
    char mes[SCIP2_MAX_LENGTH];
    //! return value of function
    int ret;

    switch ( acEnc )
    {
    case SCIP2_ENC_3BYTE:
    case SCIP2_ENC_3X2BYTE:
        break;
    default:
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Unsupported encording type selected.\n" );
#endif											/* SCIP2_DEBUG */
        return 0;
        break;
    }

    if( aGroup == 0 )
        aGroup = 1;
    aData->nbuf = 3;
    aData->thr->start = aData->sec->start = aData->pri->start = aStart;
    aData->thr->end = aData->sec->end = aData->pri->end = aEnd;
    aData->thr->group = aData->sec->group = aData->pri->group = aGroup;
    aData->thr->cull = aData->sec->cull = aData->pri->cull = aCull;
    aData->thr->enc = aData->sec->enc = aData->pri->enc = acEnc;
    aData->thr->port = aData->sec->port = aData->pri->port = apPort;
    aData->thr->num = aData->sec->num = aData->pri->num = aNum;
    aData->thr->memsize = aData->sec->memsize = aData->pri->memsize = 0;

    switch ( acEnc )
    {
    case SCIP2_ENC_3BYTE:
        sprintf( mes, "ND%04d%04d%02d%d%02d", aStart, aEnd, aGroup, aCull, aNum );
        break;
    case SCIP2_ENC_3X2BYTE:
        sprintf( mes, "NE%04d%04d%02d%d%02d", aStart, aEnd, aGroup, aCull, aNum );
        break;
    default:
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Unsupported encording type selected.\n" );
#endif											/* SCIP2_DEBUG */
        return 0;
    }
    ret = Scip2_Send( apPort, mes );
    if( !Scip2_RecvTerm( apPort ) )
        return 0;
    if( ret != 0 )
        return 0;

    pthread_create( &( aData->thread ), NULL, S2Sdd_RecvDataCont, ( void * )aData );

    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief Stop ND scanning
 * @param *apPort Pointer to SCIP2.0 Device Port
 * @param *aData Pointer to buffer structure
 * @return failed: false, succeeded: true
 */
/*--------------------------------------------------------------*/
int Scip2CMD_StopND( S2Port * apPort, S2Sdd_t * aData )
{
    S2Sdd_StopThread( aData );
    return Scip2CMD_QT( apPort );
}
