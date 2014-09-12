#ifndef FTPClient_h
#define FTPClient_h

#include "Network.h"

#ifdef SUPPORT_FTP

extern bool ftpOpen();

extern bool ftpFileExists( const char* fname );
    
extern bool ftpSetMode( const char* fname, const char* mode, bool ignoreErrors );

extern void ftpClose();

extern EthernetClient ftpClient;
  
#endif // SUPPORT_FTP
#endif // FTPClient_h

