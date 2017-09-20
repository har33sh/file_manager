g++ -o proxy_server   -I/usr/local/include -I/usr/local/include/cppconn   -Wl,-Bdynamic db.cpp server.cpp -lmysqlcppconn
