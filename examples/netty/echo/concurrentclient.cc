#include <muduo/net/TcpClient.h>

#include <muduo/base/Logging.h>
#include <muduo/base/Thread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThreadPool.h>
#include <muduo/net/InetAddress.h>

#include <boost/bind.hpp>

#include <utility>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <set>

using namespace muduo;
using namespace muduo::net;
typedef boost::shared_ptr<TcpClient> TcpClientPtr;
typedef std::set<TcpClientPtr> TcpClientSet;

static bool localBind = false;

void onConnection(TcpClientPtr tc, const TcpConnectionPtr& conn)
{
  LOG_TRACE << conn->localAddress().toIpPort() << " -> "
    << conn->peerAddress().toIpPort() << " is "
    << (conn->connected() ? "UP" : "DOWN");

  if (conn->connected())
  {
    conn->setTcpNoDelay(true);
  }
}

void onMessage(TcpClientPtr tc, const TcpConnectionPtr& conn,
      Buffer* buf, Timestamp time)
{
  LOG_TRACE << conn->localAddress().toIpPort() << " -> "
    << conn->peerAddress().toIpPort()
    << " recved a message:" << buf->retrieveAllAsString();
}

TcpClientPtr newTcpClient(EventLoop* loop,
      const InetAddress& localAddr, const InetAddress& serverAddr)
{
  TcpClientPtr c(new TcpClient(loop, serverAddr,
          localAddr.toIpPort() + "->" + serverAddr.toIpPort()));
  c->setConnectionCallback(boost::bind(&onConnection, c, _1));
  c->setMessageCallback(boost::bind(&onMessage, c, _1, _2, _3));
  if (localBind)
  {
    c->bind(localAddr);
  }
  c->connect();
  return c;
}

void sendMessage(TcpClientSet* map, const string& msg)
{
  TcpClientSet::iterator it (map->begin());
  TcpClientSet::iterator ite(map->end());
  for (; it != ite; ++it)
  {
    TcpConnectionPtr conn = (*it)->connection();
    if (conn && conn->connected())
    {
      conn->send(msg);
    }
  }
}

void usage(char* argv[])
{
  printf("usage: %s -p <listenPort>\
        -s <firstLocalIp> -n <localIpNum>\
        -S <firstServerIp> -N <serverIpNum>\
        -t <threadNum> -c <connPerIp>\
        -P <firstLocalPort> -m <localPortNum>\
        -l <messageLen> -i <sendMessageIntervalMs> -h -b\n", argv[0]);
  printf("-H The tcp server ip to connect to\n");
  printf("-p The tcp server listened port\n");
  printf("-s The first local ip to bind\n");
  printf("-n The local ip number\n");
  printf("-S The first server ip to connect\n");
  printf("-N The server ip number\n");
  printf("-t The thread number\n");
  printf("-c The connection number created for every ip\n");
  printf("-l The length of the message sending to server\n");
  printf("-i The interval time (ms) between the message sending to server\n");
  printf("-P The first local port, default 0\n");
  printf("-m The total number of local port, default 1\n");
  printf("-b Do the local binding, default false\n");
}

string getLocalIp(const char* firstLocalIp, int index)
{
  const char* last = strrchr(firstLocalIp, '.');
  assert(last);
  int start = atoi(last+1);
  string ip = string(firstLocalIp, last - firstLocalIp + 1);
  char buf[64] = {0};
  snprintf(buf, sizeof(buf), "%d", index + start);
  return ip + buf;
}

int main(int argc, char* argv[])
{
  int listenPort = 2007;
  const char* firstLocalIp = "192.168.1.50"; // -s
  int localIpNum = 1; // -n
  const char* firstServerIp = "192.168.1.100"; // -S
  int serverIpNum = 1; // -N
  int threadNum = 1; // -t 
  int connPerIp = 1000; // The connection number created for every ip. -c
  int messageLen = 1024; // -l
  int sendMessageIntervalMs = 1000; // -i
  int localPortNum = 1;
  int firstLocalPort = 0;
  int c;
  while (-1 != (c = getopt(argc, argv, "p:s:n:S:N:t:l:i:m:P:bh")))
  {
    switch (c)
    {
      case 'p':
        listenPort = atoi(optarg);
        break;
      case 's':
        firstLocalIp = optarg;
        break;
      case 'n':
        localIpNum = atoi(optarg);
        break;
      case 'S':
        firstServerIp = optarg;
        break;
      case 'N':
        serverIpNum = atoi(optarg);
        break;
      case 't':
        threadNum = atoi(optarg);
        break;
      case 'c':
        connPerIp = atoi(optarg);
        break;
      case 'l':
        messageLen = atoi(optarg);
        break;
      case 'i':
        sendMessageIntervalMs = atoi(optarg);
        break;
      case 'P':
        firstLocalPort = atoi(optarg);
        break;
      case 'm':
        localPortNum = atoi(optarg);
        break;
      case 'h':
        usage(argv);
        return -1;
      case 'b':
        localBind = true;
      default:
        fprintf(stderr, "Illegal argument \"%c\"\n", c);
        usage(argv);
        return 1;
    }
  }
  LOG_INFO 
    << "listenPort=" << listenPort << "\n"
    << "firstLocalIp=" << firstLocalIp << "\n"
    << "localIpNum=" << localIpNum << "\n"
    << "firstServerIp=" << firstServerIp << "\n"
    << "localPortNum=" << localPortNum << "\n"
    << "firstLocalPort=" << firstLocalPort << "\n"
    << "serverIpNum=" << serverIpNum << "\n"
    << "threadNum=" << threadNum << "\n"
    //<< "connPerIp=" << connPerIp << "\n"
    << "messageLen=" << messageLen << "\n"
    << "sendMessageIntervalMs=" << sendMessageIntervalMs;

  TcpClientSet clients;

  EventLoop baseLoop;
  EventLoopThreadPool pool(&baseLoop);
  pool.setThreadNum(threadNum);
  pool.start();

  for (int port = 0; port < localPortNum; port++)
  {
    for (int ipIndex = 0; ipIndex < localIpNum; ipIndex++)
    {
      string localIp = getLocalIp(firstLocalIp, ipIndex);
      LOG_DEBUG << "next local ip " << localIp << " , local port " << port;
      InetAddress localAddr(localIp, uint16_t(port + firstLocalPort));
      InetAddress serverAddr(firstServerIp, uint16_t(listenPort));
      EventLoop* loop = pool.getNextLoop();
      TcpClientPtr tc = newTcpClient(loop, localAddr, serverAddr);
      clients.insert(tc);
    }
  }

  string msg(messageLen, 'c');
  baseLoop.runEvery(double(sendMessageIntervalMs)/1000.0, boost::bind(&sendMessage, &clients, msg));
  baseLoop.loop();
}

