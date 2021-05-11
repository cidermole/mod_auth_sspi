#!/perl/bin/perl

sub indentPrintln($$) {
    my($level, $text) = @_;

    for (my $i = 0; $i < $level; $i++) {
        print("    ");
    }

    print("$text\n");
}

@pieces = split(/\\/, $ENV{'REMOTE_USER'});
$DOMAIN = $pieces[0];
$USER = $pieces[1];

$SERVER = $ENV{'SERVER_NAME'};

print("Content-type: text/html\r\n");
print("\r\n");

indentPrintln(0, "<HTML>");
indentPrintln(1, "<HEAD>");
indentPrintln(2, "<TITLE>whoami at $SERVER</TITLE>");
indentPrintln(1, "</HEAD>");
indentPrintln(1, "<BODY>");
indentPrintln(2, "<FONT FACE=\"Verdana\" SIZE=-1>");
indentPrintln(3, "You appear to be user <B>$USER</B>, ");
indentPrintln(3, "logged into the Windows NT domain <B>$DOMAIN</B>.");
indentPrintln(2, "</FONT>");
indentPrintln(1, "</BODY>");
indentPrintln(0, "</HTML>");


