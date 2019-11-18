#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define PROGRAM_USAGE_STRING "usage: %s [ex)20160237 ,MAXIMUM length 100]\n"

#define DOTM_MAGIC		0xBC
#define DOTM_SET_ALL		_IOW(DOTM_MAGIC, 0, int)
#define DOTM_SET_CLEAR		_IOW(DOTM_MAGIC, 1, int)
#define DOTM_RIGHT_SHIFT	_IOW(DOTM_MAGIC, 2, int)
#define DOTM_LEFT_SHIFT		_IOW(DOTM_MAGIC, 3, int)
#define DOTM_KOR_PRINT		_IOW(DOTM_MAGIC, 4, int)
#define DOTM_ENG_PRINT		_IOW(DOTM_MAGIC, 5, int)


int main(int argc, char **argv)
{
	int fd, i;
	unsigned char buffer[100] ={0, };

	if (argc <= 1) {
		fprintf(stderr, PROGRAM_USAGE_STRING, argv[0]);
		return -1;
	}
	
	if(strlen(argv[1]) > 100){
		fprintf(stderr, PROGRAM_USAGE_STRING, argv[0]);
		return -1;
	}

	//insert buffer
	for(i = 0; i< strlen(argv[1]);i++){
		buffer[i] = (unsigned char) argv[1][i] - '0';
	}

	fd = open("/dev/dotmatrix", O_WRONLY);

	if (fd != -1){
		//initalize
		ioctl(fd, DOTM_SET_ALL, NULL, _IOC_SIZE(DOTM_SET_ALL));
		usleep(200000);

		write(fd, buffer, sizeof(unsigned char)*strlen(argv[1]));
		usleep(200000);

		
		for(i = 0; i< 56; i++){
			ioctl(fd, DOTM_LEFT_SHIFT, NULL, _IOC_SIZE(DOTM_LEFT_SHIFT));
		}

		ioctl(fd, DOTM_KOR_PRINT, NULL, _IOC_SIZE(DOTM_KOR_PRINT));
		usleep(200000);
		ioctl(fd, DOTM_ENG_PRINT, NULL, _IOC_SIZE(DOTM_ENG_PRINT));
		usleep(200000);
		ioctl(fd, DOTM_SET_CLEAR, NULL, _IOC_SIZE(DOTM_SET_CLEAR));
		usleep(200000);

		close(fd);
	} else {
		fprintf(stderr, "error opening device\n");
		return -1;
	}

	return 0;
}
