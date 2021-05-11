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
#define SECURITY_WIN32
#include <windows.h>
#include <sspi.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    FARPROC pInit;
    SecurityFunctionTable *functable;
    HANDLE securityDLL;
    ULONG numpackages, ctr;
    PSecPkgInfo info;

    securityDLL = LoadLibrary("security.dll");
    if (securityDLL == NULL) {
        fprintf(stderr, "unable to load security.dll\n");
        return 1;
    }
    pInit = GetProcAddress(securityDLL, SECURITY_ENTRYPOINT);
    if (pInit == NULL) {
        fprintf(stderr, "unable to locate security.dll entrypoint\n");
        return 2;
    }
        
    functable = (SecurityFunctionTable *) pInit();
    if (functable == NULL) {
        fprintf(stderr, "unable to get function table using security.dll" 
                "entrypoint\n");
        return 3;
    }
        
    
    functable->EnumerateSecurityPackages(&numpackages, &info);
    
    for (ctr = 0; ctr < numpackages; ctr++) {
        printf("%d: %s, version %d [%s]\n", ctr + 1, info[ctr].Name, 
               info[ctr].wVersion, info[ctr].Comment);
    }

    functable->FreeContextBuffer(info);
    CloseHandle(securityDLL);
 
    return 0;
}
