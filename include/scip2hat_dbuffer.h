
/**
  @file libscip2hat_dbuffer.h
  @brief Library for Sokuiki-Sensor "URG"
  @author Atsushi WATANABE <atusi_w[at]roboken.esys.tsukuba.ac.jp> 
 */

#ifndef __LIBscip2hat_DBUFFER_H__
#define __LIBscip2hat_DBUFFER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "scip2hat.h"

/** Buffer structure for scanned data */
	typedef struct SCIP2_SCANNED_DATA
	{
		int start;
		int end;
		int group;
		int cull;
		int size;
		int num;
		int memsize;
		unsigned long time;
		int error;
		pthread_mutex_t mutex;
		S2Port *port;
		unsigned long *data;
		S2EncType enc;
	} S2Scan_t;

/** Multi buffer structure for scanned data */
	typedef struct SCIP2_SCANNED_DATA_TRI
	{
		S2Scan_t *pri;
		S2Scan_t *sec;
		S2Scan_t *thr;
		S2Scan_t buf[3];
		int update;
		pthread_mutex_t mutexr;
		pthread_mutex_t mutexw;
		pthread_t thread;
		int nbuf;
		int ( *callback ) ( S2Scan_t *, void * );
		void *userdata;
	} S2Sdd_t;

/** user's function */
	void S2Sdd_Init( S2Sdd_t * aData );
	void S2Sdd_Dest( S2Sdd_t * aData );
	void S2Sdd_setCallback( S2Sdd_t * aData, int ( *aCallback ) ( S2Scan_t *, void * ), void *aUserdata );
	int S2Sdd_IsError( S2Sdd_t * aData );

	void S2Sdd_End( S2Sdd_t * aData );
	int S2Sdd_Begin( S2Sdd_t * aData, S2Scan_t ** aScan );

/** program function */
	void *S2Sdd_RecvData( void *aArg );
	void *S2Sdd_RecvDataCont( void *aArg );
	void S2Sdd_StopThread( S2Sdd_t * aData );

#ifdef __cplusplus
}
#endif

#endif											/* __LIBscip2hat_DBUFFER_H__ */
