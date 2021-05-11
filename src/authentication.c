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

static int get_package_max_token_size(PSecPkgInfo pkgInfo, ULONG numPackages, char *package)
{
    ULONG ctr;

    for (ctr = 0; ctr < numPackages; ctr++) {
        if (! strcmp(package, pkgInfo[ctr].Name)) {
            return pkgInfo[ctr].cbMaxToken;
        }
    }

    return 0;
}

static int obtain_credentials(sspi_auth_ctx* ctx)
{
    SECURITY_STATUS ss;
    TimeStamp throwaway;
    sspi_header_rec *auth_id;

#ifdef UNICODE
    #define SEC_WINNT_AUTH_IDENTITY_UNICODE 0x2
    ctx->hdr.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
#else
    #define SEC_WINNT_AUTH_IDENTITY_ANSI 0x1
    ctx->hdr.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
#endif

    if (ctx->hdr.authtype == typeBasic) {
        auth_id = &ctx->hdr;
        if (auth_id->Domain == NULL && ctx->crec->sspi_domain != NULL) {
            auth_id->Domain = ctx->crec->sspi_domain;
            auth_id->DomainLength = (unsigned long)strlen(ctx->crec->sspi_domain);
        }
    } else {
        auth_id = NULL;
    }

    if (! (ctx->scr->client_credentials.dwLower || ctx->scr->client_credentials.dwUpper)) {
        if ((ss = sspiModuleInfo.functable->AcquireCredentialsHandle(
                                        NULL,
                                        ctx->scr->package,
                                        SECPKG_CRED_OUTBOUND,
                                        NULL, auth_id, NULL, NULL,
                                        &ctx->scr->client_credentials,
                                        &throwaway)
                                        ) != SEC_E_OK) {
            if (ss == SEC_E_SECPKG_NOT_FOUND) {
                ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_FROM_OS_ERROR(GetLastError()), ctx->r,
                    "access to %s failed, reason: unable to acquire credentials "
                    "handle", ctx->r->uri, ctx->scr->package);
            }
            return HTTP_INTERNAL_SERVER_ERROR;
        }
    }

    if (! (ctx->scr->server_credentials.dwLower || ctx->scr->server_credentials.dwUpper)) {
        if ((ss = sspiModuleInfo.functable->AcquireCredentialsHandle(
                                        NULL,
                                        ctx->scr->package,
                                        SECPKG_CRED_INBOUND,
                                        NULL, NULL, NULL, NULL,
                                        &ctx->scr->server_credentials,
                                        &throwaway)
                                        ) != SEC_E_OK) {
            return HTTP_INTERNAL_SERVER_ERROR;
        }
    }

    ctx->scr->have_credentials = TRUE;

    return OK;
}

apr_status_t cleanup_sspi_connection(void *param)
{
    sspi_connection_rec *scr = (sspi_connection_rec *)param;

    if (scr != NULL) {
        if (scr->client_credentials.dwLower || scr->client_credentials.dwUpper) {
            sspiModuleInfo.functable->FreeCredentialHandle(&scr->client_credentials);
            scr->client_credentials.dwLower = 0;
            scr->client_credentials.dwUpper = 0;
        }

        if (scr->server_credentials.dwLower || scr->server_credentials.dwUpper) {
            sspiModuleInfo.functable->FreeCredentialHandle(&scr->server_credentials);
            scr->server_credentials.dwLower = 0;
            scr->server_credentials.dwUpper = 0;
        }

        if (scr->client_context.dwLower || scr->client_context.dwUpper) {
            sspiModuleInfo.functable->DeleteSecurityContext(&scr->client_context);
            scr->client_context.dwLower = 0;
            scr->client_context.dwUpper = 0;
        }

        if (scr->server_context.dwLower || scr->server_context.dwUpper) {
            sspiModuleInfo.functable->DeleteSecurityContext(&scr->server_context);
            scr->server_context.dwLower = 0;
            scr->server_context.dwUpper = 0;
        }

        scr->have_credentials = FALSE;

        if (scr->usertoken) {
            CloseHandle(scr->usertoken);
            scr->usertoken = NULL;
            /* XXX: Memory for these will only be freed after the connection is closed... possible DOS */
            scr->username = NULL;
            scr->groups = NULL;
        }
    }

    return APR_SUCCESS;
}

