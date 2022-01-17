#pragma once

//
// Check for platform
//

#if defined(_WIN32) || defined(_WIN64)
#    define ENV_WINDOWS
#else
#    error "This code cannot compile on non windows platforms"
#endif