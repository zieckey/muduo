// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: zieckey (zieckey at gmail dot com)
//

#include <muduo/net/udp/UdpClient.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Connector.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <stdio.h>  // snprintf
#include <errno.h>  // snprintf

using namespace muduo;
using namespace muduo::net;

// UdpClient::UdpClient(EventLoop* loop)
//   : loop_(loop)
// {
// }

// UdpClient::UdpClient(EventLoop* loop, const string& host, uint16_t port)
//   : loop_(CHECK_NOTNULL(loop)),
//     serverAddr_(host, port)
// {
// }

namespace muduo
{
namespace net
{
namespace detail
{
typedef struct sockaddr SA;
const SA* sockaddr_cast(const struct sockaddr_in* addr)
{
  return static_cast<const SA*>(implicit_cast<const void*>(addr));
}
}
}
}

UdpClient::UdpClient(EventLoop* loop,
                     const InetAddress& serverAddr,
                     const string& name)
  : loop_(CHECK_NOTNULL(loop)),
    name_(name),
    serverAddr_(serverAddr),
    //messageCallback_(defaultMessageCallback), //TODO
    retry_(false),
    connect_(false)
{
  LOG_INFO << "UdpClient::UdpClient[" << name_
           << "] - remoteAddr " << serverAddr_.toIpPort();
}

UdpClient::~UdpClient()
{
  LOG_INFO << "UdpClient::~UdpClient[" << name_
           << "] - channel_ " << get_pointer(channel_);
  loop_->assertInLoopThread();
  close();
}

bool UdpClient::connect()
{
  LOG_INFO << "UdpClient::connect[" << name_ << "] - connecting to "
           << serverAddr_.toIpPort();
  assert(!connect_);
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  assert(sockfd > 0);
  if (sockfd < 0) {
    return false;
  }

  const struct sockaddr_in& remoteAddr = serverAddr_.getSockAddrInet();
  socklen_t addrLen = sizeof(remoteAddr);
  int ret = ::connect(sockfd, detail::sockaddr_cast(&remoteAddr), addrLen);

  if (ret != 0)
  {
    int  savedErrno = errno;
    close();
    const struct sockaddr_in *paddr = &remoteAddr;
    LOG_ERROR << "Failed to connect to remote IP="
        << inet_ntoa(paddr->sin_addr)
        << ", port=" << ntohs(paddr->sin_port)
        << ", errno=" << savedErrno;
    ::close(sockfd);
    return false;
  }

  channel_.reset(new Channel(getLoop(), sockfd));
  channel_->setReadCallback(
      boost::bind(&UdpClient::handleRead, this, _1));
//  channel_->setWriteCallback(
//      boost::bind(&UdpClient::handleWrite, this));
//  channel_->setCloseCallback(
//      boost::bind(&UdpClient::handleClose, this));
//  channel_->setErrorCallback(
//      boost::bind(&UdpClient::handleError, this));

  connect_ = true;
  return true;
}

void UdpClient::close()
{
  if (loop_->isInLoopThread())
  {
    closeInLoop();
  }
  else
  {
    loop_->runInLoop(
        boost::bind(&UdpClient::closeInLoop, this));  
  }
}

void UdpClient::closeInLoop()
{
  loop_->assertInLoopThread();
  connect_ = false;
  if (channel_.get()) {
    if (channel_->fd() > 0) {
      ::close(channel_->fd());
    }
    channel_->disableAll();
    channel_->remove();
    channel_.reset();
  }
}

void UdpClient::send(const void* data, size_t len)
{
  assert(connect_);
  if (connect_)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(data, len);
    }
    else
    {
      string message(static_cast<const char*>(data), len);
      loop_->runInLoop(
          boost::bind(&UdpClient::sendInLoop,
                      this,     // FIXME
                      message));
    }
  }
}

void UdpClient::send(const StringPiece& message)
{
  assert(connect_);
  if (connect_)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(message);
    }
    else
    {
      loop_->runInLoop(
          boost::bind(&UdpClient::sendInLoop,
                      this,
                      message.as_string()));
    }
  }
}

// FIXME efficiency!!!
void UdpClient::send(Buffer* buf)
{
  assert(connect_);
  if (connect_)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(buf->peek(), buf->readableBytes());
      buf->retrieveAll();
    }
    else
    {
      loop_->runInLoop(
          boost::bind(&UdpClient::sendInLoop,
                      this,
                      buf->retrieveAllAsString()));
    }
  }
}

void UdpClient::sendInLoop(const StringPiece& message)
{
  sendInLoop(message.data(), message.size());
}

void UdpClient::sendInLoop(const void* data, size_t len)
{
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  bool error = false;
  if (!connect_)
  {
    LOG_WARN << "disconnected, give up writing";
    return;
  }

  //send udp data directly
  const struct sockaddr_in& remoteAddr = serverAddr_.getSockAddrInet();
  socklen_t addrLen = sizeof(remoteAddr);
  nwrote = ::sendto(channel_->fd(), data, len, 0,
              detail::sockaddr_cast(&remoteAddr), addrLen);
  if (nwrote >= 0)
  {
      //sent data OK
  }
  else // nwrote < 0
  {
    nwrote = 0;
    if (errno != EWOULDBLOCK)
    {
      LOG_SYSERR << "UdpClient::sendInLoop";
      if (errno == EPIPE) // FIXME: any others?
      {
        error = true;
      }
    }

    //TODO FIXME fix this routine problem : when sendto return -1
  }
}

void UdpClient::handleRead(Timestamp receiveTime)
{
  loop_->assertInLoopThread();
  //int savedErrno = 0;
  size_t initialSize = 1472; // The UDP max payload size
  BufferPtr inputBuffer(new Buffer(initialSize));
  struct sockaddr remoteAddr;
  socklen_t addrLen = sizeof(remoteAddr);
  ssize_t readn = ::recvfrom(channel_->fd(), inputBuffer->beginWrite(), 
      inputBuffer->writableBytes(),
      0, &remoteAddr, &addrLen);
  int savedErrno = sockets::getSocketError(channel_->fd());
  if (readn == 0)
  {
    //TODO ERROR process
    LOG_ERROR << "errno=" << savedErrno << " " << strerror(savedErrno) << " recvfrom return " << readn;
    //handleClose();
  }
  else if (readn > 0)
  {
    inputBuffer->hasWritten(readn);
    if (messageCallback_) {
      messageCallback_(shared_from_this(), inputBuffer, receiveTime);
    }
  }
  else
  {
      //TODO
    LOG_ERROR << "errno=" << savedErrno << " " << strerror(savedErrno) << " recvfrom return " << readn;
    //handleError();
  } 
}