static char *get_username_from_context(apr_pool_t *p, 
                                       SecurityFunctionTable *functable, 
                                       CtxtHandle *context)
{
    SecPkgContext_Names names;
    SECURITY_STATUS ss;
    char *retval = NULL;

    if ((ss = functable->QueryContextAttributes(context, 
                                                SECPKG_ATTR_NAMES, 
                                                &names)
        ) == SEC_E_OK) {
        retval = apr_pstrdup(p, names.sUserName);
        functable->FreeContextBuffer(names.sUserName);
    }

    return retval;
}

static void log_sspi_auth_failure(request_rec *r, sspi_header_rec *hdr, apr_status_t errcode, char *reason)
{
    if (hdr->User && hdr->Domain) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, errcode, r,
            "user %s\\%s: authentication failure for \"%s\"%s", hdr->Domain, hdr->User, r->uri, reason);
    } else if (hdr->User) {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, errcode, r,
            "user %s: authentication failure for \"%s\"%s", hdr->User, r->uri, reason);
    } else {
        ap_log_rerror(APLOG_MARK, APLOG_ERR, errcode, r,
            "authentication failure for \"%s\": user unknown%s", r->uri, reason);
    }
}

static void log_sspi_logon_denied(request_rec *r, sspi_header_rec *hdr, apr_status_t errcode)
{
    log_sspi_auth_failure(r, hdr, errcode, "");
}

static void log_sspi_invalid_token(request_rec *r, sspi_header_rec *hdr, apr_status_t errcode)
{
    log_sspi_auth_failure(r, hdr, errcode, ", reason: cannot generate context");
}

static SECURITY_STATUS gen_client_context(SecurityFunctionTable *functable, 
                                          CredHandle *credentials, 
                                          CtxtHandle *context, TimeStamp *ctxtexpiry,
                                          BYTE *in, DWORD *inlen, BYTE *out, DWORD *outlen,
                                          LPSTR package)
{
    SecBuffer inbuf, outbuf;
    SecBufferDesc inbufdesc, outbufdesc;
    SECURITY_STATUS ss;
    ULONG ContextAttributes;
    BOOL havecontext = (context->dwLower || context->dwUpper);

    outbuf.cbBuffer = *outlen;
    outbuf.BufferType = SECBUFFER_TOKEN;
    outbuf.pvBuffer = out;
    outbufdesc.ulVersion = SECBUFFER_VERSION;
    outbufdesc.cBuffers = 1;
    outbufdesc.pBuffers = &outbuf;

    if (in) {
        inbuf.cbBuffer = *inlen;
        inbuf.BufferType = SECBUFFER_TOKEN;
        inbuf.pvBuffer = in;
        inbufdesc.ulVersion = SECBUFFER_VERSION;
        inbufdesc.cBuffers = 1;
        inbufdesc.pBuffers = &inbuf;
    }

    ss = functable->InitializeSecurityContext(
                        credentials,
                        havecontext ? context : NULL,
                        package,
                        ISC_REQ_DELEGATE,
                        0,
                        SECURITY_NATIVE_DREP,
                        in ? &inbufdesc : NULL,
                        0,
                        context,
                        &outbufdesc,
                        &ContextAttributes,
                        ctxtexpiry);


    if (ss == SEC_I_COMPLETE_NEEDED || ss == SEC_I_COMPLETE_AND_CONTINUE) {
        functable->CompleteAuthToken(context, &outbufdesc);
    }

    *outlen = outbuf.cbBuffer;

    return ss;
}

static SECURITY_STATUS gen_server_context(SecurityFunctionTable *functable, 
                                          CredHandle *credentials, 
                                          CtxtHandle *context, TimeStamp *ctxtexpiry,
                                          BYTE *in, DWORD *inlen, BYTE *out, DWORD *outlen)
                                          
{
    SecBuffer inbuf, outbuf;
    SecBufferDesc inbufdesc, outbufdesc;
    SECURITY_STATUS ss;
    ULONG ContextAttributes;
    BOOL havecontext = (context->dwLower || context->dwUpper);

    outbuf.cbBuffer = *outlen;
    outbuf.BufferType = SECBUFFER_TOKEN;
    outbuf.pvBuffer = out;
    outbufdesc.ulVersion = SECBUFFER_VERSION;
    outbufdesc.cBuffers = 1;
    outbufdesc.pBuffers = &outbuf;

    inbuf.cbBuffer = *inlen;
    inbuf.BufferType = SECBUFFER_TOKEN;
    inbuf.pvBuffer = in;
    inbufdesc.ulVersion = SECBUFFER_VERSION;
    inbufdesc.cBuffers = 1;
    inbufdesc.pBuffers = &inbuf;

    ss = functable->AcceptSecurityContext(
                          credentials,
                          havecontext ? context : NULL,
                          &inbufdesc,
                          ASC_REQ_DELEGATE,
                          SECURITY_NATIVE_DREP,
                          context,
                          &outbufdesc,
                          &ContextAttributes,
                          ctxtexpiry);

    if (ss == SEC_I_COMPLETE_NEEDED || ss == SEC_I_COMPLETE_AND_CONTINUE) {
        functable->CompleteAuthToken(context, &outbufdesc);
    }

    *outlen = outbuf.cbBuffer;

    return ss;
}

