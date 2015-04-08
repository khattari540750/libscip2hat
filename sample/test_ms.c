
/**
  @file libscip2hat_test.c
  @brief Test program for libscip2
  @author Atsushi WATANEBE <atusi_w@roboken.esys.tsukuba.ac.jp> <atusi_w@doramanjyu.com>
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

#include "scip2hat.h"

// #define USE_GS

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
	/* Pointer to data buffer */
	S2Scan_t *data;
	/* Loop valiant */
	int j;
	/* Returned value */
	int ret;
	/* Local time */
	struct timeval tm;

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

	/* Demand sending me scanned data */
	/* Data will be recived in another thread */
	/* MS command */
	Scip2CMD_StartMS( port, 44, 725, 1, 0, 0, &buf, SCIP2_ENC_2BYTE );

	while( !gShutoff )
	{
		/* Start using scanned data */
		ret = S2Sdd_Begin( &buf, &data );
		/* Returned value is 0 when buffer is having been used now */
		if( ret > 0 )
		{
			/* You can start reciving next data after S2Sdd_Begin succeeded */
			gettimeofday( &tm, NULL );
			/* then you can analyze data in parallel with reciving next data */

			/* ---- analyze data ---- */
			usleep( 90000 );

			for ( j = 0; j < data->size; j++ )
			{
				// printf( "%d, ", (int)data->data[j] );
			}
			gettimeofday( &tm, NULL );
			printf( "[P%d][U%d] %d\n",
					( int )( ( ( tm.tv_sec * 1000 ) & 0x7FFFFFFF ) +
							 tm.tv_usec / 1000 ), ( int )data->time, ( int )data->data[data->size / 2] );

			/* Don't forget S2Sdd_End to unlock buffer */
			S2Sdd_End( &buf );
		}
		else if( ret == -1 )
		{
			fprintf( stderr, "ERROR: Fatal error occurred.\n" );
			break;
		}
		else
		{
			usleep( 100 );
		}
	}
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
