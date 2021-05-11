/* ====================================================================
 * This code is copyright 1999 - 2006 Tim Costello <tim@syneapps.com>
 * It may be freely distributed, as long as the above notices are reproduced.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "mod_auth_sspi.h"

static apr_table_t *groups_for_user(request_rec *r, HANDLE usertoken)
{
    TOKEN_GROUPS *groupinfo = NULL;
    int groupinfosize = 0;
    SID_NAME_USE sidtype;
    char group_name[_MAX_PATH], domain_name[_MAX_PATH];
    int grouplen, domainlen;
    apr_table_t *grps;
    unsigned int i;

    if ((GetTokenInformation(usertoken, TokenGroups, groupinfo, groupinfosize, &groupinfosize))
        || (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
        return NULL;
    }

    groupinfo = apr_palloc(r->pool, groupinfosize);

    if (!GetTokenInformation(usertoken, TokenGroups, groupinfo, groupinfosize, &groupinfosize)) {
        return NULL;
    }

    grps = apr_table_make(r->connection->pool, groupinfo->GroupCount);

    for (i = 0; i < groupinfo->GroupCount; i++) {
        grouplen = _MAX_PATH;
        domainlen = _MAX_PATH;

        if (!LookupAccountSid(NULL, groupinfo->Groups[i].Sid, 
                              group_name, &grouplen,
                              domain_name, &domainlen,
                              &sidtype)) {
            if (GetLastError() != ERROR_NONE_MAPPED) {
                return NULL;
            }
        } else {
            apr_table_setn(grps, apr_psprintf(r->connection->pool, "%s\\%s", domain_name, group_name), "in");
        }
    }

    return grps;
}

int check_user_access(request_rec *r)
{
    const sspi_config_rec *crec = get_sspi_config_rec(r);
    sspi_connection_rec *scr;
    const char *user = r->user;
    int m = 1 << r->method_number;
    int method_restricted = 0;
    register int x;
    const char *t;
    const char *w;
    int needgroups = 0;
    const apr_array_header_t *reqs_arr = ap_requires(r);
    const require_line *reqs;

    /*
     * If the switch isn't on, don't bother. 
     */
    if (!crec->sspi_on) {
        return DECLINED;
    }
    
    /* BUG FIX: tadc, 11-Nov-1995.  If there is no "requires" directive, 
     * then any user will do.
     */
    if (!reqs_arr) {
        return OK;
    }

    apr_pool_userdata_get(&scr, sspiModuleInfo.userDataKeyString, r->connection->pool);
    /*
     * Did we authenticate this user?
     * If not, we don't want to do user/group checking.
     */
    if (scr == 0 || scr->username != r->user) {
        return DECLINED;
    }

    reqs = (const require_line *) reqs_arr->elts;
        
    /*
     * Fetching the groups for a user is a particularly expensive operation.
     * A quick scan of the requirements should tell us if we don't have to 
     * check groups. This results in a large performance increase for users
     * who only want to check names, or for a "valid-user".
     */
    for (x = 0; x < reqs_arr->nelts; x++) {
        if (!(reqs[x].method_mask & m)) {
            continue;
        }

        t = reqs[x].requirement;
        w = ap_getword_white(r->pool, &t);

        if (!lstrcmpi(w, "group")) {
            needgroups = 1;
            break;
        }
    }

    if (needgroups && scr->groups == NULL) {
        scr->groups = groups_for_user(r, scr->usertoken);
        if (scr->groups == NULL) {
            if (crec->sspi_authoritative) {
                ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_FROM_OS_ERROR(GetLastError()), r,
                    "access to %s failed, reason: cannot get groups for user:"
                    "\"%s\"", r->uri, r->user);
                return HTTP_INTERNAL_SERVER_ERROR;
            } else {
                return DECLINED;
            }
        }
    }

    for (x = 0; x < reqs_arr->nelts; x++) {
        if (!(reqs[x].method_mask & m))
            continue;

        method_restricted = 1;

        t = reqs[x].requirement;
        w = ap_getword_white(r->pool, &t);
        if (!lstrcmpi(w, "valid-user"))
            return OK;
        if (!lstrcmpi(w, "user")) {
            while (t[0]) {
            w = ap_getword_conf(r->pool, &t);
            if (!lstrcmpi(user, w))
                return OK;
            }
        }
        else if (!lstrcmpi(w, "group")) {
            while (t[0]) {
            w = ap_getword_conf(r->pool, &t);
            if (apr_table_get(scr->groups, w))
                return OK;
            }
        } else if (crec->sspi_authoritative) {
            /* if we aren't authoritative, any require directive could be
             * valid even if we don't grok it.  However, if we are 
             * authoritative, we can warn the user they did something wrong.
             * That something could be a missing "AuthAuthoritative off", but
             * more likely is a typo in the require directive.
             */
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, 0, r,
            "access to %s failed, reason: unknown require directive:"
            "\"%s\"", r->uri, reqs[x].requirement);
        }
    }

    if (!method_restricted) {
        return OK;
    }

    /* At this stage we know that the client isn't valid by our records. 
     * georg.weber@infineon.com: only free_credentials if it is_main */
    if (r->main == NULL) {
        cleanup_sspi_connection(scr);
    }

    if (!(crec->sspi_authoritative)) {
        return DECLINED;
    }

    ap_log_rerror(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, 0, r,
      "access to %s failed, reason: user %s not allowed access",
      r->uri, user);
    
    note_sspi_auth_failure(r);

    /*
     * We return HTTP_UNAUTHORIZED (401) because the client may wish
     * to authenticate using a different scheme, or a different 
     * username. If this works, they can be granted access. If we 
     * returned HTTP_FORBIDDEN (403) then they don't get a second
     * chance. 
     */
    return HTTP_UNAUTHORIZED;
}

