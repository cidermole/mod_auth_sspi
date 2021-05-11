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
