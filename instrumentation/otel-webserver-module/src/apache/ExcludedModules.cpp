/*
* Copyright 2022, OpenTelemetry Authors. 
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

// file header
#include "ExcludedModules.h"
// system headers
#include <cstddef>
#include <unordered_set>
#include <sstream>
// apache headers
#include <httpd.h>

const std::unordered_set<std::string> ExcludedModules::excludedAlways
{
    "mod_apache_otel.cpp" // filter out our own mod! else insertHook will infinite loop!
};

std::unordered_set<std::string> ExcludedModules::userSpecified;

// Apache 2.4 excluded module list
const std::unordered_set<std::string> ExcludedModules::excluded24
{
    "core.c"
    ,"http_core.c"
    ,"mod_access_compat.c"
    ,"mod_actions.c"
    ,"mod_alias.c"
    ,"mod_allowmethods.c"
    ,"mod_auth_basic.c"
    ,"mod_auth_digest.c"
    ,"mod_authn_anon.c"
    ,"mod_authn_core.c"
    ,"mod_authn_dbd.c"
    ,"mod_authn_dbm.c"
    ,"mod_authn_file.c"
    ,"mod_authn_socache.c"
    ,"mod_authz_core.c"
    ,"mod_authz_dbd.c"
    ,"mod_authz_dbm.c"
    ,"mod_authz_groupfile.c"
    ,"mod_authz_host.c"
    ,"mod_authz_owner.c"
    ,"mod_authz_user.c"
    ,"mod_autoindex.c"
    ,"mod_cache.c"
    ,"mod_cache_disk.c"
    ,"mod_cgi.c"
    ,"mod_data.c"
    ,"mod_dbd.c"
    ,"mod_deflate.c"
    ,"mod_dir.c"
    ,"mod_dumpio.c"
    ,"mod_echo.c"
    ,"mod_env.c"
    ,"mod_expires.c"
    ,"mod_ext_filter.c"
    ,"mod_filter.c"
    ,"mod_headers.c"
    ,"mod_include.c"
    ,"mod_info.c"
    ,"mod_lbmethod_bybusyness.c"
    ,"mod_lbmethod_byrequests.c"
    ,"mod_lbmethod_bytraffic.c"
    ,"mod_lbmethod_heartbeat.c"
    ,"mod_log_config.c"
    ,"mod_logio.c"
    ,"mod_lua.c"
    ,"mod_mime.c"
    ,"mod_mime_magic.c"
    ,"mod_negotiation.c"
    ,"mod_remoteip.c"
    ,"mod_reqtimeout.c"
    ,"mod_rewrite.c"
    ,"mod_setenvif.c"
    ,"mod_slotmem_plain.c"
    ,"mod_slotmem_shm.c"
    ,"mod_so.c"
    ,"mod_socache_dbm.c"
    ,"mod_socache_memcache.c"
    ,"mod_socache_shmcb.c"
    ,"mod_status.c"
    ,"mod_substitute.c"
    ,"mod_suexec.c"
    ,"mod_systemd.c"
    ,"mod_unique_id.c"
    ,"mod_unixd.c"
    ,"mod_userdir.c"
    ,"mod_version.c"
    ,"mod_vhost_alias.c"
    ,"prefork.c"
};

const std::unordered_set<std::string> ExcludedModules::excluded22
{
    "core.c"
    ,"http_core.c"
    ,"mod_actions.c"
    ,"mod_alias.c"
    ,"mod_auth_basic.c"
    ,"mod_auth_digest.c"
    ,"mod_authn_alias.c"
    ,"mod_authn_anon.c"
    ,"mod_authn_dbm.c"
    ,"mod_authn_default.c"
    ,"mod_authn_file.c"
    ,"mod_authnz_ldap.c"
    ,"mod_authz_dbm.c"
    ,"mod_authz_default.c"
    ,"mod_authz_groupfile.c"
    ,"mod_authz_host.c"
    ,"mod_authz_owner.c"
    ,"mod_authz_user.c"
    ,"mod_autoindex.c"
    ,"mod_cache.c"
    ,"mod_cgi.c"
    ,"mod_deflate.c"
    ,"mod_dir.c"
    ,"mod_disk_cache.c"
    ,"mod_env.c"
    ,"mod_expires.c"
    ,"mod_ext_filter.c"
    ,"mod_file_cache.c"
    ,"mod_headers.c"
    ,"mod_include.c"
    ,"mod_info.c"
    ,"mod_log_config.c"
    ,"mod_logio.c"
    ,"mod_mem_cache.c"
    ,"mod_mime.c"
    ,"mod_mime_magic.c"
    ,"mod_negotiation.c"
    ,"mod_perl.c"
    ,"mod_python.c"
    ,"mod_rewrite.c"
    ,"mod_setenvif.c"
    ,"mod_so.c"
    ,"mod_speling.c"
    ,"mod_ssl.c"
    ,"mod_status.c"
    ,"mod_suexec.c"
    ,"mod_userdir.c"
    ,"mod_usertrack.c"
    ,"mod_version.c"
    ,"mod_vhost_alias.c"
    ,"prefork.c"
    ,"util_ldap.c"
};

// Find the modules that we want to instrument for a stage.
// Add modules that are specified by the user and  modules NOT in the exclude lists.
void ExcludedModules::findHookPoints(
        std::vector<HookInfo> &hooks_found,
        hook_get_t getHooks,
        const std::string& stage)
{
    // function prototype for the apache module linked list structure
    typedef struct
    {                               //XXX: should get something from apr_hooks.h instead
        void (*pFunc) (void);       // just to get the right size
        const char *szName;
        const char *const *aszPredecessors;
        const char *const *aszSuccessors;
        int nOrder;
    } hook_struct_t;

    apr_array_header_t* hooks = getHooks();
    if (hooks)
    {
        // see if any hooks are specified by user
        for (int ix = 0; ix < hooks->nelts; ix++)
        {
            hook_struct_t* elts = (hook_struct_t*)hooks->elts;
            // not in excludeAlways and (specified by user or not in exluded lists)

            if (excludedAlways.find(elts[ix].szName) == excludedAlways.end())
            {
                if (userSpecified.find(elts[ix].szName) != userSpecified.end())
                {
                    hooks_found.push_back((HookInfo(stage, elts[ix].szName, elts[ix].nOrder)));
                }
                else if ((excluded22.find(elts[ix].szName) == excluded22.end()) &&
                         (excluded24.find(elts[ix].szName) == excluded24.end()))
                {   // TODO: find a way to determine if we are in 2.2 or 2.4 and only use one of these.
                    hooks_found.push_back((HookInfo(stage, elts[ix].szName, elts[ix].nOrder)));
                }
            }
        }
    }
}

// parse the comma separated modules from user
void ExcludedModules::getUserSpecifiedModules(const char* moduleStr)
{
    /*
        We'll ask the user to provide us with specific modules which need to be monitored
        either through configuration file or through the UI.
        Push the modules into a list/map.
    */
}
