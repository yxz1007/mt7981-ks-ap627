#include <stdio.h>
#include "memctrl.h"
#include <unistd.h>

//#define	ENABLE_DEBUG_MBW

size_t mmap_size = 0x1000;
long EMI_REG_BASE = 0;

static int usage()
{
	fprintf(stderr, "Command: mbw Type Cycle Freq emi_reg_base read_write \n");
	fprintf(stderr, "----------------------------------\n");
	fprintf(stderr, "run Type : Monitor type, 0-> time, 1-> EMI Bus cycle stop cnt\n");
	fprintf(stderr, "run Cycle : Monitor cycle \n");
	fprintf(stderr, "run Freq  : Monitor time(ms) per-cycle or BSTP cnt. Note that bw value is the average bw in one cycle.\n");
	fprintf(stderr, "emi_reg_base  : set emi base address \n");
	fprintf(stderr, "read_write  : set monitor read_only(1) or write_only(2) or read_and_write(others) \n");
	fprintf(stderr, "----------------------------------\n");
	fprintf(stderr, "Example : \n");
	fprintf(stderr, "\"mbw 0 10 500 0x10219000 0\" will choose type0(time), monitor 10 cycles and each cycle 500ms, monitor read_and_write\n");
	fprintf(stderr, "\"mbw 1 10 500*1000*1000(ns)/(10^9/(3200/8))(ns) 0x10219000 1\" will choose type1(BSTP), monitor 10 cycles and each cycle 500ms, only monitor read transaction \n");

	fprintf(stderr, "----------------------------------\n");
	return -1;
}

void showtime(struct timespec * t1, struct timespec * t0)
{
	long ms = 0;
	long s = 0;
	long s1 = 1000000000l;

	if (t1->tv_nsec < t0->tv_nsec)
	{
		ms = (s1 + t1->tv_nsec - t0->tv_nsec)/(1000*1000);
		s = t1->tv_sec - 1 - t0->tv_sec;
	}
	else
	{
		ms = (t1->tv_nsec - t0->tv_nsec)/(1000*1000);
		s = t1->tv_sec - t0->tv_sec;
	}

	fprintf(stderr, "time use : %ld.%ld s \n", s, ms);
}

unsigned long gettime(struct timespec * t1, struct timespec * t0)
{

	unsigned long ns = 0;
	unsigned long s = 0;
	unsigned long s1 = 1000000000;

	if(t1->tv_sec < t0->tv_sec) {
		fprintf(stderr, "time error! \n");
	} else {
		if (t1->tv_nsec < t0->tv_nsec)
		{
			ns = (s1 + t1->tv_nsec - t0->tv_nsec);
			s = t1->tv_sec - 1 - t0->tv_sec;
		}
		else
		{
			ns = (t1->tv_nsec - t0->tv_nsec);
			s = t1->tv_sec - t0->tv_sec;
		}
	}

	return s* 1000000000 + ns;
}


unsigned int readl(long reg) {
	return *((unsigned int*)(reg));
}
void writel(int val, long reg) {
	*((int*)(reg)) = val;
}

void enable_for_latency(void)
{
	writel(readl(REG_EMI_BMEN) & (~(0xFF<<16)), (REG_EMI_BMEN));
	writel(0, (REG_EMI_MSEL));
	writel(0, (REG_EMI_MSEL2));
	writel(0, (REG_EMI_MSEL3));
	writel(readl(REG_EMI_MSEL4) & (~0xFF), (REG_EMI_MSEL4));

	writel(readl((REG_EMI_BMEN)) | 0x1 << 16, (REG_EMI_BMEN));	//PORT0
	writel(readl((REG_EMI_MSEL)) | 0x2, (REG_EMI_MSEL));		//PORT1
	writel(readl((REG_EMI_MSEL)) | 0x4 << 16, (REG_EMI_MSEL));	//PORT2
	writel(readl((REG_EMI_MSEL2)) | 0x8, (REG_EMI_MSEL2));		//PORT3
	writel(readl((REG_EMI_MSEL2)) | 0x10 << 16, (REG_EMI_MSEL2));	//PORT4
	writel(readl((REG_EMI_MSEL3)) | 0x20, (REG_EMI_MSEL3));		//PORT5
	writel(readl((REG_EMI_MSEL3)) | 0x40 << 16, (REG_EMI_MSEL3));	//PORT6
	writel(readl((REG_EMI_MSEL4)) | 0x80, (REG_EMI_MSEL4));		//PORT7

#ifdef	ENABLE_DEBUG_MBW
	fprintf(stderr, " REG_EMI_BMEN[0x%x] REG_EMI_MSEL[0x%x] REG_EMI_MSEL2[0x%x] REG_EMI_MSEL3[0x%x] REG_EMI_MSEL4[0x%x] \n", readl(REG_EMI_BMEN), readl(REG_EMI_MSEL),
			readl(REG_EMI_MSEL2), readl(REG_EMI_MSEL3), readl(REG_EMI_MSEL4));
#endif
}

