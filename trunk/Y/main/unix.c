/************************************************************************
 *   Copyright (C) Andrew Suffield <asuffield@debian.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <Y/main/unix.h>
#include <Y/const.h>

#include <Y/message/message.h>
#include <Y/message/client.h>
#include <Y/message/client_p.h>

#include <Y/util/yutil.h>
#include <Y/util/dbuffer.h>
#include <Y/util/log.h>
#include <Y/util/llist.h>
#include <Y/util/rbtree.h>

#include <Y/main/control.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <netinet/in.h>

static char *socket_path = NULL;
static int listen_fd = -1;

struct unixControlMessage
{
  enum YUnixControlMessageType type;
  int fd;
  uint32_t id;
};

struct unixClient
{
  struct Client client;
  int control_fd;
  bool authenticated;
  uid_t uid;
  gid_t gid;
  pid_t pid;
  struct rbtree *channels;
  uint32_t next_channel;
  struct llist *control_queue;
};

struct unixChannel
{
  uint32_t id;
  int fd;
  struct unixClient *client;
  struct dbuffer *sendq;
};

static void unixClose (struct Client *c);
static void unixWriteData (struct Client *self_c, uint32_t channel_id, const char *data, size_t len);
static bool unixNewChannel (struct Client *self_c, uint32_t *channel_id);

struct ClientClass unixClientClass =
{
  name: "Unix Domain Socket Client",
  newChannel: unixNewChannel,
  writeData: unixWriteData,
  close: unixClose
};

static struct unixClient *
castBack (struct Client *self_c)
{
  /* assert ( self_c -> c == &unixClientClass ); */
  return (struct unixClient *)self_c;
}

static struct unixChannel *
unixChannelCreate(struct unixClient *self, uint32_t id, int fd)
{
  struct unixChannel *channel = ymalloc(sizeof(*channel));
  channel->id = id;
  channel->fd = fd;
  channel->client = self;
  channel->sendq = new_dbuffer();
  return channel;
}

static int
unixChannelKey(const void *key_v, const void *obj_v)
{
  const uint32_t *key = key_v;
  const struct unixChannel *channel = obj_v;
  return *key - channel->id;
}

static int
unixChannelCmp(const void *obj1_v, const void *obj2_v)
{
  const struct unixChannel *channel1 = obj1_v;
  const struct unixChannel *channel2 = obj2_v;
  return channel2->id - channel1->id;
}

static void
unixChannelDestroy(struct unixChannel *channel)
{
  if (!channel)
    return;
  controlUnregisterFileDescriptor(channel->fd);
  close(channel->fd);
  free_dbuffer(channel->sendq);
  yfree(channel);
}

static struct unixControlMessage *
unixControlMessageNewChannel(uint32_t id, int fd)
{
  struct unixControlMessage *msg = ymalloc(sizeof(*msg));
  msg->type = ucmtNewChannel;
  msg->id = id;
  msg->fd = fd;
  return msg;
}

static void
unixControlMessageDestroy(struct unixControlMessage *msg)
{
  if (!msg)
    return;
  switch(msg->type)
    {
    case ucmtNewChannel:
      close(msg->fd);
      break;
    case ucmtAuthenticate:
      break;
    }
  yfree(msg);
}

static bool
unixNewChannel(struct Client *self_c, uint32_t *channel_id)
{
  struct unixClient *self = castBack (self_c);
  assert(self->authenticated);

  uint32_t id = self->next_channel++;
  int sv[2];
  int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sv);
  if (ret == 0)
    {
      rbtree_insert(self->channels, unixChannelCreate(self, id, sv[0]));
      llist_add_tail(self->control_queue, unixControlMessageNewChannel(id, sv[1]));
      *channel_id = id;
      controlChangeFileDescriptorMask (self->control_fd,
                                       CONTROL_WATCH_WRITE | CONTROL_WATCH_READ);
      return true;
    }

  Y_WARN ("Failed to create unix socket pair: %s", strerror(errno));
  return false;
}

static struct unixChannel *
unixFindChannel(struct unixClient *self, uint32_t id)
{
  struct rbtree_node *node = rbtree_find(self->channels, &id);
  struct unixChannel *channel = rbtree_node_data(node);
  return channel;
}

static void
unixWriteData (struct Client *self_c, uint32_t channel_id, const char *data, size_t len)
{
  struct unixClient *self = castBack (self_c);
  assert(self->authenticated);

  struct unixChannel *channel = unixFindChannel(self, channel_id);
  assert(channel);

  dbuffer_add(channel->sendq, data, len);

  if (dbuffer_len(channel->sendq) > 0)
    controlChangeFileDescriptorMask (channel->fd,
                                     CONTROL_WATCH_WRITE | CONTROL_WATCH_READ );
}

static void
doChannelRead(struct unixClient *self, struct unixChannel *channel)
{
  char buf[4096];

  ssize_t ret = read(channel->fd, buf, sizeof(buf));
  if (ret < 0)
    {
      int e = errno;
      if (e == EINTR || e == EAGAIN)
        return;
      Y_TRACE ("Read error from client %d: %s", clientGetID(&self->client), strerror(e));
      clientClose (&(self -> client));
      return;
    }

  if (ret == 0)
    {
      Y_TRACE ("Connection closed by client %d", clientGetID(&self->client));
      clientClose (&(self -> client));
      return;
    }

  clientReadData(&self->client, channel->id, buf, ret);
}

