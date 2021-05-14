# mod_authnz_sspi

Single sign on plugin for Windows.

Modified to provide group information.

usage in httpd.conf:
```

# require any SSPI authenticated user (Jenkins will check the rest)
Require valid-sspi-user

# pass user and group information to Jenkins
RequestHeader set X-Forwarded-User expr=%{REMOTE_USER}
RequestHeader set X-Forwarded-Groups %{REMOTE_GROUPS}e
```

Based on r17 of <https://sourceforge.net/p/mod-auth-sspi/code/HEAD/tree/branches/mod_authnz_sspi/mod_authnz_sspi/>.

```
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * This code is copyright 1999 - 2002 Tim Costello <tim@syneapps.com>
 * It may be freely distributed, as long as the above notices are reproduced.
 *
 * modifications by SourceForce user ennnnnio.
 * modifications by David Madl <git@abanbytes.eu>
```