void enable(unsigned int read_write)
{
	int tmp = 0;
	int port_id = 0;
	long DBW_X = 0;
	long DBW_X_2ND = 0;

	enable_for_latency();
	writel(readl(REG_EMI_BMEN) & (~0xFFFF), (REG_EMI_BMEN));

	tmp = (readl(REG_EMI_BMEN)) & (~(0x3<<4));
	if(1==read_write) {		//read_only
		tmp = tmp | (0x1<<4);	//read cmd
	} else if(2==read_write) {	//write_only
		tmp = tmp | (0x1<<5);	//write cmd
	} else {			//read_write
		tmp = tmp | (0x3<<4);
	}
	writel(tmp, (REG_EMI_BMEN));

	writel(0, (REG_EMI_DBWA));
	writel(0, (REG_EMI_DBWB));
	writel(0, (REG_EMI_DBWC));
	writel(0, (REG_EMI_DBWD));
	writel(0, (REG_EMI_DBWE));
	writel(0, (REG_EMI_DBWF));

	writel(0, (REG_EMI_DBWA_2ND));
	writel(0, (REG_EMI_DBWB_2ND));
	writel(0, (REG_EMI_DBWC_2ND));
	writel(0, (REG_EMI_DBWD_2ND));
	writel(0, (REG_EMI_DBWE_2ND));
	writel(0, (REG_EMI_DBWF_2ND));

	for(port_id=0;port_id < 8;port_id++) {
		if((port_id == 0x0) || (port_id == 0x1)) {
			DBW_X = REG_EMI_DBWA;
			DBW_X_2ND = REG_EMI_DBWA_2ND;
		} else if(port_id == 0x2) {
			DBW_X = REG_EMI_DBWB;
			DBW_X_2ND = REG_EMI_DBWB_2ND;
		} else if((port_id == 0x3) || (port_id == 0x7)) {
			DBW_X = REG_EMI_DBWF;
			DBW_X_2ND = REG_EMI_DBWF_2ND;
		} else {
			DBW_X = REG_EMI_DBWA + 4*(port_id-2);
			DBW_X_2ND = REG_EMI_DBWA_2ND + 4*(port_id-2);
		}
		tmp = readl(DBW_X);
		tmp = tmp   | ((0x1<<port_id)<<8)      //select port ID
			| (0x1<<4)      //disable boundary limitation, monitor all type
			| (0x1<<2)      //monitor normal priority
			| (0x1<<1)      //write cmd
			| (0x1<<0);     //read cmd

		tmp = tmp   & (~(0x01<<3)); //disable ID selection, monitor all transaction ID type

		if(1==read_write) {		//read_only
			tmp = tmp & (~(0x3));
			tmp = tmp | (0x1<<0);	//read cmd
		} else if(2==read_write) {	//write_only
			tmp = tmp & (~(0x3));
			tmp = tmp | (0x1<<1);	//write cmd
		} else {			//read_write
			tmp = tmp | 0x3;
		}
		writel(tmp, DBW_X);

		tmp = readl(DBW_X_2ND);
		tmp = tmp   | (0xF<<28)	//monitor high priority
			| (0xF<<12)	//select all channel,all rank
			| (0x1FF<<0);	//byte up boundary, select all bytes

		tmp = tmp   & (~(0xFF<<16));//byte low boundary, select all bytes

		writel(tmp, DBW_X_2ND);
	}

#ifdef	ENABLE_DEBUG_MBW
	fprintf(stderr, " REG_EMI_DBWX value:[0x%x] [0x%x] [0x%x] [0x%x] [0x%x] [0x%x]\n", readl(REG_EMI_DBWA), readl(REG_EMI_DBWB),
			readl(REG_EMI_DBWC), readl(REG_EMI_DBWD), readl(REG_EMI_DBWE), readl(REG_EMI_DBWF));
	fprintf(stderr, " REG_EMI_DBWX_2ND value:[0x%x] [0x%x] [0x%x] [0x%x] [0x%x] [0x%x]\n", readl(REG_EMI_DBWA_2ND), readl(REG_EMI_DBWB_2ND),
			readl(REG_EMI_DBWC_2ND), readl(REG_EMI_DBWD_2ND), readl(REG_EMI_DBWE_2ND), readl(REG_EMI_DBWF_2ND));
#endif
}

