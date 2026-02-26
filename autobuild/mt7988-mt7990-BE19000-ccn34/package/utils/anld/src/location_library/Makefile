C_LIB_FILES = epo/epo_download.c \
		epo/epo_host_aiding.c \
		location_service.c \
		location_command.c \
		location_io.c \

C_LIB_OBJS = $(patsubst %.c,%.o,$(C_LIB_FILES)) 

C_INCLUDE = -I../host_download/inc/ \
			-Iepo/ \
			-I./ \
			-I../utils \

all: liblocation_library.a

liblocation_library.a:$(C_LIB_OBJS)
		ar -crs $@ $^
		mv liblocation_library.a ../prebuilt/lib/
		cp location_service.h ../prebuilt/inc/

%.o:%.c
		$(CC) -c -o $@ $^ $(C_INCLUDE) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(C_LIB_OBJS)