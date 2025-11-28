// XeBootArgs.cpp - implementation of boot arg parsing for XePCI
#include "XeBootArgs.hpp"
#include <IOKit/IOLib.h>
#include <pexpert/pexpert.h>
#include <string.h>   // strcmp, strchr
#include <ctype.h>    // tolower

XeBootFlags gXeBoot; // global instance

void XeParseBootArgs() {
    char buf[128] = {};
    if (!PE_parse_boot_argn("xepci", buf, sizeof(buf))) {
        return; // no boot arg provided
    }

    // Tokenize by commas
    char *p = buf;
    while (p && *p) {
        char *comma = strchr(p, ',');
        if (comma) *comma = '\0';
        // Trim and lowercase token
        while (*p == ' ' || *p == '\t') ++p;
        char token[64] = {};
        size_t i = 0;
        while (p[i] && i < sizeof(token)-1) { token[i] = (char)tolower(p[i]); ++i; }
        token[i] = '\0';
        if (strcmp(token, "verbose") == 0) {
            gXeBoot.verbose = true;
        } else if (strcmp(token, "noforcewake") == 0) {
            gXeBoot.disableForcewake = true;
        } else if (strcmp(token, "nocs") == 0) {
            gXeBoot.disableCommandStream = true;
        } else if (strcmp(token, "strictsafe") == 0) {
            gXeBoot.strictSafe = true;
            gXeBoot.disableForcewake = true;
            gXeBoot.disableCommandStream = true;
        }
        if (!comma) break;
        p = comma + 1;
    }
    IOLog("XePCI: boot flags: verbose=%d noforcewake=%d nocs=%d strictsafe=%d\n",
          gXeBoot.verbose, gXeBoot.disableForcewake, gXeBoot.disableCommandStream, gXeBoot.strictSafe);
}
