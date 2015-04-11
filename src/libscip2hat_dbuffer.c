/****************************************************************/
/**
  @file   libscip2hat_dbuffer.c
  @brief  Library for Sokuiki-Sensor "URG"
  @author HATTORI Kohei <hattori[at]team-lab.com>
 */
/****************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "scip2hat.h"



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Initialize dual buffer structure
  @param *aData Pointer to dual buffer structure
 */
/*--------------------------------------------------------------*/
void S2Sdd_Init( S2Sdd_t * aData )
{
    aData->thread = 0;
    aData->pri = &( aData->buf[0] );
    aData->pri->size = 0;
    aData->pri->data = 0;
    aData->pri->error = 0;
    aData->pri->memsize = 0;
    aData->pri->data = 0;
    aData->sec = &( aData->buf[1] );
    aData->sec->size = 0;
    aData->sec->data = 0;
    aData->sec->error = 0;
    aData->sec->memsize = 0;
    aData->sec->data = 0;
    aData->thr = &( aData->buf[2] );
    aData->thr->size = 0;
    aData->thr->data = 0;
    aData->thr->error = 0;
    aData->thr->memsize = 0;
    aData->thr->data = 0;
    pthread_mutex_init( &( aData->pri->mutex ), 0 );
    pthread_mutex_init( &( aData->sec->mutex ), 0 );
    pthread_mutex_init( &( aData->thr->mutex ), 0 );
    pthread_mutex_init( &( aData->mutexr ), 0 );
    pthread_mutex_init( &( aData->mutexw ), 0 );
    aData->update = 0;
    aData->callback = NULL;
    aData->userdata = NULL;
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Destruct dual buffer structure
  @param *aData Pointer to dual buffer structure
 */
/*--------------------------------------------------------------*/
void S2Sdd_Dest( S2Sdd_t * aData )
{
    S2Sdd_StopThread( aData );

    pthread_mutex_lock( &( aData->mutexw ) );
    pthread_mutex_lock( &( aData->sec->mutex ) );
    pthread_mutex_unlock( &( aData->sec->mutex ) );
    pthread_mutex_unlock( &( aData->mutexw ) );

    pthread_mutex_destroy( &( aData->pri->mutex ) );
    pthread_mutex_destroy( &( aData->sec->mutex ) );
    pthread_mutex_destroy( &( aData->thr->mutex ) );
    if( aData->pri->data != 0 )
        free( aData->pri->data );
    if( aData->sec->data != 0 )
        free( aData->sec->data );
    if( aData->thr->data != 0 )
        free( aData->thr->data );
    pthread_mutex_destroy( &( aData->mutexr ) );
    pthread_mutex_destroy( &( aData->mutexw ) );
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Set callback function for analyze scan data
  @param *aData Pointer to dual buffer structure
  @param *aCallback Pointer to callback function.
         This callback function return value 1 at success, and retrun value 0 at fail.
         If callback function returns value 0, dual buffer thread will stop.
  @param *aUserdata Pointer of argument for callback function
 */
/*--------------------------------------------------------------*/
void S2Sdd_setCallback( S2Sdd_t * aData, int ( *aCallback ) ( S2Scan_t *, void * ), void *aUserdata )
{
    pthread_mutex_lock( &( aData->mutexw ) );
    aData->callback = aCallback;
    aData->userdata = aUserdata;
    pthread_mutex_unlock( &( aData->mutexw ) );
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief check data is error
  @retval 1 error
  @retvak 0 normal
 */
/*--------------------------------------------------------------*/
int S2Sdd_IsError( S2Sdd_t * aData )
{
    pthread_mutex_lock( &( aData->mutexw ) );
    if( aData->sec->error || aData->pri->error || aData->thr->error )
    {
        pthread_mutex_unlock( &( aData->mutexw ) );
        return 1;
    }
    pthread_mutex_unlock( &( aData->mutexw ) );
    return 0;
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Start using Data ( Non-Blocking )
  @param *aData Pointer to dual buffer structure
  @param **aScan Pointer to buffer structure handle
  @return failed: NULL, fatal error: -1, succeeded: Pointer to front buffer
 */
/*--------------------------------------------------------------*/
int S2Sdd_Begin( S2Sdd_t * aData, S2Scan_t ** aScan )
{
    if( pthread_mutex_trylock( &( aData->mutexr ) ) != 0 )
    {
        return 0;
    }
    pthread_mutex_lock( &( aData->mutexw ) );
    if( aData->sec->error || aData->pri->error || aData->thr->error )
    {
        pthread_mutex_unlock( &( aData->mutexw ) );
        pthread_mutex_unlock( &( aData->mutexr ) );
        return -1;
    }
    if( aData->update )
    {
        aData->update = 0;
        switch ( aData->nbuf )
        {
        case 2:
            *aScan = aData->pri;
            break;
        case 3:
            *aScan = aData->sec;
            aData->sec = aData->pri;
            aData->pri = *aScan;
            break;
        default:
            pthread_mutex_unlock( &( aData->mutexw ) );
            pthread_mutex_unlock( &( aData->mutexr ) );
            return 0;
        }
        pthread_mutex_unlock( &( aData->mutexw ) );
        return 1;
    }
    pthread_mutex_unlock( &( aData->mutexw ) );
    pthread_mutex_unlock( &( aData->mutexr ) );
    return 0;
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief End using Data
  @param *aData Pointer to dual buffer structure
 */
/*--------------------------------------------------------------*/
void S2Sdd_End( S2Sdd_t * aData )
{
    pthread_mutex_unlock( &( aData->mutexr ) );
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Stop thread which reads data of MS/GS command
  @param *aData Pointer to buffer structure
 */
/*--------------------------------------------------------------*/
void S2Sdd_StopThread( S2Sdd_t * aData )
{
    if( aData->thread )
    {
        if( pthread_cancel( aData->thread ) == 0 )
        {
            pthread_join( aData->thread, NULL );
        }
        aData->thread = 0;
    }
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Get Scanned data
  @param *aArg Pointer to dual buffer structure
 */
/*--------------------------------------------------------------*/
void *S2Sdd_RecvData( void *aArg )
{
    //! Pointer to dual buffer structure
    S2Sdd_t *data;
    //! Pointer to front buffer
    S2Scan_t *scan;
    //! Pointer to Buffer
    unsigned long *pos;
    //! Returned value
    int ret;
    //! Send & Recive Buffer
    char buf[SCIP2_MAX_LENGTH];
    //! Remains value of line
    unsigned long value;
    //! Number of remains value of line
    int nrem;

    pthread_setcanceltype( PTHREAD_CANCEL_DEFERRED, NULL );

    data = ( S2Sdd_t * ) aArg;

    pthread_mutex_lock( &( data->mutexw ) );
    scan = data->sec;
    pthread_mutex_unlock( &( data->mutexw ) );

    pthread_mutex_lock( &( scan->mutex ) );

    switch ( scan->enc )
    {
    case SCIP2_ENC_2BYTE:
        sprintf( buf, "GS%04d%04d%02d", scan->start, scan->end, scan->group );
        break;
    case SCIP2_ENC_3BYTE:
        sprintf( buf, "GD%04d%04d%02d", scan->start, scan->end, scan->group );
        break;
    case SCIP2_ENC_3X2BYTE:
        sprintf( buf, "GE%04d%04d%02d", scan->start, scan->end, scan->group );
        break;
    default:
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Unsupported encording type selected.\n" );
        fflush( stderr );
        fflush( stderr );
#endif											/* SCIP2_DEBUG */
        scan->error = 1;
        pthread_mutex_unlock( &( scan->mutex ) );
        pthread_testcancel(  );
        pthread_detach( data->thread );
        pthread_exit( NULL );
        break;
    }

    ret = Scip2_Send( scan->port, buf );
    if( ret != 0 )
    {
        scan->error = 2;
        pthread_mutex_unlock( &( scan->mutex ) );
        pthread_testcancel(  );
        pthread_detach( data->thread );
        pthread_exit( NULL );
    }

    value = 0;
    nrem = 0;
    //! Start reading
    ret = Scip2_RecvEncodedLine( scan->port, &scan->time, 1, SCIP2_ENC_4BYTE, &value, &nrem );
    if( ret != 1 )
    {
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Failed to recive time stamp.\n" );
        fflush( stderr );
        fflush( stderr );
#endif											/* SCIP2_DEBUG */
        scan->error = 1;
        pthread_mutex_unlock( &( scan->mutex ) );
        pthread_testcancel(  );
        pthread_detach( data->thread );
        pthread_exit( NULL );
    }
#ifdef SCIP2_DEBUG_ALL
    fprintf( stderr, "SCIP2 INFO: Reciving data at %d.\n", ( int )scan->time );
    fflush( stderr );
#endif											/* SCIP2_DEBUG_ALL */
    if( scan->memsize < ( scan->end - scan->start ) / scan->group + 1024 )
    {
        if( scan->data )
            free( scan->data );
        scan->memsize = ( scan->end - scan->start ) / scan->group + 1024;
        scan->data = ( unsigned long * )malloc( sizeof ( unsigned long ) * scan->memsize );
        if( scan->data == 0 )
        {
#ifdef SCIP2_DEBUG
            fprintf( stderr, "SCIP2 ERROR: malloc failed.\n" );
            fflush( stderr );
#endif											/* SCIP2_DEBUG */
            scan->memsize = -1;
            scan->error = 2;
            pthread_mutex_unlock( &( scan->mutex ) );
            pthread_testcancel(  );
            pthread_detach( data->thread );
            pthread_exit( NULL );
        }
    }
    value = 0;
    nrem = 0;
    pos = scan->data;
    while( ( ret =
             Scip2_RecvEncodedLine( scan->port, pos,
                                    scan->memsize - ( pos - scan->data ), scan->enc, &value, &nrem ) ) > 0 )
    {
        pos += ret;
    }

    if( ret == -1 )
    {
        scan->error = 1;
        pthread_mutex_unlock( &( scan->mutex ) );
        pthread_testcancel(  );
        pthread_detach( data->thread );
        pthread_exit( NULL );
    }
    scan->size = pos - scan->data;
#ifdef SCIP2_DEBUG_ALL
    fprintf( stderr, "SCIP2 INFO: %d steps recived.\n", scan->size );
    fflush( stderr );
#endif											/* SCIP2_DEBUG_ALL */

    pthread_mutex_unlock( &( scan->mutex ) );

    //! run callback function
    if( data->callback )
    {
        pthread_mutex_lock( &( scan->mutex ) );
        data->callback( scan, data->userdata );
        pthread_mutex_unlock( &( scan->mutex ) );
    }

    //! swap buffer
    pthread_mutex_lock( &( data->mutexw ) );
    data->sec = data->pri;
    data->pri = scan;
    data->update = 1;
    pthread_mutex_unlock( &( data->mutexw ) );

    pthread_testcancel(  );
    pthread_detach( data->thread );
    pthread_exit( NULL );
    return 0;
}



#if defined(SCIP2_DEBUG_ALL) || defined(SCIP2_OUTPUT_CONTDATA)
char errbuf[2][8192];
int nerrbuf;
char *perrbuf;
char *perrbuf2;
#endif											/* SCIP2_DEBUG_ALL */



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Recive scanned data continually
  @param *aArg Pointer to dual buffer structure
 */
/*--------------------------------------------------------------*/
void *S2Sdd_RecvDataCont( void *aArg )
{
    //! Pointer to dual buffer structure
    S2Sdd_t *data;
    //! Pointer to front buffer
    S2Scan_t *scan;
    //! Pointer to Buffer
    unsigned long *pos;
    //! Returned value
    void *ret;
    //! Send & Recive Buffer
    char buf[SCIP2_MAX_LENGTH];
    //! Command
    char mes[SCIP2_MAX_LENGTH];
    //! Strtok save ptr
    char *ptr;
    //! Remaining scan number
    char rem[3];
    //! Remaining scan number
    int remnum;
    //! Remains value of line
    unsigned long value;
    //! Number of remains value of line
    int nrem;
    //! Number of data in 1 step
    int multi;
    //! Returned status number
    int status;
    //! Number of encoded/decoded lines
    int nlines;

    int enc;
#if defined(SCIP2_DEBUG_ALL) || defined(SCIP2_OUTPUT_CONTDATA)
    int pid;
    pid = getpid(  );
#endif

    pthread_setcanceltype( PTHREAD_CANCEL_DEFERRED, NULL );

    data = ( S2Sdd_t * ) aArg;
#ifdef SCIP2_OUTPUT_CONTDATA
    nerrbuf = 0;
#endif

    pthread_mutex_lock( &( data->mutexw ) );
    scan = data->thr;
    multi = 1;
    enc = scan->enc;
    switch ( scan->enc )
    {
    case SCIP2_ENC_2BYTE:
        sprintf( mes, "MS%04d%04d%02d%d", scan->start, scan->end, scan->group, scan->cull );
        break;
    case SCIP2_ENC_3BYTE:
        sprintf( mes, "MD%04d%04d%02d%d", scan->start, scan->end, scan->group, scan->cull );
        break;
    case SCIP2_ENC_3X2BYTE:
        sprintf( mes, "ME%04d%04d%02d%d", scan->start, scan->end, scan->group, scan->cull );
        enc = SCIP2_ENC_3BYTE;
        multi = 2;
        break;
    default:
        scan->error = 2;
        pthread_mutex_unlock( &( data->mutexw ) );
        pthread_testcancel(  );
        pthread_detach( data->thread );
        pthread_exit( NULL );
        break;
    }
    pthread_mutex_unlock( &( data->mutexw ) );

    while( 1 )
    {
#ifdef SCIP2_OUTPUT_CONTDATA
        perrbuf = errbuf[nerrbuf];
        nerrbuf = ( nerrbuf + 1 ) & 1;
        perrbuf2 = errbuf[nerrbuf];
        *perrbuf = 0;
#endif

        pthread_mutex_lock( &( scan->mutex ) );

        if( scan->memsize < ( scan->end - scan->start + 1 ) * multi / scan->group + 1024 )
        {
            if( scan->data )
                free( scan->data );
            scan->memsize = ( int )( scan->end - scan->start + 1 ) * multi / scan->group + 1024;
            scan->data = ( unsigned long * )malloc( sizeof ( unsigned long ) * scan->memsize );
            if( scan->data == 0 )
            {
#ifdef SCIP2_DEBUG
                fprintf( stderr, "SCIP2 ERROR: malloc failed.\n" );
                fflush( stderr );
#endif											/* SCIP2_DEBUG */
                scan->memsize = -1;
                scan->error = 2;
                pthread_mutex_unlock( &( scan->mutex ) );
                pthread_testcancel(  );
                pthread_detach( data->thread );
                pthread_exit( NULL );
            }
        }

        pos = scan->data;
        buf[0] = 0;
        ret = fgets( buf, SCIP2_MAX_LENGTH, scan->port );
#ifdef SCIP2_OUTPUT_CONTDATA
        memcpy( perrbuf, buf, strlen( buf ) );
        perrbuf += strlen( buf );
        *perrbuf = 0;
#endif
        if( ret == NULL )
        {
#ifdef SCIP2_DEBUG
            fprintf( stderr, "SCIP2 ERROR: %d: Failed to read echo back message.\n", pid );
            fflush( stderr );
#endif											/* SCIP2_DEBUG */
#ifdef SCIP2_OUTPUT_CONTDATA
            fprintf( stderr,
                     "SCIP2 ERROR: %d: sz:%d,scaned:%d\n-------DUMP-------\n%s\n-----------------\n%s\n-----------------\n",
                     pid, ( int )scan->memsize, pos - scan->data, errbuf[( nerrbuf + 1 ) & 1], errbuf[nerrbuf] );
            fflush( stderr );
#endif
            scan->error = 2;
            pthread_mutex_unlock( &( scan->mutex ) );
            pthread_testcancel(  );
            pthread_detach( data->thread );
            pthread_exit( NULL );
        }
        strtok_r( buf, "\n", &ptr );
        if( strlen( buf ) < 15 )
        {
#ifdef SCIP2_DEBUG
            fprintf( stderr, "SCIP2 ERROR: %d:  Invalid echo back returns \"%s\" \"%s\".\n", pid, buf, mes );
            fflush( stderr );
#endif											/* SCIP2_DEBUG */
#ifdef SCIP2_OUTPUT_CONTDATA
            fprintf( stderr,
                     "SCIP2 ERROR: %d: sz:%d,scaned:%d\n-------DUMP-------\n%s\n-----------------\n%s\n-----------------\n",
                     pid, ( int )scan->memsize, pos - scan->data, errbuf[( nerrbuf + 1 ) & 1], errbuf[nerrbuf] );
            fflush( stderr );
#endif
            scan->error = 2;
            pthread_mutex_unlock( &( scan->mutex ) );
            pthread_testcancel(  );
            pthread_detach( data->thread );
            pthread_exit( NULL );
        }

        rem[0] = buf[13];
        rem[1] = buf[14];
        rem[2] = 0;
        remnum = atoi( rem );
        buf[13] = 0;

        if( strcmp( buf, mes ) != 0 )
        {
            buf[13] = rem[0];
#ifdef SCIP2_DEBUG
            fprintf( stderr, "SCIP2 ERROR: %d: Invalid echo back returns \"%s\" \"%s\".\n", pid, buf, mes );
            fflush( stderr );
#endif											/* SCIP2_DEBUG */
#ifdef SCIP2_OUTPUT_CONTDATA
            fprintf( stderr,
                     "SCIP2 ERROR: %d: sz:%d,scaned:%d\n-------DUMP-------\n%s\n-----------------\n%s\n-----------------\n",
                     pid, ( int )scan->memsize, pos - scan->data, errbuf[( nerrbuf + 1 ) & 1], errbuf[nerrbuf] );
            fflush( stderr );
#endif
            scan->error = 2;
            pthread_mutex_unlock( &( scan->mutex ) );
            pthread_testcancel(  );
            pthread_detach( data->thread );
            pthread_exit( NULL );
        }

        ret = fgets( buf, SCIP2_MAX_LENGTH, scan->port );
#ifdef SCIP2_OUTPUT_CONTDATA
        memcpy( perrbuf, buf, strlen( buf ) );
        perrbuf += strlen( buf );
        *perrbuf = 0;
#endif
        if( ret == NULL )
        {
#ifdef SCIP2_DEBUG
            fprintf( stderr, "SCIP2 ERROR: %d: Failed to read status.\n", pid );
            fflush( stderr );
#endif											/* SCIP2_DEBUG */
#ifdef SCIP2_OUTPUT_CONTDATA
            fprintf( stderr,
                     "SCIP2 ERROR: %d: sz:%d,scaned:%d\n-------DUMP-------\n%s\n-----------------\n%s\n-----------------\n",
                     pid, ( int )scan->memsize, pos - scan->data, errbuf[( nerrbuf + 1 ) & 1], errbuf[nerrbuf] );
            fflush( stderr );
#endif
            scan->error = 2;
            pthread_mutex_unlock( &( scan->mutex ) );
            pthread_testcancel(  );
            pthread_detach( data->thread );
            pthread_exit( NULL );
        }
#ifdef SCIP2_DEBUG_ALL
        fprintf( stderr, "D:%s", buf );
        fflush( stderr );
#endif											/* SCIP2_DEBUG_ALL */
        if( strlen( buf ) == 1 )
        {
            scan->error = 1;
            pthread_mutex_unlock( &( scan->mutex ) );
            pthread_testcancel(  );
            pthread_detach( data->thread );
            pthread_exit( NULL );
        }
        status = ( buf[0] - '0' ) * 10 + ( buf[1] - '0' );
        if( buf[0] < '0' || buf[1] < '0' || buf[0] > '9' || buf[1] > '9' )
        {
            status = -( int )( ( unsigned )buf[0] * 0x100 + ( unsigned )buf[1] );
            if( status < -( '0' * 0x100 + 'I' ) )
            {
                fprintf( stderr, "SCIP2 FATAL ERROR: Sensor is update mode (%d).\n", status );
                fflush( stderr );
                Scip2_Flush( scan->port );
                Scip2CMD_RS( scan->port );
                exit( 1 );
            }
        }
#ifdef SCIP2_ENABLE_CHECKSUM
        sum = ( buf[0] + buf[1] ) & 0x3F + 0x30;
        if( sum != buf[2] )
        {
#ifdef SCIP2_DEBUG
            fprintf( stderr, "SCIP2 ERROR: Checksum mismatch.\n" );
            fflush( stderr );
#endif											/* SCIP2_DEBUG */
            scan->error = 2;
            pthread_mutex_unlock( &( scan->mutex ) );
            pthread_testcancel(  );
            pthread_detach( data->thread );
            pthread_exit( NULL );
        }
#endif											/* SCIP2_ENABLE_CHECKSUM */
        if( status != 99 )
        {
#ifdef SCIP2_DEBUG
            fprintf( stderr, "SCIP2 ERROR: error status recived (%d).\n", status );
            fflush( stderr );
#endif											/* SCIP2_DEBUG */
            scan->error = 1;
            pthread_mutex_unlock( &( scan->mutex ) );
            pthread_testcancel(  );
            pthread_detach( data->thread );
            pthread_exit( NULL );
        }

        value = 0;
        nrem = 0;
        //! Start reading
        nlines = Scip2_RecvEncodedLine( scan->port, &scan->time, 1, SCIP2_ENC_4BYTE, &value, &nrem );
#ifdef SCIP2_OUTPUT_CONTDATA
        memcpy( perrbuf, scip2_debuf, strlen( scip2_debuf ) );
        perrbuf += strlen( scip2_debuf );
        *perrbuf = 0;
#endif

        if( nlines != 1 )
        {
#ifdef SCIP2_DEBUG
            fprintf( stderr, "SCIP2 ERROR: Failed to recive time stamp.\n" );
            fflush( stderr );
#endif											/* SCIP2_DEBUG */
#ifdef SCIP2_OUTPUT_CONTDATA
            fprintf( stderr,
                     "SCIP2 ERROR: %d: sz:%d,scaned:%d\n-------DUMP-------\n%s\n-----------------\n%s\n-----------------\n",
                     pid, ( int )scan->memsize, pos - scan->data, errbuf[( nerrbuf + 1 ) & 1], errbuf[nerrbuf] );
            fflush( stderr );
#endif
            scan->error = 1;
            pthread_mutex_unlock( &( scan->mutex ) );
            pthread_testcancel(  );
            pthread_detach( data->thread );
            pthread_exit( NULL );
        }
#ifdef SCIP2_DEBUG_ALL
        fprintf( stderr, "SCIP2 INFO: %d: Reciving data at %d.\n", pid, ( int )scan->time );
#endif											/* SCIP2_DEBUG_ALL */
        if( scan->memsize < ( scan->end - scan->start + 1 ) * multi / scan->group )
        {
            if( scan->data )
                free( scan->data );
            scan->memsize = ( scan->end - scan->start + 1 ) * multi / scan->group + 8;
            scan->data = ( unsigned long * )malloc( sizeof ( unsigned long ) * scan->memsize );
            if( scan->data == 0 )
            {
#ifdef SCIP2_DEBUG
                fprintf( stderr, "SCIP2 ERROR: malloc failed.\n" );
                fflush( stderr );
#endif											/* SCIP2_DEBUG */
                scan->memsize = -1;
                scan->error = 1;
                pthread_mutex_unlock( &( scan->mutex ) );
                pthread_testcancel(  );
                pthread_detach( data->thread );
                pthread_exit( NULL );
            }
        }
        value = 0;
        nrem = 0;
        pos = scan->data;

        while( ( nlines =
                 Scip2_RecvEncodedLine( scan->port, pos,
                                        scan->memsize - ( pos - scan->data ), enc, &value, &nrem ) ) > 0 )
        {

#ifdef SCIP2_OUTPUT_CONTDATA
            memcpy( perrbuf, scip2_debuf, strlen( scip2_debuf ) );
            perrbuf += strlen( scip2_debuf );
            *perrbuf = 0;
#endif
            pos += nlines;
        }
#ifdef SCIP2_OUTPUT_CONTDATA
        memcpy( perrbuf, scip2_debuf, strlen( scip2_debuf ) );
        perrbuf += strlen( scip2_debuf );
        *perrbuf = 0;
#endif
        if( nlines < 0 )
        {
#ifdef SCIP2_OUTPUT_CONTDATA
            fprintf( stderr,
                     "SCIP2 ERROR: %d: sz:%d,scaned:%d\n-------DUMP-------\n%s\n-----------------\n%s\n-----------------\n",
                     pid, ( int )scan->memsize, pos - scan->data, errbuf[( nerrbuf + 1 ) & 1], errbuf[nerrbuf] );
            fflush( stderr );
#endif
            scan->error = 1;
            pthread_mutex_unlock( &( scan->mutex ) );
            pthread_testcancel(  );
            pthread_detach( data->thread );
            pthread_exit( NULL );
        }
        scan->size = pos - scan->data;
#ifdef SCIP2_DEBUG_ALL
        fprintf( stderr, "SCIP2 INFO: %d: %d steps recived.\n", pid, scan->size );
#endif											/* SCIP2_DEBUG_ALL */
        pthread_mutex_unlock( &( scan->mutex ) );

        //! run callback function
        if( data->callback )
        {
            int ret;
            pthread_mutex_lock( &( scan->mutex ) );
            ret = data->callback( scan, data->userdata );
            pthread_mutex_unlock( &( scan->mutex ) );
            if( !ret )
                break;
        }

        //! swap buffer
        pthread_mutex_lock( &( data->mutexw ) );
        data->thr = data->sec;
        data->sec = scan;
        scan = data->thr;
        data->update = 1;
        pthread_mutex_unlock( &( data->mutexw ) );

        pthread_testcancel(  );

        //! Stop if remain number is 0
        if( remnum == 0 && scan->num != 0 )
        {
            break;
        }
    }
    pthread_testcancel(  );
    pthread_detach( data->thread );
    pthread_exit( NULL );
}
