Installation
Dependencies : MySQL,BOOST, libmysqlclient-dev, cmake
( for dependencies installation go to the end of the file)

Files :
.:
client  file_server  proxy_server  readme.txt

./client:
client client.cpp

./file_server:
file_server file_server.cpp

./proxy_server:
db.cpp  db.h  file_manager  make.sh  server.cpp



Change the location to the client and compile the program using
g++  client.cpp -o client
Change the location to the file_server and compile the program using
g++  file_server.cpp -o file_server
Change the location to the  proxy_server and compile the program using
chmod  u+x make.sh
sh make.sh




Running the code:
Change the location to the file_server and run the backend for the application using
./file_server
Change the location to the  proxy_server and run the proxy server using
        ./proxy_server
Change the location to the client and run the client using
./client



Testing of Code:
1. Registration
 Register a new user
Registering the same user again

2.Login
logging in using random credentials
Provide valid credentials that you have used during the registration

3.Upload (Complete path of file has to be provided)
Uploading of file that doesn't exist
Upload correct file

4.Download
Enter the index of the file to be downloaded

5.Logout/Exit
Perform logout or exit at any time during the program execution



Dependencies installation:

Installation of BOOST:
Download tar.gz file from https://dl.bintray.com/boostorg/release/1.65.0/source/ and install
More info : http://www.boost.org/

Installtion of libmysqlclient-dev, cmake:
Recommended : Install using apt [ sudo apt install libmysqlclinet-dev cmake]

Installation of MySQL:
Dependencies : BOOST, libmysqlclient-dev, Cmake, git (If building from source )

Clone the mysqlconnector from git clone https://github.com/mysql/mysql-connector-cpp
Change location to the top-level directory of the source distribution:
shell> cd /path/to/mysql-connector-cpp

Run CMake to build a Makefile:
shell> cmake .
-- Check for working C compiler: /usr/local/bin/gcc
-- Check for working C compiler: /usr/local/bin/gcc -- works
[...]
-- Generating done
-- Build files have been written to: /path/to/mysql-connector-cpp/

Use make to build Connector/C++. First make sure you have a clean build, then build the connector:
shell> make clean
shell> make
[  1%] Building CXX object »
driver/CMakeFiles/mysqlcppconn.dir/mysql_connection.o
[  3%] Building CXX object »
driver/CMakeFiles/mysqlcppconn.dir/mysql_constructed_resultset.o
[...]
[100%] Building CXX object examples/CMakeFiles/statement.dir/statement.o
Linking CXX executable statement
If all goes well, you will find the Connector/C++ library in the driver directory.
Install the header and library files
shell> make install
 Check the installation by compiling the program here https://dev.mysql.com/doc/connector-cpp/en/connector-cpp-examples-complete-example-1.html
g++ -o test_install \
  -I/usr/local/include -I/usr/local/include/cppconn \
  -Wl,-Bdynamic standalone_example.cpp -lmysqlcppconn


More info : https://dev.mysql.com/doc/connector-cpp/en/connector-cpp-installation-source.html
