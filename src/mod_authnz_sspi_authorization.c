#include "mod_authnz_sspi.h"

/* Register as part of authnz_sspi_module */
APLOG_USE_MODULE(authnz_sspi);

int is_member(request_rec *r, HANDLE usertoken, const char *w)
{
    PSID pGroupSid = NULL;
    int sidsize = 0;
    char domain_name[_MAX_PATH];
    int domainlen = _MAX_PATH;
    SID_NAME_USE snu;
    int member = 0;

    /* Get the SID for the group pointed by w */
    LookupAccountName(NULL, w, pGroupSid, &sidsize, domain_name, &domainlen,
            &snu);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        if (pGroupSid = apr_palloc(r->pool, sidsize)) {
            if (LookupAccountName(NULL, w, pGroupSid, &sidsize, domain_name,
                    &domainlen, &snu)) {
                BOOL IsMember;
                /* Check if the current user is allowed in this SID */
                if (CheckTokenMembership(usertoken, pGroupSid, &IsMember)) {
                    if (IsMember) {
                        member = 1;
                    }
                }
                else {
                    ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ALERT, 0,
                            r, "CheckTokenMembership(): error %d",
                            GetLastError());
                }
            }
            else {
                ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ALERT, 0, r,
                        "LookupAccountName(2): error %d", GetLastError());
            }
        }
        else {
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ALERT, 0, r,
                    "An error occured in is_member(): apr_palloc() failed.");
        }
    }
    else {
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ALERT, 0, r,
                "LookupAccountName(1): error %d", GetLastError());
    }

    return member;
}


#if 0
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
        if (crec->sspi_chain_auth) {
            ap_log_rerror(APLOG_MARK,
                    APLOG_NOERRNO | APLOG_INFO,  0 , r, "mod_sspi: "
            "Declining decision for user %s to %s, no SSPI "
            "restriction, let alternative auth. module decide",
            user, r->uri);
            return DECLINED;
        }
        else {
            return OK;
        }
    }

    apr_pool_userdata_get(&scr, sspiModuleInfo.userDataKeyString,
            r->connection->pool);
    /*
     * Did we authenticate this user?
     * If not, we don't want to do user/group checking.
     */
    if (scr == 0 || scr->username != r->user) {
        if (crec->sspi_chain_auth) {
            ap_log_rerror(
                    APLOG_MARK,
                    APLOG_NOERRNO | APLOG_ERR,
                    0,
                    r,
                    "mod_sspi: Blocked request, user not SSPI authenticated, "
                    "do not check alternative auth. module");
            return HTTP_UNAUTHORIZED;
        }
        else {
            return DECLINED;
        }
    }

    reqs = (const require_line *) reqs_arr->elts;

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
                if (crec->sspi_chain_auth) {
                    ap_log_rerror(
                            APLOG_MARK,
                            APLOG_NOERRNO | APLOG_INFO,
                            0,
                            r,
                            "mod_sspi: Declining decision for user %s to "
                            "%s, SSPI authorized user, let alternative auth. "
                            "module decide",
                            user, r->uri);
                    return DECLINED;
                }
                else {
                    return OK;
                }
            }
        }
        else if (!lstrcmpi(w, "group")) {
            while (t[0]) {
                w = ap_getword_conf(r->pool, &t);
                if (is_member(r, scr->usertoken, w))
                if (crec->sspi_chain_auth) {
                    ap_log_rerror(
                            APLOG_MARK,
                            APLOG_NOERRNO | APLOG_INFO,
                            0,
                            r,
                            "mod_sspi: Declining decision for user %s to %s, "
                            "SSPI authorized group member, let alternative "
                            "auth. module decide",
                            user, r->uri);
                    return DECLINED;
                }
                else {
                    return OK;
                }
            }
        }
        else if (crec->sspi_authoritative) {
            /* if we aren't authoritative, any require directive could be
             * valid even if we don't grok it.  However, if we are
             * authoritative, we can warn the user they did something wrong.
             * That something could be a missing "AuthAuthoritative off", but
             * more likely is a typo in the require directive.
             */
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ERR, 0, r,
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

    if (crec->sspi_chain_auth) {
        ap_log_rerror(
                APLOG_MARK,
                APLOG_NOERRNO | APLOG_ERR,
                0,
                r,
                "mod_sspi: Blocked request to %s, user %s not allowed access "
                "at SSPI level, do not check alternative auth. module",
                r->uri, user);
        return HTTP_UNAUTHORIZED;
    }

    if (!(crec->sspi_authoritative)) {
        return DECLINED;
    }

    ap_log_rerror(APLOG_MARK, APLOG_NOERRNO | APLOG_ERR, 0, r,
            "access to %s failed, reason: user %s not allowed access", r->uri,
            user);

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
#endif

int sspi_common_authz_check ( request_rec *r,
                                    const sspi_config_rec *crec,
                                    sspi_connection_rec **pscr,
                                    authz_status *pas)
{
    int res = 1;

    /*
     * If the switch isn't on, don't bother.
     */
    if (!crec->sspi_on) {

        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, SSPILOGNO(00004)
                  "Access to %s failed, reason: SSPIAuth is off",
                  r->uri);

        *pas = AUTHZ_DENIED;
        res = 0;
    }

    /*
     * If no user was authenticated, deny.
     */
    if (res && !r->user) {

        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, SSPILOGNO(00005)
                  "Access to %s failed, reason: No user authenticated",
                  r->uri);

        *pas = AUTHZ_DENIED_NO_USER;
        res = 0;
    }

    /* Retrieve SSPI Connection Record */
    if (res)
    {
        apr_pool_userdata_get(pscr, sspiModuleInfo.userDataKeyString,
                r->connection->pool);

        /*
         * Did we authenticate this user?
         * If not, we don't want to do further checking.
         */
        if (*pscr == 0 || (*pscr)->username != r->user) {

            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, SSPILOGNO(00006)
                      "Access to %s failed, reason: inconsistent SSPI record",
                      r->uri);

            *pas = AUTHZ_DENIED;
            res = 0;
        }
    }

    return res;
}