void print(unsigned int type, unsigned counter, unsigned long ms_bstp)
{
	struct timespec start_timer_value;
	struct timespec stop_timer_value;
	unsigned total_count = counter;
	unsigned long long bw, bw0, bw1, bw2, bw3, bw4, bw5;
	unsigned long long counter_value, counter_value0, counter_value1, counter_value2, counter_value3, counter_value4, counter_value5;
	unsigned long timer_value;
	unsigned long long latency0, latency1, latency2, latency3;
	unsigned long long latency4, latency5, latency6, latency7;
	unsigned long long transaction0, transaction1, transaction2, transaction3;
	unsigned long long transaction4, transaction5, transaction6, transaction7;

	//disable DCM -> 0x10205060[31:24] = 0xff;
	writel((readl(REG_EMI_CONM) | (0xff << 24)), REG_EMI_CONM);
	writel((readl(REG_EMI_CONN) | (0xff << 24)), REG_EMI_CONN);

	writel((readl(REG_EMI_BMEN) | (0x1 << 3)), REG_EMI_BMEN);//disable idle

	while (counter--)
	{
		bw5 = bw4 = bw3 = bw2 = bw1 = bw = 0;

		//Clear Register 0x10205400[0] = 0;
		writel(readl((REG_EMI_BMEN)) & ~0x1, (REG_EMI_BMEN));
		writel(readl((REG_EMI_BMEN2)) & ~(1<<25), (REG_EMI_BMEN2));

		//Un Pause Counter -> 0x10205400[1] = 0;
		writel(readl((REG_EMI_BMEN)) & ~0x2, (REG_EMI_BMEN));

		clock_gettime(CLOCK_MONOTONIC, &start_timer_value);

		writel(readl((REG_EMI_BMEN2)) | (1 << 25), (REG_EMI_BMEN2));

		if(0 == type) { //time
			//Start Counter -> 0x10205400[0] = 1;
			writel(readl((REG_EMI_BMEN)) | 0x1, (REG_EMI_BMEN));
			usleep(ms_bstp*1000);
			//Pause Counter -> 0x10205400[1] = 1;
			writel(readl((REG_EMI_BMEN)) | 0x2, (REG_EMI_BMEN));
		} else {    //bstp
			writel(ms_bstp, REG_EMI_BSTP);
			//Start Counter -> 0x10205400[0] = 1;
			writel(readl((REG_EMI_BMEN)) | 0x1, (REG_EMI_BMEN));
			fprintf(stderr, "polling check (EMI_BMEN >> 2):[%d],BSTP:[0x%x] \n", ((readl(REG_EMI_BMEN)>>2) & 0x1), readl(REG_EMI_BSTP));
			while(0x4 != (readl(REG_EMI_BMEN)& 0x4)); //polling check stop status
			fprintf(stderr, "polling check done \n");
		}

		clock_gettime(CLOCK_MONOTONIC, &stop_timer_value);

		//Read counter_value ->  0x10205420
		counter_value = readl(REG_EMI_WACT);
		counter_value0 = readl(REG_EMI_WSCT);
		counter_value1 = readl(REG_EMI_WSCT2);
		counter_value2 = readl(REG_EMI_WSCT3);
		counter_value3 = readl(REG_EMI_WSCT4);
		counter_value4 = readl(REG_EMI_DBWG);
		counter_value5 = readl(REG_EMI_DBWH);

		latency0 = readl(REG_EMI_EMITYPE1);
		latency1 = readl(REG_EMI_EMITYPE2);
		latency2 = readl(REG_EMI_EMITYPE3);
		latency3 = readl(REG_EMI_EMITYPE4);

		latency4 = readl(REG_EMI_EMITYPE5);
		latency5 = readl(REG_EMI_EMITYPE6);
		latency6 = readl(REG_EMI_EMITYPE7);
		latency7 = readl(REG_EMI_EMITYPE8);

		transaction0 = readl(REG_EMI_EMITYPE9);
		transaction1 = readl(REG_EMI_EMITYPE10);
		transaction2 = readl(REG_EMI_EMITYPE11);
		transaction3 = readl(REG_EMI_EMITYPE12);

		transaction4 = readl(REG_EMI_EMITYPE13);
		transaction5 = readl(REG_EMI_EMITYPE14);
		transaction6 = readl(REG_EMI_EMITYPE15);
		transaction7 = readl(REG_EMI_EMITYPE16);

		//printk("bandwidth is %ld\n",counter_value);
		if (0xffffffff != counter_value)
		{
			//pr_info("Counter value is %ld\n",counter_value);
		}
		else
		{
			printf("Counter value is OVERFLOW\n");
		}

		timer_value = gettime(&stop_timer_value, &start_timer_value)/(1000*1000); //unit:ms

		//calc bandwidth = counter_value *8  / timer_value
		if(timer_value != 0) {
			bw = ((counter_value * 8 / timer_value) * 1000) / (1024*1024);

			bw0 = ((counter_value0 * 8 / timer_value) * 1000) / (1024*1024);

			bw1 = ((counter_value1 * 8 / timer_value) * 1000) / (1024*1024);

			bw2 = ((counter_value2 * 8 / timer_value) * 1000) / (1024*1024);

			bw3 = ((counter_value3 * 8 / timer_value) * 1000) / (1024*1024);

			bw4 = ((counter_value4 * 8 / timer_value) * 1000) / (1024*1024);

			bw5 = ((counter_value5 * 8 / timer_value) * 1000) / (1024*1024);
		} else {
			fprintf(stderr, "!!! timer_value == 0, continue \n");
			continue;
		}

		fprintf(stderr, "\nThis is %d/%d test !\n", total_count - counter, total_count);
#ifdef	ENABLE_DEBUG_MBW
		fprintf(stderr, "counter_value:%llu\n", counter_value);
		fprintf(stderr, "counter_value0:%llu\n", counter_value0);
		fprintf(stderr, "counter_value1:%llu\n", counter_value1);
		fprintf(stderr, "counter_value2:%llu\n", counter_value2);
		fprintf(stderr, "counter_value3:%llu\n", counter_value3);
		fprintf(stderr, "counter_value2:%llu\n", counter_value4);
		fprintf(stderr, "counter_value3:%llu\n", counter_value5);
		fprintf(stderr, "timer_value:%lu\n", timer_value);
		fprintf(stderr, "REG_EMI_BMEN:0x%x\n", readl((REG_EMI_BMEN)));
		fprintf(stderr, "gettime(&stop_timer_value, &start_timer_value):_____%ld______\n", gettime(&stop_timer_value, &start_timer_value));
#endif

#if 1
		fprintf(stderr, "----------------------------------\n");
		fprintf(stderr, "Total bandwidth is %llu MB/s\n", bw);
		fprintf(stderr, "----------------------------------\n");
		fprintf(stderr, "Port bandwidth:\n");

		fprintf(stderr, "Port 0 & port 1:	%llu MB/s\n", bw0);
		fprintf(stderr, "Port 2:			%llu MB/s\n", bw1);
		fprintf(stderr, "Port 4:			%llu MB/s\n", bw2);
		fprintf(stderr, "Port 5:			%llu MB/s\n", bw3);
		fprintf(stderr, "Port 6:			%llu MB/s\n", bw4);
		fprintf(stderr, "Port 3 & port 7:	%llu MB/s\n\n", bw5);
#endif

#ifdef	ENABLE_DEBUG_MBW
		fprintf(stderr, "----------------------------------\n");
		fprintf(stderr, "latency0:%llu\n", latency0);
		fprintf(stderr, "latency1:%llu\n", latency1);
		fprintf(stderr, "latency2:%llu\n", latency2);
		fprintf(stderr, "latency3:%llu\n", latency3);
		fprintf(stderr, "latency4:%llu\n", latency4);
		fprintf(stderr, "latency5:%llu\n", latency5);
		fprintf(stderr, "latency6:%llu\n", latency6);
		fprintf(stderr, "latency7:%llu\n", latency7);

		fprintf(stderr, "transaction0:%llu\n", transaction0);
		fprintf(stderr, "transaction1:%llu\n", transaction1);
		fprintf(stderr, "transaction2:%llu\n", transaction2);
		fprintf(stderr, "transaction3:%llu\n", transaction3);
		fprintf(stderr, "transaction4:%llu\n", transaction4);
		fprintf(stderr, "transaction5:%llu\n", transaction5);
		fprintf(stderr, "transaction6:%llu\n", transaction6);
		fprintf(stderr, "transaction7:%llu\n", transaction7);
#endif

#if 1
		fprintf(stderr, "----------------------------------\n");
		fprintf(stderr, "Port latency:\n");
		fprintf(stderr, "Port 0:	%llu T\t\tPort 1:	%llu T \n", latency0 / (transaction0 ? transaction0 : 1), latency1 / (transaction1 ? transaction1 : 1));
		fprintf(stderr, "Port 2:	%llu T\t\tPort 3:	%llu T \n", latency2 / (transaction2 ? transaction2 : 1), latency3 / (transaction3 ? transaction3 : 1));
		fprintf(stderr, "Port 4:	%llu T\t\tPort 5:	%llu T \n", latency4 / (transaction4 ? transaction4 : 1), latency5 / (transaction5 ? transaction5 : 1));
		fprintf(stderr, "Port 6:	%llu T\t\tPort 7:	%llu T \n", latency6 / (transaction6 ? transaction6 : 1), latency7 / (transaction7 ? transaction7 : 1));
#endif
	}

	fprintf(stderr, "----------------------------------\n");

	return ;
}

