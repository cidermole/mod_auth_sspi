#include "mod_authnz_sspi.h"

/* Register as part of authnz_sspi_module */
APLOG_USE_MODULE(authnz_sspi);

void *create_sspi_server_config(apr_pool_t *p, server_rec *s)
{
    sspi_config_rec *crec = (sspi_config_rec *) apr_pcalloc(p, sizeof(sspi_config_rec));

    /* Set the defaults. */
    crec->sspi_offersspi = TRUE;
    crec->sspi_authoritative = TRUE;

    return crec;
}

void *create_sspi_dir_config(apr_pool_t *p, char *d)
{
    sspi_config_rec *crec = (sspi_config_rec *) apr_pcalloc(p, sizeof(sspi_config_rec));

    /* Set the defaults. */
    crec->sspi_offersspi = TRUE;
    crec->sspi_authoritative = TRUE;

    return crec;
}

void * merge_sspi_dir_config(apr_pool_t *p, void *base_conf, void *new_conf)
{
  sspi_config_rec* merged_rec = create_sspi_dir_config(p, 0);
  sspi_config_rec* base_rec = base_conf;
  sspi_config_rec* new_rec  = new_conf;

#ifdef _DEBUG
    if (sspiModuleInfo.currentlyDebugging == FALSE) {
        sspiModuleInfo.currentlyDebugging = TRUE;
        DebugBreak();
    }
#endif /* def _DEBUG */

  if (base_rec->sspi_on && !new_rec->sspi_on)
    memcpy(merged_rec, base_rec, sizeof(sspi_config_rec));
  else
  if (!base_rec->sspi_on && new_rec->sspi_on)
    memcpy(merged_rec, new_rec, sizeof(sspi_config_rec));

  return merged_rec;
}

static int get_sspi_userpass(sspi_auth_ctx* ctx, const char *auth_line)
{
    int len;

    /*
     * OK, we're here. The client is using SSPI and _should_ have 
     * sent a token to us. All we have to do is decode it and return...
     */
    if (auth_line) {
        ctx->hdr.Password = uudecode_binary(ctx->r->pool, auth_line, &len);
        ctx->hdr.PasswordLength = len;
        ctx->hdr.authtype = typeSSPI;
    } else {
        if (ctx->crec->sspi_authoritative) {
            return HTTP_BAD_REQUEST;
        } else {
            return DECLINED;
        }
    }

    if (!ctx->hdr.PasswordLength || !ctx->hdr.Password) {
        if (ctx->crec->sspi_authoritative) {
            return HTTP_BAD_REQUEST;
        } else {
            return DECLINED;
        }
    }

    return OK;
}

static int get_basic_userpass(sspi_auth_ctx* ctx, const char *auth_line)
{
    char *ptr, *domainptr;
    int len;

    if (!(ptr = uudecode_binary(ctx->r->pool, auth_line, &len))) {
        note_sspi_auth_failure(ctx->r);
        if (ctx->crec->sspi_authoritative) {
            return HTTP_BAD_REQUEST;
        } else {
            return DECLINED;
        }
    }

    ctx->hdr.User = ap_getword_nulls(ctx->r->pool, &ptr, ':');
    if (ctx->hdr.User) {
        ctx->hdr.UserLength = strlen(ctx->hdr.User);
    } else {
        note_sspi_auth_failure(ctx->r);
        if (ctx->crec->sspi_authoritative) {
            return HTTP_BAD_REQUEST;
        } else {
            return DECLINED;
        }
    }

    for (domainptr = ctx->hdr.User; (unsigned long)(domainptr - ctx->hdr.User) < ctx->hdr.UserLength; domainptr++) {
        if (*domainptr == '\\' || *domainptr == '/') { /* let's use any slash */
            *domainptr = '\0';
            ctx->hdr.Domain = ctx->hdr.User;
            ctx->hdr.DomainLength = strlen(ctx->hdr.Domain);
            ctx->hdr.User = domainptr + 1;
            ctx->hdr.UserLength = strlen(ctx->hdr.User);
            break;
        }
    }
    
    ctx->hdr.Password = ptr;
    if (ctx->hdr.Password) {
        ctx->hdr.PasswordLength = strlen(ctx->hdr.Password);
    } else {
        note_sspi_auth_failure(ctx->r);
        if (ctx->crec->sspi_authoritative) {
            return HTTP_BAD_REQUEST;
        } else {
            return DECLINED;
        }
    }

    ctx->hdr.authtype = typeBasic;

    return OK;
}

const char *get_authorization_header_name(request_rec *r)
{
    return (PROXYREQ_PROXY == r->proxyreq) ? "Proxy-Authorization"
        : "Authorization";
}

const char *get_authenticate_header_name(request_rec *r)
{
    return (PROXYREQ_PROXY == r->proxyreq) ? "Proxy-Authenticate"
        : "WWW-Authenticate";
}

