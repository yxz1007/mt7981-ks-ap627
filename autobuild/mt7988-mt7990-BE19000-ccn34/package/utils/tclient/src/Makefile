LDFLAGS += -lpthread
all: tclient

tclient:
	$(CC) $(CFLAGS) -o $@ tclient.c anld_ctrl.c $(LDFLAGS)

clean:
	rm -f tclient