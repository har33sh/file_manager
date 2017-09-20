#ifndef DB_H
#define DB_H


bool authenticateUser(char *username,char *password);
bool usernameAvailable(char *username);
bool createUser(char *username,char *password);

#endif