static void
doChannelWrite(struct unixClient *self, struct unixChannel *channel)
{
  char buf[4096];
  size_t len = dbuffer_get(channel->sendq, buf, sizeof(buf));

  ssize_t ret = write(channel->fd, buf, len);
  if (ret < 0)
    {
      int e = errno;
      if (e == EINTR || e == EAGAIN)
        return;
      Y_TRACE ("Write error from client %d: %s", clientGetID(&self->client), strerror(e));
      clientClose (&(self -> client));
      return;
    }

  dbuffer_remove(channel->sendq, ret);

  if (dbuffer_len(channel->sendq) == 0)
    controlChangeFileDescriptorMask (channel->fd, CONTROL_WATCH_READ);
}

static void
unixChannelReady (int fd, int causeMask, void *data_v)
{
  struct unixChannel *channel = data_v;
  struct unixClient *self = channel->client;
  assert(channel->fd == fd);
  if (causeMask & CONTROL_WATCH_READ)
    doChannelRead(self, channel);
  if (causeMask & CONTROL_WATCH_WRITE)
    doChannelWrite(self, channel);
}

static void
doControlRead(struct unixClient *self)
{
  uint32_t msg_len;
  uint32_t msg_type;
  char buf[4096];
  struct iovec iov[] = {{&msg_len, sizeof(msg_len)}, {&msg_type, sizeof(msg_type)}, {buf, sizeof(buf)}};
  char cmsgbuf[CMSG_SPACE(sizeof(struct ucred))];
  struct msghdr msg = {NULL, 0,
                       iov, 3,
                       cmsgbuf, sizeof(cmsgbuf),
                       0};
  ssize_t len = recvmsg(self->control_fd, &msg, 0);
  if (len == -1)
    {
      if (errno == EINTR || errno == EAGAIN)
        return;
      Y_TRACE ("Read error from client %d: %s", clientGetID(&self->client), strerror(errno));
      clientClose (&(self -> client));
      return;
    }
  else if (len == 0)
    {
      Y_TRACE ("Connection closed by client %d", clientGetID(&self->client));
      clientClose (&(self -> client));
      return;
    }

  enum YUnixControlMessageType type = msg_type;
  switch(type)
    {
    case ucmtNewChannel:
      Y_TRACE ("Bogus 'new channel' control message from client %d, discarding", clientGetID(&self->client));
      break;
    case ucmtAuthenticate:
      {
        if (msg_len != 0)
          {
            Y_TRACE ("Malformed 'authenticate' control message from client %d, dropping connection", clientGetID(&self->client));
            clientClose(&self->client);
          }

        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_CREDENTIALS)
          {
            Y_TRACE ("Invalid 'authenticate' control message from client %d, dropping connection", clientGetID(&self->client));
            clientClose(&self->client);
            return;
          }
        struct ucred *creds = (struct ucred *)CMSG_DATA(cmsg);
        self->uid = creds->uid;
        self->gid = creds->gid;
        self->pid = creds->pid;

        if (!self->authenticated)
          {
            self->authenticated = true;
            uint32_t channel;
            if (!unixNewChannel(&self->client, &channel))
              {
                Y_TRACE ("Failed to create primary channel for client %d, dropping connection", clientGetID(&self->client));
                clientClose(&self->client);
                return;
              }
            assert(channel == 0);
          }

        break;
      }
    default:
      Y_TRACE ("Unrecognised control message type %d from client %d, discarding", type, clientGetID(&self->client));
      break;
    }
}

static void
doControlWrite(struct unixClient *self)
{
  if (llist_length(self->control_queue) > 0)
    {
      struct unixControlMessage *ucmsg = llist_node_data(llist_head(self->control_queue));

      switch(ucmsg->type)
        {
        case ucmtNewChannel:
          {
            struct unixChannel *channel = unixFindChannel(self, ucmsg->id);
            if (!channel)
              {
                /* Whoops. Clean up. */
                llist_node_delete(llist_head(self->control_queue));
                unixControlMessageDestroy(ucmsg);
                break;
              }

            /* Unix sockets can be delightfully evil at times. Don't
             * expect to understand this unless you understand how
             * sendmsg() works
             */
            uint32_t msg_type = ucmsg->type;
            uint32_t id = ucmsg->id;
            uint32_t msg_len = sizeof(id);
            struct iovec iov[] = {{&msg_len, sizeof(msg_len)}, {&msg_type, sizeof(msg_type)}, {&id, msg_len}};
            int sv[1];
            sv[0] = ucmsg->fd;
            char buf[CMSG_SPACE(sizeof(sv))];
            struct msghdr msg = {0, 0,
                                 iov, 3,
                                 buf, sizeof(buf),
                                 0};

            struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
            cmsg->cmsg_level = SOL_SOCKET;
            cmsg->cmsg_type = SCM_RIGHTS;
            cmsg->cmsg_len = CMSG_LEN(sizeof(sv));
            int *psv = (int *)CMSG_DATA(cmsg);
            memcpy(psv, &sv, sizeof(sv));
            msg.msg_controllen = cmsg->cmsg_len;

            ssize_t len = sendmsg(self->control_fd, &msg, 0);
            if (len == -1)
              {
                Y_TRACE ("Failed to pass channel fd for client %d, dropping connection", clientGetID(&self->client));
                clientClose(&self->client);
                return;
              }

            /* Success. Clean up this end. */
            llist_node_delete(llist_head(self->control_queue));

            controlRegisterFileDescriptor (channel->fd, CONTROL_WATCH_READ,
                                           channel, unixChannelReady);

            /* Note that this closes the local copy of the remote fd */
            unixControlMessageDestroy(ucmsg);
            break;
          }
        case ucmtAuthenticate:
          abort();
        }
    }

  if (llist_length(self->control_queue) == 0)
    controlChangeFileDescriptorMask (self->control_fd, CONTROL_WATCH_READ);
}

