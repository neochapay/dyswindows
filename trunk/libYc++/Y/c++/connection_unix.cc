/************************************************************************
 *   Copyright (C) Andrew Suffield <asuffield@debian.org>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <Y/c++/connection.h>

#include <cerrno>
#include <iostream>

#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>

void
Y::Connection::unixInitialise (const char *display)
{
  int r;
  struct sockaddr_un sockaddr;
  sockaddr.sun_family = AF_UNIX;
  strncpy (sockaddr.sun_path, display, 100);
  int fd = socket (PF_UNIX, SOCK_STREAM, 0);
  r = connect (fd, (struct sockaddr *)&sockaddr, sizeof (sockaddr));

  if (r != 0)
    {
      std::cerr << "Failed to connect to Y Server via Unix domain socket: " << strerror(errno) << std::endl;
      abort();
    }

  control_fd = fd;

  {
    uint32_t msg_type = ucmtAuthenticate;
    uint32_t msg_len = 0;
    struct iovec iov[] = {{&msg_len, sizeof(msg_len)}, {&msg_type, sizeof(msg_type)}};

    struct ucred creds;
    creds.uid = geteuid();
    creds.gid = getegid();
    creds.pid = getpid();
    char buf[CMSG_SPACE(sizeof creds)];
    struct msghdr msg = {0, 0,
                         iov, 2,
                         buf, sizeof(buf),
                         0};
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_CREDENTIALS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(creds));
    struct ucred *pcreds = (struct ucred *)CMSG_DATA(cmsg);
    memcpy(pcreds, &creds, sizeof(creds));
    msg.msg_controllen = cmsg->cmsg_len;

    ssize_t len = sendmsg(control_fd, &msg, 0);
    if (len == -1)
      {
        std::cerr << "Failed to send authenticate message to Y server: " << strerror(errno) << std::endl;
        abort();
      }
  }

  {
    uint32_t msg_len;
    uint32_t msg_type;
    uint32_t channel_id;
    struct iovec iov[] = {{&msg_len, sizeof(msg_len)}, {&msg_type, sizeof(msg_type)}, {&channel_id, sizeof(channel_id)}};
    char cmsgbuf[CMSG_SPACE(sizeof(struct ucred))];
    struct msghdr msg = {NULL, 0,
                         iov, 3,
                         cmsgbuf, sizeof(cmsgbuf),
                         0};
    ssize_t len = recvmsg(control_fd, &msg, 0);
    if (len == -1)
      {
        std::cerr << "Failed to read control message from Y server (during connection setup): " << strerror(errno) << std::endl;
        abort();
      }
    else if (len == 0)
      {
        std::cerr << "Connection closed by server" << std::endl;
        abort();
      }

    if (msg_type != ucmtNewChannel)
      {
        std::cerr << "Unexpected message type from server (got " << msg_type << ", wanted " << ucmtNewChannel << ")" << std::endl;
        abort();
      }

    if (msg_len != sizeof(channel_id))
      {
        std::cerr << "Malformed 'new channel' message from server" << std::endl;
        abort();
      }

    if (channel_id != 0)
      {
        std::cerr << "Malformed 'new channel' message from server (was expecting channel 0, but got " << channel_id << ")" << std::endl;
        abort();
      }

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS)
      {
        std::cerr << "Malformed 'new channel' message from server (didn't get an SOL_SOCKET/SCM_RIGHTS message)" << std::endl;
        abort();
      }

    if (cmsg->cmsg_len != CMSG_LEN(sizeof(int)))
      {
        std::cerr << "Malformed 'new channel' message from server (wrong length for SOL_SOCKET/SCM_RIGHTS message)" << std::endl;
        abort();
      }

    int *psv = (int *)CMSG_DATA(cmsg);
    server_fd = psv[0];
  }
}

/* arch-tag: c5315195-82c1-4c90-ba8c-d3dd4ec0bcad
 */