static int check_cleartext_auth(sspi_auth_ctx* ctx)
{
    DWORD cbOut, cbIn, maxTokenSize;
    BYTE *clientbuf, *serverbuf;
    SECURITY_STATUS ss;
  
    maxTokenSize = get_package_max_token_size(sspiModuleInfo.pkgInfo, sspiModuleInfo.numPackages, ctx->scr->package);
    serverbuf = apr_palloc(ctx->r->pool, maxTokenSize);
    clientbuf = NULL;
    cbOut = 0;

    do {
        cbIn = cbOut;
        cbOut = maxTokenSize;
        
        ss = gen_client_context(sspiModuleInfo.functable, &ctx->scr->client_credentials, &ctx->scr->client_context,
                                &ctx->scr->client_ctxtexpiry, clientbuf, &cbIn, serverbuf, &cbOut, 
                                ctx->scr->package);

        if (ss == SEC_E_OK || ss == SEC_I_CONTINUE_NEEDED || ss == SEC_I_COMPLETE_AND_CONTINUE) {
            if (clientbuf == NULL) {
                clientbuf = apr_palloc(ctx->r->pool, maxTokenSize);
            }

            cbIn = cbOut;
            cbOut = maxTokenSize;
            
            ss = gen_server_context(sspiModuleInfo.functable, &ctx->scr->server_credentials, &ctx->scr->server_context,
                                    &ctx->scr->server_ctxtexpiry, serverbuf, &cbIn, clientbuf, &cbOut);
        }
    } while (ss == SEC_I_CONTINUE_NEEDED || ss == SEC_I_COMPLETE_AND_CONTINUE);

    switch (ss) {
        case SEC_E_OK:
            return OK;

        case SEC_E_INVALID_HANDLE:
        case SEC_E_INTERNAL_ERROR:
        case SEC_E_NO_AUTHENTICATING_AUTHORITY:
        case SEC_E_INSUFFICIENT_MEMORY:
            ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_FROM_OS_ERROR(GetLastError()), ctx->r,
                "access to %s failed, reason: cannot generate context", ctx->r->uri);
            return HTTP_INTERNAL_SERVER_ERROR;

        case SEC_E_INVALID_TOKEN:
        case SEC_E_LOGON_DENIED:
        default:
            log_sspi_logon_denied(ctx->r, &ctx->hdr, APR_FROM_OS_ERROR(GetLastError()));
            note_sspi_auth_failure(ctx->r);
            cleanup_sspi_connection(ctx->scr);
            return HTTP_UNAUTHORIZED;
    }
}

static void construct_username(sspi_auth_ctx* ctx)
{
  if (ctx->crec->sspi_omitdomain) {
    char *s = strchr(ctx->scr->username, '\\'); 

    if (s)
      ctx->scr->username = s+1;
    }

  if (ctx->crec->sspi_usernamecase == NULL) {
    }
  else if (!lstrcmpi(ctx->crec->sspi_usernamecase, "Lower")) {
    _strlwr_s(ctx->scr->username, strlen(ctx->scr->username)+1);
    }
  else if (!lstrcmpi(ctx->crec->sspi_usernamecase, "Upper")) {
    _strupr_s(ctx->scr->username, strlen(ctx->scr->username)+1);
    };
}

// from accesscheck.c
apr_table_t *groups_for_user(request_rec *r, HANDLE usertoken);

