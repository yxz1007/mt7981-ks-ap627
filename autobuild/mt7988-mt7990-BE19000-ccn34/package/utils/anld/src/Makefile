C_FILES = anld.c \
		utils/location_gpio.c \
		utils/location_power.c \
		utils/location_timer.c \
		utils/location_uart.c \
		utils/location_spi.c \
		utils/location_debug.c \
		utils/location_config.c \
		utils/location_serial_port.c \
		utils/host_mux_spi/src/host_mux.c \
		utils/host_mux_spi/src/host_mux_spi_master.c \
		utils/host_mux_spi/src/port_platform_interface.c \
		host_download/src/config_parser.c \
		host_download/src/crc32.c \
		host_download/src/hdl_brom_base.c \
		host_download/src/hdl_brom_demo.c \
		host_download/src/hdl_da_cmd.c \
		host_download/src/hdl_uart_linux.c \
		host_download/src/hdl_spi_linux.c \
		host_download/src/time_utils.c \
		host_download/src/pool.c \
		host_download/src/utils.c \

C_OBJS = $(patsubst %.c,%.o,$(C_FILES)) 

C_INC = -Iutils \
		-Iprebuilt/inc/ \
		-Ihost_download/inc/ \
		-Iutils/host_mux_spi/inc/ \

LDFLAGS += -lpthread -lrt
all: anld

anld:$(C_OBJS) prebuilt/lib/liblocation_library.a
		$(CC) -o $@ $^ $(C_INC) $(CFLAGS) $(LDFLAGS)

%.o:%.c
#for library compile
		cd location_library; make
		$(CC) -c -o $@ $^ $(C_INC) $(CFLAGS)

clean:
		rm -f anld
		rm -f $(C_OBJS)
		rm -f prebuilt/lib/liblocation_library.a
		cd location_library; make clean