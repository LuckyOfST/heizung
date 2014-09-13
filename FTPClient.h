#ifndef FTPClient_h
#define FTPClient_h

#include "Network.h"

#ifdef SUPPORT_FTP

#define FTP_RECEIVE "RETR"
#define FTP_STORE "STOR"
#define FTP_APPEND "APPE"
#define FTP_MAKEDIR "MKD"  

extern EthernetClient ftpClient;

extern bool ftpOpen();
extern bool ftpFileExists( const char* fname );
extern bool ftpSetMode( const char* fname, const char* mode, bool ignoreErrors );
extern void ftpClose();
  
#endif // SUPPORT_FTP
#endif // FTPClient_h