static int set_connection_details(sspi_auth_ctx* ctx)
{
    SECURITY_STATUS ss;
    
    if (ctx->scr->username == NULL) {
        ctx->scr->username = get_username_from_context(ctx->r->connection->pool, sspiModuleInfo.functable, 
                                                   &ctx->scr->server_context);
    }

    if (ctx->scr->username == NULL)
      return HTTP_INTERNAL_SERVER_ERROR;
    else
      construct_username(ctx);
    
    if (ctx->r->user == NULL) {
        ctx->r->user = ctx->scr->username;
        ctx->r->ap_auth_type = ctx->scr->package;
    }

    if (ctx->scr->usertoken == NULL) {
        if ((ss = sspiModuleInfo.functable->ImpersonateSecurityContext(&ctx->scr->server_context)) != SEC_E_OK) {
            return HTTP_INTERNAL_SERVER_ERROR;
        }

        if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY_SOURCE | TOKEN_READ, TRUE, &ctx->scr->usertoken)) {
            sspiModuleInfo.functable->RevertSecurityContext(&ctx->scr->server_context);
            return HTTP_INTERNAL_SERVER_ERROR;
        }

        if ((ss = sspiModuleInfo.functable->RevertSecurityContext(&ctx->scr->server_context)) != SEC_E_OK) {
            return HTTP_INTERNAL_SERVER_ERROR;
        }
    }

    if(ctx->scr->groups == NULL) {
        ctx->scr->groups = groups_for_user(ctx->r, ctx->scr->usertoken);
    }

    return OK;
}

