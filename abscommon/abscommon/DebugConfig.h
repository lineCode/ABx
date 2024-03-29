#pragma once

// Used by several projects

#if defined(_DEBUG)
//#   define DEBUG_DISPATCHER
//#   define DEBUG_SCHEDULER
#   define DEBUG_NET
//#   define DEBUG_SQL
//#   define DEBUG_MATH
#   define DEBUG_GAME
#   ifdef DEBUG_GAME
//#       define DEBUG_NAVIGATION
#       define DEBUG_PROTOCOL
//#       define DEBUG_OCTREE
//#       define DEBUG_COLLISION
//#       define DEBUG_AI
#   endif
#else
#endif

#if !defined(NPROFILING)
#   define PROFILING
#else
#   undef PROFILING
#endif
