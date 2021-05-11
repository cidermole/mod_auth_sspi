/* mod_authnz_sspi common header file */
#ifndef _MOD_authnz_sspi_H_
#define _MOD_authnz_sspi_H_

/* Preprocessor macro definitions */
#define WIN32_LEAN_AND_MEAN 1
#define SECURITY_WIN32 1
#define WINNT_SECURITY_DLL "SECURITY.DLL"
#define WIN9X_SECURITY_DLL "SECUR32.DLL"
#define DEFAULT_SSPI_PACKAGE "NTLM"
#define UUID_STRING_LEN 64

#define _WIN32_WINNT 0x0400

/* Version information */
#include "mod_authnz_sspi_ver.h"

/* System headers */
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

//#include <windows.h>
#include <objbase.h>
//#include <winsock2.h>
#include <sspi.h>
#include <security.h>

/* HTTPD headers */
#include "ap_config.h"
#include "apr_base64.h"
#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"
#include "apr_tables.h"
#include "apr_strings.h"
#include "mod_auth.h"
#include "ap_provider.h"

/* These might not be available with older Platform SDK */
#ifndef SecureZeroMemory
#define SecureZeroMemory(p,s) memset(p,0,s)
#endif
#ifndef _strlwr_s
#define _strlwr_s(p,s) strlwr(p)
#endif
#ifndef _strupr_s
#define _strupr_s(p,s) strupr(p)
#endif

#define SSPILOGNO(n)              "SSPI" #n ": "

/* Type and struct definitions */
typedef struct sspi_module_struct {
    BOOL supportsSSPI;
    LPSTR defaultPackage;
    LPOSVERSIONINFO lpVersionInformation;
    char userDataKeyString[UUID_STRING_LEN];
    HMODULE securityDLL;
    SecurityFunctionTable *functable;
    ULONG numPackages;
    PSecPkgInfo pkgInfo;
#ifdef _DEBUG
    unsigned int currentlyDebugging;
#endif /* def _DEBUG */
} sspi_module_rec;

typedef struct sspi_connection_struct {
    unsigned int have_credentials;

    /* Credentials */
    CredHandle client_credentials;
    CredHandle server_credentials;
    
    /* Client context */
    CtxtHandle client_context;
    TimeStamp client_ctxtexpiry;
    
    /* Server context */
    CtxtHandle server_context;
    TimeStamp server_ctxtexpiry;
    
    /* Information about the REMOTE_USER */
    HANDLE usertoken;
    char *username;
    apr_table_t *groups;
    char *package;
    int sspi_failing;
} sspi_connection_rec;

typedef struct sspi_config_struct {
    unsigned int sspi_on;
    unsigned int sspi_authoritative;
    unsigned int sspi_offersspi;
    unsigned int sspi_offerbasic;
    unsigned int sspi_omitdomain;
    unsigned int sspi_basicpreferred;
    unsigned int sspi_msie3hack;
    char *sspi_package_basic;
    char *sspi_domain;
    char *sspi_usernamecase;
    unsigned int sspi_per_request_auth;
	unsigned int sspi_chain_auth;
    char *sspi_packages;
} sspi_config_rec;

typedef enum {
    typeSSPI = 1,
    typeBasic
} NTLMAuthType;

typedef struct sspi_header_struct {
  unsigned char *User;
  unsigned long UserLength;
  unsigned char *Domain;
  unsigned long DomainLength;
  unsigned char *Password;
  unsigned long PasswordLength;
  unsigned long Flags;
  NTLMAuthType authtype;
} sspi_header_rec;

typedef struct sspi_auth_ctx_struct {
  request_rec* r;
  sspi_config_rec* crec;
  sspi_connection_rec* scr;
  sspi_header_rec hdr;
} sspi_auth_ctx;

/* Function Prototypes */
int authenticate_sspi_user(request_rec * ) ;
int provide_auth_headers(request_rec * ) ;
authz_status sspi_user_check_authorization(request_rec *,
                                                 const char *,
                                                 const void *);
authz_status sspi_group_check_authorization(request_rec *,
                                                  const char *,
                                                  const void *);
authz_status sspi_valid_check_authorization(request_rec *,
                                                  const char *,
                                                  const void *);
apr_status_t cleanup_sspi_connection(void *);

void* create_sspi_dir_config(apr_pool_t *, char *);
void* create_sspi_server_config(apr_pool_t *p, server_rec *s);
void* merge_sspi_dir_config(apr_pool_t *p, void *base_conf, void *new_conf);

sspi_config_rec *get_sspi_config_rec(request_rec *);
int get_sspi_header(sspi_auth_ctx* ctx);
void note_sspi_auth_challenge(sspi_auth_ctx* ctx, const char *);
void note_sspi_auth_failure(request_rec *r);
void note_sspi_auth_failure_old(request_rec *);
char *uuencode_binary(apr_pool_t *, const char *, int);
unsigned char *uudecode_binary(apr_pool_t *, const char *, int *);

/* Global variables */
extern sspi_module_rec sspiModuleInfo;

#endif /* ndef _MOD_authnz_sspi_H_ */

