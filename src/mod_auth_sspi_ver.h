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
#ifndef _MOD_AUTH_SSPI_VER_H_
#define _MOD_AUTH_SSPI_VER_H_

#define MOD_AUTH_SSPI_MODULE_NAME "mod_auth_sspi"

#define MOD_AUTH_SSPI_VERSION_MAJOR 1
#define MOD_AUTH_SSPI_VERSION_MID 0
#define MOD_AUTH_SSPI_VERSION_MINOR 4

/** Properly quote a value as a string in the C preprocessor */
#define STRINGIFY(n) STRINGIFY_HELPER(n)
/** Helper macro for APR_STRINGIFY */
#define STRINGIFY_HELPER(n) #n

#define MOD_AUTH_SSPI_VERSION_STR \
    STRINGIFY(MOD_AUTH_SSPI_VERSION_MAJOR) "." \
    STRINGIFY(MOD_AUTH_SSPI_VERSION_MID) "." \
    STRINGIFY(MOD_AUTH_SSPI_VERSION_MINOR)

#endif /* ndef _MOD_AUTH_SSPI_VER_H_ */

