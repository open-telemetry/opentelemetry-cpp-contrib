#ifndef __module_version_h
#define __module_version_h

#define MODULE_SHA1 "$headSHA1"
#define MODULE_VERSION_ "$moduleVersion"
#define RELEASE_ "$moduleRelease"
#define FULL_MODULE_VERSION_ "$fullVersion"
#define CPP_SDK_VERSION_ "$cppSDKVersion"

static const char MODULE_VERSION[] =
    MODULE_VERSION_ RELEASE_  MODULE_SHA1;

static const char FULL_MODULE_VERSION[] =
    FULL_MODULE_VERSION_;

static const char CPP_SDK_VERSION[] = 
    CPP_SDK_VERSION_;

#endif

