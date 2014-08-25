set(udp_SRCS
  UdpClient.cc
  UdpMessage.cc
  )

add_library(muduo_net_udp ${udp_SRCS})
target_link_libraries(muduo_net_udp muduo_net)

install(TARGETS muduo_net_udp DESTINATION lib)
set(HEADERS
  UdpClient.h
  )
install(FILES ${HEADERS} DESTINATION include/muduo/net/udp)

#if(NOT CMAKE_BUILD_NO_EXAMPLES)
#add_executable(httpserver_test tests/HttpServer_test.cc)
#target_link_libraries(httpserver_test muduo_http)
#
#if(BOOSTTEST_LIBRARY)
#add_executable(httprequest_unittest tests/HttpRequest_unittest.cc)
#target_link_libraries(httprequest_unittest muduo_http boost_unit_test_framework)
#endif()
#
#endif()

# add_subdirectory(tests)