static int accept_security_context(sspi_auth_ctx* ctx)
{
    SECURITY_STATUS ss;
    sspi_header_rec hdrout;

    hdrout.PasswordLength = get_package_max_token_size(sspiModuleInfo.pkgInfo, sspiModuleInfo.numPackages, ctx->scr->package);
    if (!(hdrout.Password = apr_palloc(ctx->r->pool, hdrout.PasswordLength))) {
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    ss = gen_server_context(sspiModuleInfo.functable, &ctx->scr->server_credentials, &ctx->scr->server_context, 
                            &ctx->scr->server_ctxtexpiry, ctx->hdr.Password, 
                            &ctx->hdr.PasswordLength, hdrout.Password, 
                            &hdrout.PasswordLength);
    
    switch (ss) {
        case SEC_E_OK:
            return OK;

        case SEC_I_COMPLETE_NEEDED:
        case SEC_I_CONTINUE_NEEDED:
        case SEC_I_COMPLETE_AND_CONTINUE: /* already completed if 'complete and continue' */
            note_sspi_auth_challenge(ctx, 
                                     uuencode_binary(ctx->r->pool, hdrout.Password, hdrout.PasswordLength));
            return HTTP_UNAUTHORIZED;

        case SEC_E_INVALID_TOKEN:
            log_sspi_invalid_token(ctx->r, &ctx->hdr, APR_FROM_OS_ERROR(GetLastError()));
            ctx->scr->sspi_failing = 1;
            ctx->scr->package = 0;
            note_sspi_auth_failure(ctx->r);
            cleanup_sspi_connection(ctx->scr);
            return HTTP_UNAUTHORIZED;

        case SEC_E_LOGON_DENIED:
            log_sspi_logon_denied(ctx->r, &ctx->hdr, APR_FROM_OS_ERROR(GetLastError()));
            ctx->scr->sspi_failing = 1;
            ctx->scr->package = 0;
            note_sspi_auth_failure(ctx->r);
            cleanup_sspi_connection(ctx->scr);
            return HTTP_UNAUTHORIZED;

        case SEC_E_INVALID_HANDLE:
        case SEC_E_INTERNAL_ERROR:
        case SEC_E_NO_AUTHENTICATING_AUTHORITY:
        case SEC_E_INSUFFICIENT_MEMORY:
            ap_log_rerror(APLOG_MARK, APLOG_ERR, APR_FROM_OS_ERROR(GetLastError()), ctx->r,
                "access to %s failed, reason: cannot generate server context", ctx->r->uri);
            return HTTP_INTERNAL_SERVER_ERROR;
    }

    return HTTP_INTERNAL_SERVER_ERROR;
}

int authenticate_sspi_user(request_rec *r)
{
    sspi_auth_ctx ctx;

    int res;

#ifdef _DEBUG
    if (sspiModuleInfo.currentlyDebugging == FALSE) {
        sspiModuleInfo.currentlyDebugging = TRUE;
        DebugBreak();
    }
#endif /* def _DEBUG */
            
    SecureZeroMemory(&ctx, sizeof (ctx));
    ctx.r = r;
    ctx.crec = get_sspi_config_rec(r);

    if (!ctx.crec->sspi_on) {
        return DECLINED;
    }

    if (sspiModuleInfo.supportsSSPI == FALSE) {
        if (ctx.crec->sspi_authoritative) {
            ap_log_rerror(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, 0, r,
                "access to %s failed, reason: SSPI support is not available",
                r->uri);
            return HTTP_INTERNAL_SERVER_ERROR;
        } else {
            return DECLINED;
        }
    }

    if (ctx.crec->sspi_package_basic == NULL) {
      ctx.crec->sspi_package_basic = ctx.crec->sspi_packages;

      if (ctx.crec->sspi_package_basic == NULL) {
        ctx.crec->sspi_package_basic = sspiModuleInfo.defaultPackage;
      }
    }

    if (ctx.crec->sspi_packages == NULL) {
      ctx.crec->sspi_packages = ctx.crec->sspi_package_basic;
    }


    apr_pool_userdata_get(&ctx.scr, sspiModuleInfo.userDataKeyString, r->connection->pool);
    
    if (ctx.scr == NULL) {
        ctx.scr = apr_pcalloc(r->connection->pool, sizeof(sspi_connection_rec));
        apr_pool_userdata_setn(ctx.scr, sspiModuleInfo.userDataKeyString, cleanup_sspi_connection, 
          r->connection->pool);
    }

    if (ctx.scr->username == NULL) {
        if (res = get_sspi_header(&ctx)) {
            return res;
        }

        if ((! ctx.scr->have_credentials) && 
            (res = obtain_credentials(&ctx))) {
            return res;
        }

        if (ctx.hdr.authtype == typeSSPI) {
            if (res = accept_security_context(&ctx)) {
                return res;
            }
        } else if (ctx.hdr.authtype == typeBasic) {
          res = check_cleartext_auth(&ctx);
          /* don't forget to clean up open user password */
          SecureZeroMemory(&ctx.hdr, sizeof(ctx.hdr));
          if (res) {
                return res;
            }
        }

        /* we should stick with per-request auth - per connection can cause 
         * problems with POSTing and would be difficult to code such that different
         * configs were allowed on the same connection (eg. CGI that referred to 
         * images in another directory. */
        if (ctx.crec->sspi_per_request_auth) {
          apr_pool_cleanup_kill(r->connection->pool, ctx.scr, cleanup_sspi_connection);
          apr_pool_cleanup_register(r->pool, ctx.scr, cleanup_sspi_connection, apr_pool_cleanup_null);
        }
    }

    if (res = set_connection_details(&ctx)) {
        return res;
    }

    return OK;
}

static int comp_groups_size(void *rec, const char *key, const char *value)
{
    size_t *groups_size = (size_t *) rec;
    (*groups_size) += strlen(value) + 1; // +1 for '|' separator character
    return 1;
}

typedef struct {
    char *groups;
    char *p;
    unsigned int sspi_omitdomain;
} groups_collect_t;

// collect each group name. does NOT append trailing '\0'
static int comp_groups_str(void *rec, const char *key, const char *value)
{
    groups_collect_t *gct = (groups_collect_t *) rec;
    const char *s = value;

    if (gct->sspi_omitdomain) {
        const char *u = strchr(value, '\\');
        if (u)
            s = u+1;
    }

    for(; *s != '\0'; s++)
        *(gct->p++) = s;
    *(gct->p++) = '|';
}

int provide_auth_headers(request_rec *r)
{
    sspi_connection_rec* scr;
    sspi_config_rec* crec;
    apr_table_t *e;

    if (apr_pool_userdata_get(&scr, sspiModuleInfo.userDataKeyString, r->connection->pool)) {
        return OK; // should be some error handling
    }
    crec = get_sspi_config_rec(r);

    /* use a temporary apr_table_t which we'll overlap onto
     * r->subprocess_env later
     * (exception: if r->subprocess_env is empty at the start,
     * write directly into it)
     */
    if (apr_is_empty_table(r->subprocess_env)) {
        e = r->subprocess_env;
    } else {
        e = apr_table_make(r->pool, 2);
    }

    if (scr->username != NULL) {
        apr_table_addn(e, "REMOTE_USER", scr->username);
    }

    if(scr->groups != NULL) {
        size_t groups_str_size = 1;

        // compute required size
        apr_table_do(comp_groups_size, &groups_str_size, scr->groups);

        groups_collect_t gct;
        gct.groups = apr_pcalloc(r->pool, groups_str_size);
        gct.p = gct.groups;
        gct.sspi_omitdomain = crec->sspi_omitdomain;

        // compute groups string
        apr_table_do(comp_groups_str, &gct, scr->groups);
        if(gct.p != gct.groups) {
            gct.p--;
            *(gct.p) = '\0';
        }

        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, APR_SUCCESS, r,
            "adding REMOTE_GROUPS for user \"%s\": \"%s\"", scr->username, gct.groups);

        apr_table_addn(e, "REMOTE_GROUPS", gct.groups);
    }

    if (e != r->subprocess_env) {
      apr_table_overlap(r->subprocess_env, e, APR_OVERLAP_TABLES_SET);
    }

    return OK;
}