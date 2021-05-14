/*
*    "This product includes software developed by the Apache Group
*    for use in the Apache HTTP server project (http://www.apache.org/)."
*
* Copyright:
* mod_auth_sspi code is originally Copr. 1999 Tim Costello <tim@syneapps.com>
* mod_authnz_sspi code is Copr. 2012 Ennio Zarlenga <wzzxo@yahoo.fr>
*
* This code may be freely distributed, as long as the above notices are reproduced.
*/

#include "mod_authnz_sspi.h"


module AP_MODULE_DECLARE_DATA authnz_sspi_module;

sspi_module_rec sspiModuleInfo = {0,};


static const command_rec sspi_cmds[] =
{
    AP_INIT_FLAG("SSPIAuth", ap_set_flag_slot, 
                 (void *) APR_OFFSETOF(sspi_config_rec, sspi_on), OR_AUTHCFG, 
                 "set to 'on' to activate SSPI authentication here"),
    AP_INIT_FLAG("SSPIOfferSSPI", ap_set_flag_slot,
                 (void *) APR_OFFSETOF(sspi_config_rec, sspi_offersspi), OR_AUTHCFG, 
                 "set to 'off' to allow access control to be passed along to "
                 "lower modules if the UserID is not known to this module"),
    AP_INIT_FLAG("SSPIAuthoritative", ap_set_flag_slot,
                 (void *) APR_OFFSETOF(sspi_config_rec, sspi_authoritative), OR_AUTHCFG, 
                 "set to 'off' to allow access control to be passed along to "
                 "lower modules if the UserID is not known to this module"),
    AP_INIT_FLAG("SSPIOfferBasic", ap_set_flag_slot,
                 (void *) APR_OFFSETOF(sspi_config_rec, sspi_offerbasic), OR_AUTHCFG, 
                 "set to 'on' to allow the client to authenticate against NT "
                 "with 'Basic' authentication instead of using the NTLM protocol"),
    AP_INIT_TAKE1("SSPIPackage", ap_set_string_slot,
                 (void *) APR_OFFSETOF(sspi_config_rec, sspi_package_basic), OR_AUTHCFG, 
                 "set to the name of the package you want to use to "
                 "authenticate users"),
    AP_INIT_TAKE1("SSPIPackages", ap_set_string_slot,
                 (void *) APR_OFFSETOF(sspi_config_rec, sspi_packages), OR_AUTHCFG, 
                 "set to the name of the package you want to use to "
                 "authenticate users"),
    AP_INIT_TAKE1("SSPIDomain", ap_set_string_slot,
                 (void *) APR_OFFSETOF(sspi_config_rec, sspi_domain), OR_AUTHCFG, 
                 "set to the domain you want users authenticated against for "
                 "cleartext authentication - if not specified, the local "
                 "machine, then all trusted domains are checked"),
    AP_INIT_FLAG("SSPIOmitDomain", ap_set_flag_slot,
                 (void *) APR_OFFSETOF(sspi_config_rec, sspi_omitdomain), OR_AUTHCFG, 
                 "set to 'on' if you want the usernames to have the domain "
                 "prefix OMITTED, on = user, off = DOMAIN\\user"),
    AP_INIT_TAKE1("SSPIUsernameCase", ap_set_string_slot,
                 (void *) APR_OFFSETOF(sspi_config_rec, sspi_usernamecase), OR_AUTHCFG,
                 "set to 'lower' if you want the username and domain to be lowercase, "
                 "set to 'upper' if you want the username and domain to be uppercase, "
                 "if not specified, username and domain case conversion is disabled"),
    AP_INIT_FLAG("SSPIBasicPreferred", ap_set_flag_slot,
                 (void *) APR_OFFSETOF(sspi_config_rec, sspi_basicpreferred), OR_AUTHCFG, 
                 "set to 'on' if you want basic authentication to be the "
                 "higher priority"),
    AP_INIT_FLAG("SSPIMSIE3Hack", ap_set_flag_slot,
                 (void *) APR_OFFSETOF(sspi_config_rec, sspi_msie3hack), OR_AUTHCFG, 
                 "set to 'on' if you expect MSIE 3 clients to be using this server"),
    AP_INIT_FLAG("SSPIPerRequestAuth", ap_set_flag_slot,
                 (void *) APR_OFFSETOF(sspi_config_rec, sspi_per_request_auth), OR_AUTHCFG, 
                 "set to 'on' if you want authorization per request instead of per connection"),
	AP_INIT_FLAG("SSPIChainAuth", ap_set_flag_slot,
                 (void *) APR_OFFSETOF(sspi_config_rec, sspi_chain_auth), OR_AUTHCFG, 
                 "set to 'on' if you want an alternative authorization module like SVNPathAuthz to work at the same level"),
    {NULL}
};

