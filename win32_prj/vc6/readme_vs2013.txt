#ifndef __FLTUSED__
#define __FLTUSED__
#if _MSC_VER > 1200
__declspec(selectany) int _fltused = 1;
#endif
#endif
-->
#ifndef __FLTUSED__
#define __FLTUSED__
#if _MSC_VER > 1800
__declspec(selectany) int _fltused = 1;
#endif
#endif



---------------


#if SDL_DYNAMIC_API
---->//#include "dynapi/SDL_dynapi_overrides.h"
/* force DECLSPEC and SDLCALL off...it's all internal symbols now.
   These will have actual #defines during SDL_dynapi.c only */
#define DECLSPEC
#define SDLCALL
#endif

----------------
