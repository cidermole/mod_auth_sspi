<html>
<head>
  <title>whoami at <?php $_SERVER['SERVER_NAME']; ?> </title>
</head>
<body style='font-family:Verdana;font-size:-1'>
<?php

$cred = explode('\\',$_SERVER['REMOTE_USER']);
if (count($cred) == 1) array_unshift($cred, "(no domain info - perhaps SSPIOmitDomain is On)");
list($domain, $user) = $cred;

echo "You appear to be user <B>$user</B><BR/>";
echo "logged into the Windows NT domain <B>$domain</B>";

?>
</body>
</html>

