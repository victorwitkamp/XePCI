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
        // Trim leading whitespace
        while (*p == ' ' || *p == '\t') ++p;
        // Copy and lowercase token, stopping at trailing whitespace
        char token[64] = {};
        size_t i = 0;
        while (p[i] && p[i] != ' ' && p[i] != '\t' && i < sizeof(token)-1) {
            token[i] = (char)tolower(p[i]);
            ++i;
        }
        token[i] = '\0';
        // Skip empty tokens
        if (token[0] == '\0') {
            if (!comma) break;
            p = comma + 1;
            continue;
        }
        if (strcmp(token, "verbose") == 0) {
            gXeBoot.verbose = true;
        } else if (strcmp(token, "noforcewake") == 0) {
            gXeBoot.disableForcewake = true;
        } else if (strcmp(token, "nocs") == 0) {
            gXeBoot.disableCommandStream = true;
        }
        if (!comma) break;
        p = comma + 1;
    }
    IOLog("XePCI: boot flags: verbose=%d noforcewake=%d nocs=%d\n",
          gXeBoot.verbose, gXeBoot.disableForcewake, gXeBoot.disableCommandStream);
}