int main (int argc, char *argv[])
{
	struct timespec t0;
	struct timespec t1;

	unsigned int type = 0;
	unsigned int cycle = 1;
	unsigned long freq = 1000;
	unsigned int read_write = 0; //default read_and_writee, 1:read_only, 2:write_only, others,read_and_write

	if(argc == 1) return usage();

	if (argc >= 2)
	{
		int fd = open("/dev/mem", O_RDWR | O_SYNC);
		if (fd < 0) {
			fprintf(stderr, "cannot open /dev/mem\n");
			return -1;
		}

		type = strtoul(argv[1], 0, 10);

		cycle = strtoul(argv[2], 0, 10);

		freq = strtoul(argv[3], 0, 16);

		EMI_IO_BASE = (strtoul(argv[4], 0, 16));

		read_write = strtoul(argv[5], 0, 10);

		if (argc < 6)
			fprintf(stderr, "Parameter is not enough\n");

		EMI_REG_BASE = (long)mmap(0, mmap_size, PROT_READ | PROT_WRITE,
				MAP_SHARED, fd, EMI_IO_BASE);

		if ((void*)EMI_REG_BASE == MAP_FAILED) {
			fprintf(stderr, "cannot mmap region\n");
			return -1;
		}

		enable(read_write);

		clock_gettime(CLOCK_MONOTONIC, &t0);

		print(type, cycle, freq);

		clock_gettime(CLOCK_MONOTONIC, &t1);
		//fprintf(stderr, "%ld-%ld \n", t1.tv_sec, t1.tv_nsec);

		munmap((void*)EMI_REG_BASE, mmap_size);

		showtime(&t1, &t0);

		close(fd);
	}
	return 0;
}
