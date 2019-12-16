#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_BUTTON 9

int main(int argc, char **argv)
{
	int fd;
	unsigned char switch_buffer[MAX_BUTTON] = {0,};
	unsigned char ret;

	fd = open("/dev/fpga_push_switch", O_RDWR);

	if (fd <0){
		printf("Device open error : /dev/fpga_push_switch\n");
		return -1;
	}


		
	read(fd, &switch_buffer, sizeof(switch_buffer));

	for(int i = 0; i< MAX_BUTTON ; i++){
		if(switch_buffer[i] != 0)
			ret |= 0x1 << i;

	}
	printf("Current button Value : 0x%x",ret);


	close(fd);

	return 0;
}
