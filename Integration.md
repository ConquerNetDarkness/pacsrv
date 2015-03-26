# Introduction #

Pacsrv is a server and client component, and needs to be integrated with pacman as it really doesn't do anything productive by itself.


# Details #

Pacsrv can be integrated with pacman without needing any modifications to pacman itself.  There is a parameter in the pacman config file which indicates which program to use to download the files.  Instead of using the internal FTP mechanism, or wget, we use the pacman client.

It may be possible, or desired, in the future to add the pacsrvclient code directly in pacman.  This will allow it to attempt to download more than one package at a time, which in theory should be good for large packages that might have a small distribution, or maybe is very popular and swamped.