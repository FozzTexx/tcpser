This is yet another fork of Jim Brain's tcpser serial to IP modem emulation program.
The original source code can be found here:
http://www.jbrain.com/pub/linux/serial/

My changes are based upon the rc12 archive dated 11Mar09.

I fixed the bug with being unable to connect to real telnet servers.

I also made the modem routines automatically detect parity and ignore
it in AT commands and print out modem responses in matching
parity. Parity is *not* stripped when sending data over the
connection, which is how a real modem behaves. This may or may not be
what you want. Some servers will expect an 8 bit connection and may
not work.

Chris Osborn <fozztexx@fozztexx.com>
http://insentricity.com
