
#include <stdio.h> 
#include <boost/bind.hpp>

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <muduo/net/udp/UdpClient.h>

using namespace muduo;
using namespace muduo::net;

void onMessage(const UdpClientPtr& client,
            BufferPtr& buffer,
            Timestamp timestamp)
{
  muduo::string msg = buffer->retrieveAllAsString();
  LOG_INFO << "recv msg:" << msg;
}

void connect(UdpClient* client)
{
  client->connect();
  client->setMessageCallback(&onMessage);
}

void quit(UdpClient* client, muduo::net::TimerId sendTimerId, muduo::net::EventLoop* loop)
{
  client->close();
  loop->cancel(sendTimerId);
  loop->quit();
}

void onTimer(UdpClient* client)
{
    client->send("123");
}

int main(int argc, char* argv[])
{
  if (argc != 3) {
    LOG_ERROR << "usage : " << argv[0] << " host port";
    return 0;
  }
  const char* host = argv[1];
  uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
  muduo::net::EventLoop loop;
  muduo::net::InetAddress remoteAddr(host, port);
  muduo::net::UdpClientPtr client(new muduo::net::UdpClient(&loop, remoteAddr, "echo"));
  muduo::net::TimerId sendTimerId = loop.runEvery(1.0, boost::bind(&onTimer, client.get()));
  loop.runAfter(0.1, boost::bind(&connect, client.get()));
  loop.runAfter(5, boost::bind(&quit, client.get(), sendTimerId, &loop));
  loop.loop();
}

