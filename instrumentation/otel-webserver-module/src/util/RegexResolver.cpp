#include "RegexResolver.h"
#include <string>
#include <regex>

bool matchIgnorePathRegex(char * uri , char * regexVar){
    const std::string uriString(uri);
    std::string regexVarString(regexVar);

    std::cmatch match;
    const std::regex pattern(regexVarString);
    bool ans = std::regex_match(uriString, pattern);
    if(ans){
        return true;
    }else{
        return false;
    }
}