static void
unixControlReady (int fd, int causeMask, void *data_v)
{
  struct unixClient *self = data_v;
  assert(self->control_fd == fd);
  if (causeMask & CONTROL_WATCH_READ)
    doControlRead(self);
  if (causeMask & CONTROL_WATCH_WRITE)
    doControlWrite(self);
}

static void
unixSocketReady (int fd, int causeMask, void *data_v)
{
  struct unixClient *newClient;

  assert(fd == listen_fd);

  int new_fd = accept (fd, NULL, NULL);
  if (new_fd == -1)
    return;

  int sock_opt = 1;
  if (setsockopt(new_fd, SOL_SOCKET, SO_PASSCRED, (void *)&sock_opt, sizeof(sock_opt)) == -1)
    {
      Y_ERROR("Failed to set SO_PASSCRED on unix socket %d, closing: %s", new_fd, strerror(errno));
      close(new_fd);
      return;
    }

  newClient = ymalloc (sizeof (struct unixClient));
  newClient -> control_fd = new_fd;
  newClient -> authenticated = false;
  newClient -> uid = -1;
  newClient -> gid = -1;
  newClient -> pid = -1;
  newClient -> channels = new_rbtree(unixChannelKey, unixChannelCmp);
  newClient -> next_channel = 0;
  newClient -> control_queue = new_llist();

  newClient -> client.c = &unixClientClass;

  clientRegister (&(newClient -> client));

  controlRegisterFileDescriptor (newClient->control_fd, CONTROL_WATCH_READ,
                                 newClient, unixControlReady);
};

static void
unixClose (struct Client *self_c)
{
  struct unixClient *self = castBack (self_c);
  llist_destroy(self->control_queue, unixControlMessageDestroy);
  rbtree_destroy(self->channels, unixChannelDestroy);
  controlUnregisterFileDescriptor (self->control_fd);
  close (self->control_fd);
  yfree (self);
}

void
unixInitialise (void)
{
  struct sockaddr_un sockaddr;

  pid_t pid = getpid();
  char path[64];
  snprintf(path, 64, "/tmp/.Y-unix/%lu", (long unsigned int)pid);
  socket_path = ystrdup (path);
  sockaddr.sun_family = AF_UNIX;
  strncpy (sockaddr.sun_path, socket_path, 100);

  listen_fd = socket (PF_UNIX, SOCK_STREAM, 0);

  if (listen_fd < 0)
    {
      Y_FATAL("Failed to create PF_UNIX socket: %s", strerror(errno));
      yfree(socket_path);
      exit(1);
    }

  mode_t old_umask = umask(0);
  if (mkdir("/tmp/.Y-unix", 0777) == -1 && errno != EEXIST)
    {
      umask(old_umask);
      Y_FATAL("Failed to create /tmp/.Y-unix: %s", strerror(errno));
      exit(1);
    }
  umask(old_umask);
 
  unlink (socket_path);
  int r = bind (listen_fd, (struct sockaddr *)&sockaddr, sizeof (sockaddr));

  if (r < 0)
    {
      Y_FATAL("Failed to bind to socket %s: %s", socket_path, strerror(errno));
      close(listen_fd);
      listen_fd = -1;
      yfree(socket_path);
      socket_path = NULL;
      exit(1);
    }

  r = chmod (socket_path, 0777);

  if (r < 0)
    Y_ERROR("Failed to set mode of %s to 0777: %s", socket_path, strerror(errno));

  r = listen (listen_fd, 64);

  if (r < 0)
    {
      Y_FATAL("Failed to listen on socket %s: %s", socket_path, strerror(errno));
      close(listen_fd);
      listen_fd = -1;
      yfree(socket_path);
      socket_path = NULL;
      exit(1);
    }
  
  controlRegisterFileDescriptor (listen_fd, CONTROL_WATCH_READ,
                                 NULL, unixSocketReady);
}

void
unixFinalise (void)
{
  unlink (socket_path);
  yfree (socket_path);
}

/* arch-tag: 52459b6c-74d5-4bba-8ef1-7043ff4879e4
 */
