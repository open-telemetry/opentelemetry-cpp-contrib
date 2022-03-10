#ifndef __module_version_h
#define __module_version_h

#define MODULE_SHA1 "$headSHA1"
#define MODULE_VERSION_ "$moduleVersion"
#define RELEASE_ "$moduleRelease"
#define FULL_MODULE_VERSION_ "$fullVersion"

static const char MODULE_VERSION[] =
    MODULE_VERSION_ RELEASE_  MODULE_SHA1;

static const char FULL_MODULE_VERSION[] =
    FULL_MODULE_VERSION_;

#endif

