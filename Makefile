CC_OPTS = -Wall -Werror -lev

all: unix-echo udp-echo

udp-echo: udp-echo.c
	$(CC) $(CC_OPTS) -o udp-echo udp-echo.c

unix-echo: unix-echo.c
	$(CC) $(CC_OPTS) -o unix-echo unix-echo.c
