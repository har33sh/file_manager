// All the database operations goes here

#include <iostream>
#include <string>
#include<string.h>
#include <stdlib.h>
#include <errno.h>
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include<mysql/mysql.h>

#define  IP_ADDR "tcp://10.129.23.200:3306"
#define  user  "root"
#define  pass "root"
#define  DB_NAME "test"


using namespace std;

bool authenticateUser(char *username_arg,char *password_arg){
  string username=username_arg;
  string password=password_arg;
  bool auth=false;
  try {
    sql::Driver *driver;
    sql::Connection *con;
    sql::Statement *stmt;
    sql::ResultSet *res;
    driver = get_driver_instance();
    con = driver->connect(IP_ADDR,user,pass );
    con->setSchema(DB_NAME);
    stmt = con->createStatement();
    res = stmt->executeQuery("SELECT password from users where user_id= '"+username+"' limit 1;");
    while (res->next()) {
      string res_passwrd=string(res->getString(1));
      const char *query_password=res_passwrd.c_str();
      const char *given_password=password.c_str();
      if (strcmp(query_password,given_password)==0)
          auth=true;
      else
          auth=false;
    }

  delete res;
  delete stmt;
  delete con;

  }
  catch (sql::SQLException &e) {
    cerr << "DB Auth: "<< " (MySQL error code: " << e.getErrorCode() <<endl;
  }

  return auth;

}

bool usernameAvailable(char *username_arg){
  string username=username_arg;
  bool auth=true;
  try {
    sql::Driver *driver;
    sql::Connection *con;
    sql::Statement *stmt;
    sql::ResultSet *res;
    driver = get_driver_instance();
    con = driver->connect(IP_ADDR,user,pass );
    con->setSchema(DB_NAME);
    stmt = con->createStatement();
    res = stmt->executeQuery("SELECT user_id from users where user_id= '"+username+"' limit 1;");
    while (res->next()) {
      auth=false;
    }
  delete res;
  delete stmt;
  delete con;

  }
  catch (sql::SQLException &e) {
    auth=true;
    cerr << "DB Username Avail (ignore) (MySQL error code: " << e.getErrorCode()<<endl;
  }
  return auth;

}

bool createUser(char *username_arg,char *password_arg){
  string username=username_arg;
  string password=password_arg;
  bool auth=false;
  try {
    sql::Driver *driver;
    sql::Connection *con;
    sql::Statement *stmt;
    sql::ResultSet *res;
    driver = get_driver_instance();
    con = driver->connect(IP_ADDR,user,pass);
    con->setSchema(DB_NAME);
    stmt = con->createStatement();
    stmt->executeQuery("insert into users values('"+username+"','"+password+"');");

    delete res;
    delete stmt;
    delete con;
    auth=true;
  }
  catch (sql::SQLException &e) {
    cerr << "DB Creating user (MySQL error code: " << e.getErrorCode()<<endl;
  }
  return auth;

}
