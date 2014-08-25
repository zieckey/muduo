// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: zieckey (zieckey at gmail dot com)


#include <muduo/net/udp/UdpMessage.h>
#include <muduo/net/SocketsOps.h>
#include <muduo/base/Logging.h>

namespace muduo
{
namespace net
{

bool UdpMessage::send(const UdpMessage* msg) {
  ssize_t sentn = ::sendto(msg->sockfd(),
              msg->data(), msg->size(), 0, 
              sockets::sockaddr_cast(&msg->remoteAddr()), 
              sizeof(msg->remoteAddr()));
  if (sentn < 0) {
    //TODO FIXME
    LOG_ERROR << "sentn=" << sentn << " , dlen=" << msg->size();
    return false;
  }

  return true;
}

}
}

#