static apr_status_t sspi_module_cleanup(void *unused)
{
	UNREFERENCED_PARAMETER(unused);

	if (sspiModuleInfo.securityDLL != NULL) {
		if (sspiModuleInfo.functable != NULL) {
			sspiModuleInfo.functable->FreeContextBuffer(sspiModuleInfo.pkgInfo);
		}
		FreeLibrary(sspiModuleInfo.securityDLL);
	}

	return APR_SUCCESS;
}


static int init_module(apr_pool_t *pconf, apr_pool_t *ptemp, apr_pool_t *plog, server_rec *s)
{
	GUID userDataKey;
	OLECHAR userDataKeyString[UUID_STRING_LEN];
	LPSTR lpDllName = NULL;
	INIT_SECURITY_INTERFACE pInit;
	SECURITY_STATUS ss = SEC_E_INTERNAL_ERROR;

	if (sspiModuleInfo.lpVersionInformation == NULL) {
		/* 1. Figure out which platform we're running on. */
        sspiModuleInfo.lpVersionInformation = apr_pcalloc(pconf, sizeof(OSVERSIONINFO));
		sspiModuleInfo.lpVersionInformation->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(sspiModuleInfo.lpVersionInformation);

		/* 2. Set optimistic support flag. */
		sspiModuleInfo.supportsSSPI = TRUE;

		/* 3. Set default package. */
		sspiModuleInfo.defaultPackage = DEFAULT_SSPI_PACKAGE;

		/* 4. Generate a unique ID for connection info storage. */
		CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);
		CoCreateGuid(&userDataKey);
		StringFromGUID2(&userDataKey, userDataKeyString, UUID_STRING_LEN);
		WideCharToMultiByte(CP_ACP, 0, userDataKeyString, -1, sspiModuleInfo.userDataKeyString, UUID_STRING_LEN, NULL, NULL);
		CoUninitialize();

		/* 5. Determine the name of the security support provider DLL for this platform. */
		if (sspiModuleInfo.lpVersionInformation->dwPlatformId == VER_PLATFORM_WIN32_NT) {
			lpDllName = WINNT_SECURITY_DLL;
		} else {
			lpDllName = WIN9X_SECURITY_DLL;
		}

		/* 6. Attempt to load the support provider DLL. */
		__try {
			if (!(sspiModuleInfo.securityDLL = LoadLibrary(lpDllName))) {
				ap_log_error(APLOG_MARK, APLOG_CRIT, APR_FROM_OS_ERROR(GetLastError()), s, "%s: could not load security support provider DLL", MOD_AUTHNZ_SSPI_MODULE_NAME);
				return HTTP_INTERNAL_SERVER_ERROR;
			}

			if (!(pInit = (INIT_SECURITY_INTERFACE) GetProcAddress(sspiModuleInfo.securityDLL, SECURITY_ENTRYPOINT))) {
				ap_log_error(APLOG_MARK, APLOG_CRIT, APR_FROM_OS_ERROR(GetLastError()), s, "%s: could not locate security support provider entrypoint in DLL", MOD_AUTHNZ_SSPI_MODULE_NAME);
				return HTTP_INTERNAL_SERVER_ERROR;
			}

			if (!(sspiModuleInfo.functable = pInit())) {
				ap_log_error(APLOG_MARK, APLOG_CRIT, APR_FROM_OS_ERROR(GetLastError()), s, "%s: could not get security support provider function table from initialisation function", MOD_AUTHNZ_SSPI_MODULE_NAME);
				return HTTP_INTERNAL_SERVER_ERROR;
			}

			ss = sspiModuleInfo.functable->EnumerateSecurityPackages(&sspiModuleInfo.numPackages, &sspiModuleInfo.pkgInfo);
		} __finally {
			if (ss != SEC_E_OK) {
				sspi_module_cleanup(NULL);
				sspiModuleInfo.supportsSSPI = FALSE;
			}
		}
	}

	apr_pool_cleanup_register(pconf, NULL, sspi_module_cleanup, sspi_module_cleanup);

	ap_add_version_component(pconf, apr_psprintf(pconf, "%s/%d.%d.%d", 
		MOD_AUTHNZ_SSPI_MODULE_NAME, MOD_AUTHNZ_SSPI_VERSION_MAJOR, 
		MOD_AUTHNZ_SSPI_VERSION_MID, MOD_AUTHNZ_SSPI_VERSION_MINOR));

	return OK;
}

