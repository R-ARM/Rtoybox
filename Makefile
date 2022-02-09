default:
	make -C Rmenu2	CFLAGS="$(CFLAGS)"
	make -C Rmedia	CFLAGS="$(CFLAGS)"
	make -C Rdevkit	CFLAGS="$(CFLAGS)"
	make -C Rpause	CFLAGS="$(CFLAGS)"
