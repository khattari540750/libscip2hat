/****************************************************************/
/**
  @file   libscip2hat.c
  @brief  Library for Sokuiki-Sensor "URG"
  @author HATTORI Kohei <hattori[at]team-lab.com>
  @date   2015/04/11
 */
/****************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/file.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "scip2hat.h"
#include "scip2hat_base.h"
#include "scip2hat_cmd.h"



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Convert S2Bitrate_e to integer
  @param acBitrate Bitrate
  @return Integer Notation Bitrate
 */
/*--------------------------------------------------------------*/
int bitrate2i( const speed_t acBitrate )
{
    if( acBitrate == B9600 )
        return 9600;
    if( acBitrate == B19200 )
        return 19200;
    if( acBitrate == B38400 )
        return 38400;
    if( acBitrate == B57600 )
        return 57600;
    if( acBitrate == B115200 )
        return 115200;
#ifdef B230400
    if( acBitrate == B230400 )
        return 230400;
#endif
#ifdef B460800
    if( acBitrate == B460800 )
        return 460800;
#endif
#ifdef B500000
    if( acBitrate == B500000 )
        return 500000;
#endif
    return 0;
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Send Terminal Charactor
  @param *apPort Pointer to SCIP2.0 Device Port
  @return false: failed, true: succeeded
 */
/*--------------------------------------------------------------*/
int Scip2_SendTerm( S2Port * apPort )
{
    //! return value of fwrite
    size_t ret;

    ret = fwrite( "\n", 1, sizeof ( char ), apPort );
    if( ret == 0 )
        return 0;

    return 1;
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Recive Terminal Charactor
  @param *apPort Pointer to SCIP2.0 Device Port
  @return false: failed, true: succeeded
 */
/*--------------------------------------------------------------*/
int Scip2_RecvTerm( S2Port * apPort )
{
    //! return value of fget
    void *ret;
    //! Recive Buffer
    char buf[SCIP2_MAX_LENGTH] = "\0";

    ret = fgets( buf, SCIP2_MAX_LENGTH, apPort );
    if( ret == NULL || buf[0] != '\n' ){
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Failed to read terminal character.\n" );
        fflush( stderr );
        fflush( stderr );
#endif				/* SCIP2_DEBUG */
        return 0;
    }
    return 1;
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Recieve SCIP2.0 Status value
  @param *apPort Pointer to SCIP2.0 Device Port
  @return ERROR: -1, SUCCESS: Status Value
 */
/*--------------------------------------------------------------*/
int Scip2_RecvStatus( S2Port * apPort )
{
    //! return value of fgets
    void *ret2;
    //! return value of function
    int s_ret;
    //! Recive Buffer
    char buf[SCIP2_MAX_LENGTH] = "\0";
#ifdef SCIP2_ENABLE_CHECKSUM
    //! Checksum
    char sum;
#endif											/* SCIP2_ENABLE_CHECKSUM */

    ret2 = fgets( buf, SCIP2_MAX_LENGTH, apPort );
    if( ret2 == NULL )
    {
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Failed to read status.\n" );
        fflush( stderr );
        fflush( stderr );
#endif											/* SCIP2_DEBUG */
        return -1;
    }
#ifdef SCIP2_DEBUG_ALL
    fprintf( stderr, "D:%s", buf );
    fflush( stderr );
#endif											/* SCIP2_DEBUG_ALL */
    if( buf[1] == '\n' )
    {
        s_ret = ( buf[0] - '0' );
    }
    else
    {
        s_ret = ( buf[0] - '0' ) * 10 + ( buf[1] - '0' );
    }
    if( buf[0] < '0' || buf[1] < '0' || buf[0] > '9' || buf[1] > '9' )
    {
        if( buf[1] == '\n' )
        {
            s_ret = -( int )( ( unsigned )buf[0] );
        }
        else
        {
            s_ret = -( int )( ( unsigned )buf[0] * 0x100 + ( unsigned )buf[1] );
        }
        if( s_ret < -( '0' * 0x100 + 'I' ) )
        {
            fprintf( stderr, "SCIP2 FATAL ERROR: Sensor is update mode (%d).\n", s_ret );
            fflush( stderr );
            Scip2_Flush( apPort );
            Scip2CMD_RS( apPort );
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
        return -1;
    }
#endif											/* SCIP2_ENABLE_CHECKSUM */

    return s_ret;
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Send SCIP2.0 Message
  @param *apPort Pointer to SCIP2.0 Device Port
  @param *apcMes Pointer to SCIP2.0 Message( without LF terminater )
  @return error: -1, otherwise: return value of device
 */
/*--------------------------------------------------------------*/
int Scip2_Send( S2Port * apPort, const char *apcMes )
{
    //! return value of fwrite
    size_t ret;
    //! return value of fgets
    void *ret2;
    //! return value of function
    int s_ret;
    //! Recive Buffer
    char buf[SCIP2_MAX_LENGTH] = "\0";
    //! Strtok save ptr
    char *ptr;

#ifdef SCIP2_DEBUG_ALL
    fprintf( stderr, "H:%s\n", apcMes );
#endif											/* SCIP2_DEBUG_ALL */
    ret = fwrite( apcMes, strlen( apcMes ), sizeof ( char ), apPort );
    if( Scip2_SendTerm( apPort ) == 0 )
    {
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Failed to send message.\n" );
        fflush( stderr );
#endif											/* SCIP2_DEBUG */
        return -1;
    }
    if( ret == 0 )
    {
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Failed to send message.\n" );
        fflush( stderr );
#endif											/* SCIP2_DEBUG */
        return -1;
    }

    buf[0] = 0;
    ret2 = fgets( buf, SCIP2_MAX_LENGTH, apPort );
    if( ret2 == NULL )
    {
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Failed to read echo back message.\n" );
        fflush( stderr );
#endif											/* SCIP2_DEBUG */
        return -1;
    }
#ifdef SCIP2_DEBUG_ALL
    fprintf( stderr, "D:%s", buf );
    fflush( stderr );
#endif											/* SCIP2_DEBUG_ALL */

    strtok_r( buf, "\n", &ptr );
    if( strcmp( buf, apcMes ) != 0 )
    {
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Invalid echo back returns.\n" );
        fflush( stderr );
#endif											/* SCIP2_DEBUG */
        return -1;
    }

    s_ret = Scip2_RecvStatus( apPort );

    return s_ret;
}

#if defined(SCIP2_DEBUG_ALL) || defined(SCIP2_OUTPUT_CONTDATA)
char scip2_debuf[SCIP2_MAX_LENGTH];
#endif



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Recive encoded data
  @param *apPort Pointer to SCIP2.0 Device Port
  @param *apBuf Pointer to Buffer
  @param *aNBuf Size of Buffer
  @param acEnc Encode type
  @param *apRemains Remaining value
  @param *apNRemains Number of remaining bytes
  @return failed: -1, succeeded: size of recived data
 */
/*--------------------------------------------------------------*/
int
Scip2_RecvEncodedLine( S2Port * apPort, unsigned long *apBuf, int aNBuf,
                       const S2EncType acEnc, unsigned long *apRemains, int *apNRemains )
{
    //! Scanning Pointer
    char *pos;
    //! Recive Buffer
    char buf[SCIP2_MAX_LENGTH];
    //! General
    int i, j;
    //! Decoded data
    unsigned long value;
    //! Decode mask
    unsigned long mask;
#ifdef SCIP2_ENABLE_CHECKSUM
    //! Check sum
    unsigned int sum = 0;
#endif											/* SCIP2_ENABLE_CHECKSUM */

    mask = 0xFFFFFFFF >> ( 32 - acEnc * 6 );

    if( fgets( buf, sizeof ( buf ), apPort ) == NULL )
        return -1;
    j = 0;
    i = *apNRemains;
    value = *apRemains;

#if defined(SCIP2_DEBUG_ALL) || defined(SCIP2_OUTPUT_CONTDATA)
    memcpy( scip2_debuf, buf, strlen( buf ) );
    scip2_debuf[strlen( buf )] = 0;
#endif
    if( buf[0] == '\n' )
        return 0;
    if( strlen( buf ) < 2 )
    {
        return -2;
    }

    for ( pos = buf; ( *( pos + 1 ) != '\n' ) && ( *pos ); pos++ )
    {
        value = value << 6;
        value |= ( *pos - 0x30 );
#ifdef SCIP2_ENABLE_CHECKSUM
        sum += *pos;
#endif											/* SCIP2_ENABLE_CHECKSUM */
        i++;
        if( i == acEnc )
        {
            i = 0;
            apBuf[j] = value & mask;
            j++;
            if( j > aNBuf )
            {
#ifdef SCIP2_DEBUG
                fprintf( stderr, "SCIP2 ERROR: Recive buffer over flow.\n" );
                fflush( stderr );
#endif											/* SCIP2_DEBUG */
                Scip2_SendTerm( apPort );
                return -1;
            }
        }
    }
    *apNRemains = i;
    *apRemains = value;

#ifdef SCIP2_ENABLE_CHECKSUM
    sum = sum & 0x3F + 0x30;
    pos--;
    if( sum != *pos )
        fprintf( stderr, "SCIP2 ERROR: Checksum mismatch.\n" );
    fflush( stderr );
#endif											/* SCIP2_ENABLE_CHECKSUM */

    return j;
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Flush buffer of port
  @param *apPort Pointer to SCIP2.0 Device Port
  @return failed: false, succeeded: true
 */
/*--------------------------------------------------------------*/
void Scip2_Flush( S2Port * apPort )
{
    //! File number of Port
    int fn;

    fn = fileno( apPort );

    //! Flash Input/Output Buffer
    tcflush( fn, TCIFLUSH );
    usleep( 5000 );
    Scip2_SendTerm( apPort );
    usleep( 5000 );
    tcflush( fn, TCIFLUSH );
    usleep( 5000 );
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Change Bitrate of Port
  @param *apPort Pointer to SCIP2.0 Device Port
  @param acBitrate Bitrate
  @return failed: false, succeeded: true
 */
/*--------------------------------------------------------------*/
int Scip2_ChangeBitrate( S2Port * apPort, const speed_t acBitrate )
{
    //! Parameter of terminal io
    struct termios term;
    //! Return value
    int ret;
    //! File number of Port
    int fn;

    fn = fileno( apPort );
    tcgetattr( fn, &term );

    cfsetospeed( &term, acBitrate );
    cfsetispeed( &term, acBitrate );

    //! cfmakeraw( &term );
    term.c_iflag &= ~( IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON );
    term.c_oflag &= ~OPOST;
    term.c_lflag &= ~( ECHO | ECHONL | ICANON | ISIG | IEXTEN );
    term.c_cflag &= ~( CSIZE | PARENB );
    term.c_cflag |= CS8;

    term.c_cflag &= ~CRTSCTS;					//! Don't control hardware flow
    term.c_cc[VTIME] = 6;						//! Set time out to 600 ms
    term.c_cc[VMIN] = 0;						//! Set minimum string length

    //! Apply setting
    ret = tcsetattr( fn, TCSANOW, &term );
    if( ret != 0 )
    {
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Failed to Change Bitrate.\n" );
        fflush( stderr );
#endif											/* SCIP2_DEBUG */
        return 0;
    }

    //! Flash Input/Output Buffer
    Scip2_Flush( apPort );

    return 1;
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Close SCIP2.0 Device Port
  @param *apPort Pointer to SCIP2.0 Device Port
  @return failed: false, succeeded: true
 */
/*--------------------------------------------------------------*/
int Scip2_Close( S2Port * apPort )
{
    Scip2CMD_RS( apPort );
    Scip2_SendTerm( apPort );
    Scip2_SendTerm( apPort );
    if( fclose( apPort ) == 0 )
        return 1;
    return 0;
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief Open SCIP2.0 Device Port
  @param acpDevice Pointer to Name of SCIP2.0 Device
  @param acBitrate Bitrate
  @return failed: NULL, succeeded: Pointer to Device Handle
 */
/*--------------------------------------------------------------*/
S2Port *Scip2_Open( const char *acpDevice, const speed_t acBitrate )
{
    //! Handle to the Device
    FILE *this;
    //! Bitrate List ( order in which tried )
    speed_t rates[] = {
        B19200,
        B115200,
        B9600,
        B38400,
        B57600,
    #ifdef B230400
        B230400,
    #endif
    #ifdef B460800
        B460800,
    #endif
    #ifdef B500000
        B500000,
    #endif
        B19200,
        B0
    };
    //! Bitrate
    speed_t *rate;
    //! Return value of function
    int ret;
    //! Loop valiant
    int i;

    for ( i = 0; i < 2; i++ )
    {

        this = fopen( acpDevice, "w+" );
        if( this == NULL )
        {
#ifdef SCIP2_DEBUG
            fprintf( stderr, "SCIP2 ERROR: Failed to Open '%s'.\n", acpDevice );
            fflush( stderr );
#endif											/* SCIP2_DEBUG */
            return NULL;
        }
        if( lockf( fileno( this ), F_TLOCK, 0 ) != 0 )
        {
            // if( flock( fileno(this), LOCK_EX | LOCK_NB ) != 0 ){
#ifdef SCIP2_DEBUG
            fprintf( stderr, "SCIP2 ERROR: '%s' Locked.\n", acpDevice );
            fflush( stderr );
#endif											/* SCIP2_DEBUG */
            fclose( this );
            return NULL;
        }

        Scip2_Flush( this );

        for ( rate = rates; *rate != B0; rate++ )
        {
#ifdef SCIP2_DEBUG
            fprintf( stderr, "Trying %d bps...\n", bitrate2i( *rate ) );
            fflush( stderr );
#endif											/* SCIP2_DEBUG */
            if( !Scip2_ChangeBitrate( this, *rate ) )
            {
                fclose( this );
                return NULL;
            }
            Scip2_Flush( this );
            if( Scip2CMD_RS( this ) )
            {
                Scip2_Flush( this );
                break;
            }
            if( Scip2CMD_QT( this ) )
            {
                Scip2_Flush( this );
                break;
            }
            //! Try SCIP2.0 command
            ret = Scip2CMD_SCIP2( this );
            if( ret == -1 )
            {
                usleep( 50000 );
                Scip2_Flush( this );
            }
            if( ret == 0 || ret == -( '0' * 0x100 + 'E' ) )
                break;
            Scip2_Flush( this );
        }
    }
    if( *rate == B0 )
    {
#ifdef SCIP2_DEBUG
        fprintf( stderr, "SCIP2 ERROR: Failed to Fit Bitrate as.\n" );
        fflush( stderr );
#endif											/* SCIP2_DEBUG */
        fclose( this );
        return NULL;
    }
#ifdef SCIP2_DEBUG
    fprintf( stderr, "Changing bitrate to %d bps...\n", bitrate2i( acBitrate ) );
    fflush( stderr );
#endif											/* SCIP2_DEBUG */

    if( acBitrate != B0 )
    {
        if( !( ret = Scip2CMD_SS( this, acBitrate ) ) )
        {
            fprintf( stderr,
                     "SCIP2 WARNING: It is recommended to open device with speed of B0(without changing bitrate), check URG device series and send SS command manually if necessary.\n" );
            fflush( stderr );
#ifdef SCIP2_DEBUG
            fprintf( stderr, "SCIP2 ERROR: Failed to Change Device's Bitrate. (%02d)\n", ret );
            fflush( stderr );
#endif											/* SCIP2_DEBUG */
            // fclose( this );
            // return NULL;
        }
    }

    return this;
}



/*--------------------------------------------------------------*/
/**
  @brief Open SCIP2.0 Device Port on Ethernet
  @param acpAddress Pointer to Name ( IP_address ) of SCIP2.0 Device
  @param acPort Number of Port
  @return failed: NULL, succeeded: Pointer to Device Handle
 */
/*--------------------------------------------------------------*/
S2Port *Scip2_OpenEthernet( const char *acpAddress, const int acPort )
{
    //! Handle to the Device
    FILE *self;
    //! fd
    int fd;
    //! IP address
    struct sockaddr_in address;
    //! Loop valiant
    int i;

    for ( i = 0; i < 1; i++ )
    {
        fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
        if ( fd == -1 ) {
#ifdef SCIP2_DEBUG
            fprintf( stderr, "SCIP2 ERROR: Failed to Create socket.\n");
            fflush( stderr );
#endif											/* SCIP2_DEBUG */
            return NULL;
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr( acpAddress );
        address.sin_port = htons( (unsigned short)acPort );

        if ( connect( fd, (struct sockaddr *) &address, sizeof( address ) ) < 0 ) {
#ifdef SCIP2_DEBUG
            fprintf( stderr, "SCIP2 ERROR: Failed to Connect to '%s:%d'.\n", acpAddress, acPort );
            fflush( stderr );
#endif											/* SCIP2_DEBUG */
            return NULL;
        }

        self = fdopen( fd, "w+" );
        if( self == NULL )
        {
#ifdef SCIP2_DEBUG
            fprintf( stderr, "SCIP2 ERROR: Failed to Open '%s'.\n", acpAddress );
            fflush( stderr );
#endif											/* SCIP2_DEBUG */
            return NULL;
        }
        if( lockf( fileno( self ), F_TLOCK, 0 ) != 0 )
        {
#ifdef SCIP2_DEBUG
            fprintf( stderr, "SCIP2 ERROR: '%s' Locked.\n", acpAddress );
            fflush( stderr );
#endif											/* SCIP2_DEBUG */
            fclose( self );
            return NULL;
        }

        Scip2_Flush( self );

    }

    return self;
}
