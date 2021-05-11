/* mod_authnz_sspi version header file */
#ifndef _MOD_AUTHNZ_SSPI_VER_H_
#define _MOD_AUTHNZ_SSPI_VER_H_

#define MOD_AUTHNZ_SSPI_MODULE_NAME "mod_authnz_sspi"

#define MOD_AUTHNZ_SSPI_VERSION_MAJOR 0
#define MOD_AUTHNZ_SSPI_VERSION_MID 1
#define MOD_AUTHNZ_SSPI_VERSION_MINOR 0

/** Properly quote a value as a string in the C preprocessor */
#define STRINGIFY(n) STRINGIFY_HELPER(n)
/** Helper macro for APR_STRINGIFY */
#define STRINGIFY_HELPER(n) #n

#define MOD_AUTHNZ_SSPI_VERSION_STR \
    STRINGIFY(MOD_AUTHNZ_SSPI_VERSION_MAJOR) "." \
    STRINGIFY(MOD_AUTHNZ_SSPI_VERSION_MID) "." \
    STRINGIFY(MOD_AUTHNZ_SSPI_VERSION_MINOR)

#endif /* ndef _MOD_AUTHNZ_SSPI_VER_H_ */

