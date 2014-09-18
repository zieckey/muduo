#include <muduo/net/TcpClient.h>

#include <muduo/base/Logging.h>
#include <muduo/base/Thread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

#include <boost/bind.hpp>

#include <utility>

#include <stdio.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

class EchoClient : boost::noncopyable
{
 public:
  EchoClient(EventLoop* loop, const InetAddress& listenAddr, int size)
    : loop_(loop),
      client_(loop, listenAddr, "EchoClient"),
      message_(size, 'H')
  {
    client_.setConnectionCallback(
        boost::bind(&EchoClient::onConnection, this, _1));
    client_.setMessageCallback(
        boost::bind(&EchoClient::onMessage, this, _1, _2, _3));
  }

  void connect(const InetAddress& localAddr)
  {
    client_.bind(localAddr);
    client_.connect();
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    LOG_TRACE << conn->localAddress().toIpPort() << " -> "
        << conn->peerAddress().toIpPort() << " is "
        << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected())
    {
      conn->setTcpNoDelay(true);
      conn->send(message_);
    }
    else
    {
      loop_->quit();
    }
  }

  void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
  {
    conn->send(buf);
  }

  EventLoop* loop_;
  TcpClient client_;
  string message_;
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
  if (argc > 2)
  {
    EventLoop loop;
    InetAddress localAddr(argv[1], 0);
    InetAddress serverAddr(argv[2], 2007);

    int size = 256;
    if (argc > 3)
    {
      size = atoi(argv[3]);
    }

    EchoClient client(&loop, serverAddr, size);
    client.connect(localAddr);
    loop.loop();
  }
  else
  {
    printf("Usage: %s local_ip server_ip [msg_size]\n", argv[0]);
  }
}

