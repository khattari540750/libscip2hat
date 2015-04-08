
/**
  @file test-ms-callback.c
  @brief  Library for Sokuiki-Sensor "URG"
  @author HATTORI Kohei <hattori[at]team-lab.com>
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

#include "scip2hat.h"

/** Flag */
int gShutoff;

/**
  @brief Ctrl+C trap
  @param aN not used
 */
void ctrlc( int aN )
{
    /* It is recommended to stop URG outputting data. */
    /* or cost much time to start communication next time. */
    gShutoff = 1;
    signal( SIGINT, NULL );
}

/**
  @brief callback function
  @param *aScan Scan data
  @param *aData User's data
  @retval 1 success to analyze scan data.
  @retval 0 failed to analyze scan data and stop thread of duall buffer.
 */
int callback( S2Scan_t * aScan, void *aUser )
{
    /* Loop valiant */
    int j;
    /* Local time */
    struct timeval tm;
    gettimeofday( &tm, NULL );
    /* then you can analyze data in parallel with reciving next data */

    /* ---- analyze data ---- */
    for ( j = 0; j < aScan->size; j++ )
    {
        // printf( "%d, ", (int)aScan->data[j] );
    }
    gettimeofday( &tm, NULL );
    printf( "[P%d][U%d] %d\n",
            ( int )( ( ( tm.tv_sec * 1000 ) & 0x7FFFFFFF ) + tm.tv_usec / 1000 ),
            ( int )aScan->time, ( int )aScan->data[aScan->size / 2] );
    return 1;
}

/**
  @brief Main function
  @param aArgc Number of Arguments
  @param appArgv Arguments
  @return failed: 0, succeeded: 1
 */
int main( int aArgc, char **appArgv )
{
    /* Device Port */
    S2Port *port;
    /* Data recive dual buffer */
    S2Sdd_t buf;
    /* Returned value */
    int ret;

    if( aArgc != 2 )
    {
        fprintf( stderr, "USAGE: %s device\n", appArgv[0] );
        return 0;
    }
    /* Start trapping ctrl+c signal */
    gShutoff = 0;
    signal( SIGINT, ctrlc );

    /* Open the port */
    port = Scip2_Open( appArgv[1], B115200 );
    if( port == 0 )
    {
        fprintf( stderr, "ERROR: Failed to open device.\n" );
        return 0;
    }
    printf( "Port opened\n" );

    /* Initialize buffer before getting scanned data */
    S2Sdd_Init( &buf );
    printf( "Buffer initialized\n" );

    /* Set callback function to analyze scan data */
    S2Sdd_setCallback( &buf, callback, NULL );

    /* Demand sending me scanned data */
    /* Data will be recived in another thread */
    /* MS command */
    Scip2CMD_StartMS( port, 44, 725, 1, 0, 0, &buf, SCIP2_ENC_2BYTE );

    /* wait for singnal SIGINT */
    printf( "input 'Ctrl-C' for exit.\n" );
    pause(  );

    printf( "\nStopping\n" );

    ret = Scip2CMD_StopMS( port, &buf );
    if( ret == 0 )
    {
        fprintf( stderr, "ERROR: StopMS failed.\n" );
        return 0;
    }

    printf( "Stopped\n" );

    /* Destruct buffer */
    S2Sdd_Dest( &buf );
    printf( "Buffer destructed\n" );

    /* Close port */
    Scip2_Close( port );
    printf( "Port closed\n" );

    return 1;
}
