This is yet another fork of Jim Brain's tcpser serial to IP modem emulation program.

The original source code can be found here:
http://www.jbrain.com/pub/linux/serial/

My changes are based upon the rc12 archive dated 11Mar09.

I fixed the bug with being unable to connect to real telnet servers.

I also made the modem routines automatically detect parity. If even,
odd, or mark parity is detected then the telnet connection will be
limited to 7 bit. Space parity will be treated as 8N1 and will allow
telnet binary mode.

I also incorporated geneb's changes found at https://github.com/geneb/tcpser

Chris Osborn <fozztexx@fozztexx.com>
http://insentricity.com
