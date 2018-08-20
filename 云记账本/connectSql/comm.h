#pragma once

#include <iostream>
#include <mysql.h>

void test();
MYSQL*  mysql_connect();
int mysql_insert(MYSQL* myfd, std::string &date,std::string &expend,std::string &income,std::string &reason);
void myclose(MYSQL *myfd);
void mysql_select(MYSQL * myfd);