static const authz_provider authz_sspi_user_provider =
{
	&sspi_user_check_authorization,
	NULL,
};
static const authz_provider authz_sspi_group_provider =
{
	&sspi_group_check_authorization,
	NULL,
};
static const authz_provider authz_sspi_valid_provider =
{
	&sspi_valid_check_authorization,
	NULL,
};

#if 0
static int authenticate_sspi_user(request_rec *r)
{
	const char *current_auth;
	//authn_status auth_result;
	//authn_provider_list *current_provider;
	int res;

	/* Are we configured to be SSPI auth? */
	current_auth = ap_auth_type(r);
	if (!current_auth || strcasecmp(current_auth, "SSPI")) {
		return DECLINED;
	}

	/* ... */
	res = authenticate_sspi_user(r);

	return res;
}
#endif


static void register_hooks(apr_pool_t *p)
{
	/* Register authn provider */
	ap_hook_check_authn(authenticate_sspi_user, NULL, NULL, APR_HOOK_MIDDLE,
		AP_AUTH_INTERNAL_PER_CONF);
	//ap_register_auth_provider(p, AUTHN_PROVIDER_GROUP, "SSPI",
	//	AUTHN_PROVIDER_VERSION,
	//	&authn_sspi_check, AP_AUTH_INTERNAL_PER_CONF);

	/* Register authz providers */
	ap_register_auth_provider(p, AUTHZ_PROVIDER_GROUP, "sspi-user",
		AUTHZ_PROVIDER_VERSION,
		&authz_sspi_user_provider,
		AP_AUTH_INTERNAL_PER_CONF);
	ap_register_auth_provider(p, AUTHZ_PROVIDER_GROUP, "sspi-group",
		AUTHZ_PROVIDER_VERSION,
		&authz_sspi_group_provider,
		AP_AUTH_INTERNAL_PER_CONF);
	ap_register_auth_provider(p, AUTHZ_PROVIDER_GROUP, "valid-sspi-user",
		AUTHZ_PROVIDER_VERSION,
		&authz_sspi_valid_provider,
		AP_AUTH_INTERNAL_PER_CONF);

	ap_hook_fixups(provide_auth_headers, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_post_config(init_module,NULL,NULL,APR_HOOK_FIRST);
}

AP_DECLARE_MODULE(authnz_sspi) =
{
	STANDARD20_MODULE_STUFF,
	create_sspi_dir_config,	         /* dir config creater */
	NULL,                            /* dir merger --- default is to override */
	NULL,                            /* server config */
	NULL,                            /* merge server config */
	sspi_cmds,                       /* command apr_table_t */
	register_hooks                   /* register hooks */
};

sspi_config_rec *get_sspi_config_rec(request_rec *r)
{
	return (sspi_config_rec *)
		ap_get_module_config(r->per_dir_config, &authnz_sspi_module);
}