static void common_deny_actions (request_rec *r,
                                 sspi_connection_rec *scr)
{
    /* At this stage we know that the client isn't valid by our records. 
     * georg.weber@infineon.com: only free_credentials if it is_main */
    if (r->main == NULL) {
        cleanup_sspi_connection(scr);
    }

    ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, SSPILOGNO(00003)
                  "access to %s failed, reason: user '%s' does not meet "
                  "'require'ments for user to be allowed access",
                  r->uri, r->user);

    note_sspi_auth_failure(r);
}

authz_status sspi_user_check_authorization( request_rec *r,
                                            const char *require_args,
                                            const void *parsed_require_args)
{
    const sspi_config_rec *crec = get_sspi_config_rec(r);
    char *user = r->user;
    sspi_connection_rec *scr;
    const char *t, *w;
    authz_status as;

    // First check if a user was authenticated
    if (!sspi_common_authz_check(r, crec, &scr, &as))
    {
        return as;
    }

    // There is a valid user, check user requirements...
    t = require_args;
    while ((w = ap_getword_conf(r->pool, &t)) && w[0]) {
        if (!strcmp(user, w)) {
            return AUTHZ_GRANTED;
        }
    }

    // Prepare for "try again"
    common_deny_actions(r, scr);

    return AUTHZ_DENIED;
}

authz_status sspi_group_check_authorization(    request_rec *r,
                                                const char *require_args,
                                                const void *parsed_require_args)
{
    const sspi_config_rec *crec = get_sspi_config_rec(r);
    char *user = r->user;
    sspi_connection_rec *scr;
    const char *t, *w;
    authz_status as;

    // First check if a user was authenticated
    if (!sspi_common_authz_check(r, crec, &scr, &as))
    {
        return as;
    }

    // There is a valid user, check group requirements...
    t = require_args;
    while ((w = ap_getword_conf(r->pool, &t)) && w[0]) {
        if (is_member(r, scr->usertoken, w)) {
            return AUTHZ_GRANTED;
        }
    }

    // Prepare for "try again"
    common_deny_actions(r, scr);

    return AUTHZ_DENIED;
}

authz_status sspi_valid_check_authorization(    request_rec *r,
                                                const char *require_args,
                                                const void *parsed_require_args )
{
    const sspi_config_rec *crec = get_sspi_config_rec(r);
    char *user = r->user;
    sspi_connection_rec *scr;
    authz_status as;

    if (!sspi_common_authz_check(r, crec, &scr, &as))
    {
        return as;
    }

    // There is a valid user, that's all what we wanted to know.

    return AUTHZ_GRANTED;
}


