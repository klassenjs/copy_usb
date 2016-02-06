/*
 *   autostart: Automatically starts copy_usb on device insertion.
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

#include <alloca.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "UserInterface.h"

const char *dev_path = "/dev/disk/by-id";

struct copy_params {
  char *src_name;
  size_t src_size;
  UserInterface *ui;
} copy_params;

size_t filesize(char* filename) {
  struct stat st;
  int fd;

  fd = open(filename, O_RDONLY);
  if (fd == -1) {
    perror(__func__);
    return -1;
  }
  
  if (fstat(fd, &st) == -1) {
    perror(__func__);
    close(fd);
    return -1;
  }

  if(S_ISBLK(st.st_mode)) {
    ioctl(fd, BLKGETSIZE64, &(st.st_size));
  }

  close(fd);

  return st.st_size;
}


void copy_usb(const char *src_name, const char *dst_name)
{
  pid_t pid;
  int pipefd[2];
  
  printf("copy_usb %s %s\n", src_name, dst_name);

  if (pipe(pipefd)) {
    perror(__func__);
    exit(1);
  }

  copy_params.ui->watch_fd(dst_name, pipefd[0]);
  
  pid = fork();
  if (pid == -1) {
    perror(__func__);
    return;
  } else if (pid == 0) {
    /* Child */
    dup2(pipefd[1], 1);
    execl("./copy_usb", "./copy_usb", src_name, dst_name, "-b", NULL);
    /* Should never return unless error */
    perror(__func__);
    exit(1);
  }
}


int on_create(const struct inotify_event *evt, const char *src_name, const size_t src_size)
{
  char *dst_name;
  size_t dst_size;
  
  if (evt->mask & IN_CREATE) {
    /* Check if USB device */
    printf("CREATE %s\n", evt->name);
    if (strncmp("usb-", evt->name, 4) == 0) {
      dst_name = (char*)alloca(strlen(dev_path) + strlen(evt->name) + 2);
      sprintf(dst_name, "%s/%s", dev_path, evt->name);
      
      /* Check if correct size */
      if ((dst_size = filesize(dst_name)) == -1) {
	perror(__func__);
	return -1;
      }

      printf("Found %s size %lld\n", dst_name, dst_size);

      if (dst_size == src_size) {
	copy_usb(src_name, dst_name);
	return 1;
      }
    }
  }
  return 0;
}

void cb_watch_inotify(int fd, void* v)
{
  struct inotify_event *evt;
  struct copy_params *cp = (struct copy_params*)v;
  evt = (struct inotify_event*)alloca(sizeof(struct inotify_event) + NAME_MAX + 1);
  read(fd, evt, sizeof(struct inotify_event) + NAME_MAX + 1);
  on_create(evt, cp->src_name, cp->src_size);
}

int main(int argc, char** argv)
{
  int fd, i;
  size_t src_size;
  size_t dst_size;
  struct sigaction sa;
  int* watches;
  
  /* Read source file */
  if (argc != 2) {
    printf("Usage: %s <image to copy>\n", argv[0]);
    return 1;
  }

  if ((src_size = filesize(argv[1])) == -1) {
    return -1;
  }
  printf("Source Length: %lld\n", src_size);

  /* Create inotify watchlist for files created in /dev/disk/by-id */

  fd = inotify_init();

  if (fd == -1) {
    fprintf(stderr, "inotify_init() failed, aborting.\n");
    return 1;
  }

  if (inotify_add_watch(fd, dev_path, IN_CREATE) == -1) {
    perror("inotify_add_watch failed:");
    return 1;
  }

  /* Setup Signal Handler to cleanup after children */
  sa.sa_handler = SIG_DFL;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_NOCLDWAIT;
  sa.sa_restorer = NULL;
  sigaction(SIGCHLD, &sa, NULL); 

  /* Respond to files created */
  UserInterface ui(argc, argv);

  copy_params.src_name = argv[1];
  copy_params.src_size = src_size;
  copy_params.ui = &ui;

  Fl::add_fd(fd, FL_READ, cb_watch_inotify, (void*)&copy_params); 

  return Fl::run();
}
