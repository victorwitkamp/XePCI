// XeBootArgs.hpp - Boot argument parsing for XePCI
#pragma once
#include <stdint.h>

struct XeBootFlags {
    bool verbose {false};
    bool disableForcewake {false};
    bool disableCommandStream {false};
};

extern XeBootFlags gXeBoot; // defined in XeBootArgs.cpp

// Parse xepci= comma separated boot flags (verbose,noforcewake,nocs)
void XeParseBootArgs();
