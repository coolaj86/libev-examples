CC_OPTS = -Wall -Werror -lev
#-L/opt/local/var/macports/software/libev/3.6_0/opt/local/lib

all: unix-echo udp-echo

udp-echo: udp-echo.c
	$(CC) $(CC_OPTS) -o udp-echo udp-echo.c

unix-echo: unix-echo.c
	$(CC) $(CC_OPTS) -o unix-echo unix-echo.c
