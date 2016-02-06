/*
 *   copy_usb: utility to copy an raw image to a USB device.
 *   Copyright (C) 2016  James Klassen
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <fcntl.h>
#include <linux/fs.h> /* for ioctl */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const int BLOCKSIZE=32768;

int main(int argc, char** argv)
{
	struct stat st;
	struct stat dst_st;
	int err;

	int src_fd, dst_fd;
	uint32_t vol_id;

	size_t offset;
	size_t len, len2;

	void *src_buf, *dst_buf;

	int simple_output = 0;

	if (argc < 3 || argc > 4 ||
	    ((argc == 4) && (strcmp("-b", argv[3]) != 0))) {
	  fprintf(stderr, "Usage: %s <source> <destination> [-b]\n", argv[0]);
	  fprintf(stderr, "       -b: Format progress output to be used with autostart\n");
	  return (1);
	}

	if (argc == 4) {
	  simple_output = 1;
	}
			
	src_buf = malloc(BLOCKSIZE);
	dst_buf = malloc(BLOCKSIZE);

	if((src_buf == NULL) || (dst_buf == NULL)) {
		perror(argv[0]);
		return(1);
	}	

	src_fd = open(argv[1], O_RDONLY);
	dst_fd = open(argv[2], O_RDWR|O_EXCL);
	
	if((src_fd == -1) || (dst_fd == -1)) {
		perror(argv[0]);
		return(2);
	}
	//if(flock(dst_fd, LOCK_EX|LOCK_NB)) {
	//	perror("Could not lock dest:");
	//	return(2);
	//}

	lseek(dst_fd, 0x1b8, SEEK_SET);
	read(dst_fd, &vol_id, 4);
	//vol_id = 0x3af1595d;
	if (!simple_output)
	  printf("Overwriting Volume ID: %x\n", vol_id);

	if(fstat(src_fd, &st) == -1) {
		perror(argv[0]);
		return(3);
	}
	if(S_ISBLK(st.st_mode)) {
		ioctl(src_fd, BLKGETSIZE64, &(st.st_size));
	}
	if (!simple_output)
	  printf("Source Length: %lld\n", st.st_size);

	if(fstat(dst_fd, &dst_st) == -1) {
		perror(argv[0]);
		return(3);
	}
	if(S_ISBLK(dst_st.st_mode)) {
		ioctl(dst_fd, BLKGETSIZE64, &(dst_st.st_size));
	}
	if (!simple_output)
	  printf("Destination Length: %lld\n\n", dst_st.st_size);
	if(dst_st.st_size < st.st_size) {
		fprintf(stderr,"Error: Destination not large enough for source\n.");
		return(3);
	}


	offset = 0;
	/* First block is special */
	{
		lseek(src_fd, offset, SEEK_SET);
		lseek(dst_fd, offset, SEEK_SET);

		len = read(src_fd, src_buf, BLOCKSIZE);
		if(len < 0) {
		  perror(argv[0]);
		  return(4);
		}

		len2 = read(dst_fd, dst_buf, len);
		if(len2 != len) {
		  perror(argv[0]);
		  return(5);
		}
		
		if(memcmp(src_buf, dst_buf, len) != 0) {
		  lseek(dst_fd, offset, SEEK_SET);
		  write(dst_fd, src_buf, len);
		}

		/* Restore Volume ID */	
		lseek(dst_fd, 0x1b8, SEEK_SET);
		write(dst_fd, &vol_id, 4);	
	}	

	for(offset = BLOCKSIZE; offset < st.st_size; offset += BLOCKSIZE) {
		lseek(src_fd, offset, SEEK_SET);
		lseek(dst_fd, offset, SEEK_SET);

		len = read(src_fd, src_buf, BLOCKSIZE);
		if(len < 0) {
		  perror(argv[0]);
		  return(6);
		}

		len2 = read(dst_fd, dst_buf, len);
		if(len2 != len) {
		  perror(argv[0]);
		  return(7);
		}
		
		if(memcmp(src_buf, dst_buf, len) != 0) {
			lseek(dst_fd, offset, SEEK_SET);
			write(dst_fd, src_buf, len);
		}

		if(0 == (offset % (100*1024*1024))) {
		  if(simple_output) {
		    printf("%d\n", (int)(100.0*offset/st.st_size));
		    fflush(stdout);
		  } else
		    printf("\033[1A\033[2K%d (%d %%)\n", offset/(1024*1024), (int)(100.0*offset/st.st_size));
		  fsync(dst_fd);
		}
	}

	if(simple_output) {
	  printf("E\n");
	  fflush(stdout);
	} else
	  printf("\nSyncing...\n");
	fsync(dst_fd);

	close(src_fd);
	close(dst_fd);
	return 0;
}