static int check_package_valid(sspi_auth_ctx* ctx, char* scheme)
{
  if (0 != ctx->scr->package &&  0 != lstrcmpi(scheme, ctx->scr->package))
    return HTTP_INTERNAL_SERVER_ERROR;

  if (0 == ctx->crec->sspi_packages)
    return HTTP_INTERNAL_SERVER_ERROR;
  else
    {
    const char* list = ctx->crec->sspi_packages;
    char* w;
    while (list && list[0])
      {
      w = ap_getword_white(ctx->r->pool, &list);
      if (w && w[0] && lstrcmpi(w, scheme) ==0)
        return OK;
      }
    }

  return HTTP_INTERNAL_SERVER_ERROR;
}

int get_sspi_header(sspi_auth_ctx* ctx)
{
    char* scheme;
    const char *auth_line = apr_table_get(ctx->r->headers_in, 
        get_authorization_header_name(ctx->r));

    /*
     * If the client didn't supply an Authorization: (or Proxy-Authorization) 
     * header, we need to reply 401 and supply a WWW-Authenticate
     * (or Proxy-Authenticate) header indicating acceptable authentication
     * schemes. 
     */
    if (!auth_line) {
        note_sspi_auth_failure(ctx->r);
        return HTTP_UNAUTHORIZED;
    }

    /*
     * Do a quick check of the Authorization: header. If it is 'Basic', and we're
     * allowed, try a cleartext logon. Else if it isn't the selected package 
     * and we're authoritative, reply 401 again. 
     */
    scheme = ap_getword_white(ctx->r->pool, &auth_line);

    if (ctx->crec->sspi_offersspi && 0 == check_package_valid(ctx, scheme)) {
      if (0 == ctx->scr->package)
        ctx->scr->package = apr_pstrdup(ctx->r->connection->pool, scheme);
      return get_sspi_userpass(ctx, auth_line);
    } else if (ctx->crec->sspi_offerbasic && 0 == lstrcmpi(scheme, "Basic")) {
      ctx->scr->package = ctx->crec->sspi_package_basic;
      return get_basic_userpass(ctx, auth_line);
    }

    if (ctx->crec->sspi_authoritative) {
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, 0, ctx->r,
            "client used wrong authentication scheme: %s for %s (needed %s)", 
            ctx->scr->package, ctx->r->uri, ctx->crec->sspi_packages);
        note_sspi_auth_failure(ctx->r);
        return HTTP_UNAUTHORIZED;
    } else {
        return DECLINED;
    }

    return HTTP_INTERNAL_SERVER_ERROR;
}

void note_sspi_auth_failure(request_rec *r)
{
  const char *auth_hdr = get_authenticate_header_name(r);
  sspi_config_rec *crec = get_sspi_config_rec(r);

  char* basicline = 0;

  apr_table_unset(r->err_headers_out, auth_hdr);

  if (crec->sspi_offerbasic)
    {
    basicline = apr_psprintf(r->pool, "Basic realm=\"%s\"", ap_auth_name(r));
    }

  if (crec->sspi_offersspi)
    {
    sspi_connection_rec* scr = 0;
    apr_pool_userdata_get(&scr, sspiModuleInfo.userDataKeyString, r->connection->pool);
    
    if (scr == 0 || scr->sspi_failing == 0)
      {
      char* w;
      const char* package_list = crec->sspi_packages;

      if (crec->sspi_offerbasic && crec->sspi_basicpreferred)
        {
        apr_table_addn(r->err_headers_out, auth_hdr, basicline);
        basicline = 0;
        }

      if (package_list) while (*package_list)
        {
        w = ap_getword_white(r->pool, &package_list);
        if (w[0])
          {
          apr_table_addn(r->err_headers_out, auth_hdr, w);
          }
        }
      }
    }

  if (basicline != 0)
    {
    apr_table_addn(r->err_headers_out, auth_hdr, basicline);
    }
}

void note_sspi_auth_challenge(sspi_auth_ctx* ctx, const char *challenge)
{
    const char *auth_hdr = get_authenticate_header_name(ctx->r);

    apr_table_setn(ctx->r->err_headers_out, auth_hdr, 
      apr_psprintf(ctx->r->pool, "%s %s", ctx->scr->package, challenge));

    if (ctx->r->connection->keepalives)
        --ctx->r->connection->keepalives;

    /*
     * See above comment about ugly hack. 
     */
    if ((ctx->crec->sspi_msie3hack) && (ctx->r->proto_num < HTTP_VERSION(1,1))) {
        apr_table_setn(ctx->r->err_headers_out, "Content-Length", "0");
    }
}

char *uuencode_binary(apr_pool_t *p, const char *data, int len)
{
    int encodelength;
    char *encoded;

    encodelength = apr_base64_encode_len(len);
    encoded = apr_palloc(p, encodelength);

    if (encoded != NULL) {
        if (apr_base64_encode_binary(encoded, data, len) > 0) {
            return encoded;
        }
    }

    return NULL;
}

unsigned char *uudecode_binary(apr_pool_t *p, const char *data, int *decodelength)
{
    char *decoded;

    *decodelength = apr_base64_decode_len(data);
    decoded = apr_palloc(p, *decodelength);

    if (decoded != NULL) {
        *decodelength = apr_base64_decode_binary(decoded, data);
        if (*decodelength > 0) {
            decoded[(*decodelength)] = '\0';
            return decoded;
        }
    }

    return NULL;
}

