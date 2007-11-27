#ifndef ivstrm_h
#define ivstrm_h

/* prevent subsequent inclusion of ivstream if this
didnt come from there since it defines things already possibly defined
in config.h */
#ifndef ivstream_h
#define ivstream_h
#endif

#if defined(HAVE_SSTREAM)
/* the current standard.  Note that one of the arms below is obsolete */
/* this was introduced to avoid the g++ 3.2 warning (and to get more up
to date)
/usr/include/c++/3.2/backward/backward_warning.h:32:2: warning: #warning
This file includes at least one deprecated or antiquated header.  Please
consider using one of the 32 headers found in section 17.4.1.2 of the
C++ standard.  Examples include substituting the <X> header for the
<X.h> header for C++ includes, or <sstream> instead of the deprecated
header <strstream.h>.  To disable this warning use -Wno-deprecated. 
*/

#include <iostream>
#include <fstream>
#include <sstream>
#define IOS_OUT std::ios::out
#define IOS_IN  std::ios::in    
#define IOS_APP  std::ios::app    
using namespace std;

#else /* do not have sstream */

/* introduced for macos since stream.h does not exist.
   also takes care of the declaration of output and input
   with regard to streams
Note: the above standard certainly obsoletes the NO_OUTPUT_OPENMODE stuff.
So macos now handled by the HAVE_SSTREAM case.
*/

#if defined(HAVE_STREAM_H)
#include <stream.h>
#else
#define _STREAM_COMPAT
#include <iostream.h>
#endif

// for some compilers stream.h is insufficient
// following for gcc-3.0.1
#if defined(NO_OUTPUT_OPENMODE)
#include <fstream.h>  // for filebuf
#include <sstream> // for ends
#define IOS_OUT std::ios_base::out
#define IOS_IN  std::ios_base::in    
#define IOS_APP  std::ios_base::app    
#else
#define IOS_OUT output
#define IOS_IN  input
#define IOS_APP  append
#endif

#endif /* do not have sstream */
#endif
