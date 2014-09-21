#ifndef _COMMANDS_h
#define _COMMANDS_h

#include <Streaming.h>

typedef void (*Command)(Stream&,Stream&);
extern Command commands[];

/// <summary>Returns a part of the command descriptions text.</summary>
///
/// <remarks>The command descriptions text is an optimized storage to store the name of
/// commands, their arguments and a short description. This function is not reentrant. Call
/// it with the reset flag set to true before using this function to parse the descriptions.
/// The first call will stream the command' name, the second call the arguments, the third call
/// the description. The function returns false if no content was found. This is espacially the
/// case for the name if there are no more entries in the list.</remarks>
///
/// <param name="out">Output stream for the next part of the description.</param>
/// <param name="reset">Resets the parsing of the descriptions if set to true.</param>
///
/// <returns>True, if content was found.</returns>
extern bool commandText( Stream& out, bool reset = false );

extern void cmdHelp( Stream& in, Stream& out );

#endif // _COMMANDS_h

