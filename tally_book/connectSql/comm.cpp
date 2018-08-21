#include "comm.h"

void test()
{
	std::cout<<"client version:  "<< mysql_get_client_info() << std::endl; 
}

MYSQL*  mysql_connect()
{
	MYSQL * myfd = mysql_init(NULL);
	if(mysql_real_connect(myfd,"127.0.0.1","root",\
	"","tally_book",3306,NULL,0) == NULL)
	{
		std::cout << "connect error" << std::endl; 
	}
	else
	{
		std::cout << "connect success" << std::endl;
	}
	return myfd;
	
}

int mysql_insert(MYSQL* myfd, std::string &date,std::string &expend,std::string &income,std::string &reason)
{
	std::string sql = "INSERT INTO book (date,expend,income,reason) values(\"";
		sql += date;
		sql += "\",\"";
		sql += expend;
		sql += "\",\"";
		sql += income;
		sql += "\",\"";
		sql += reason;
		sql += "\")";

		std::cout << sql << std::endl;
		return mysql_query(myfd,sql.c_str());
}

void mysql_select(MYSQL * myfd)
{
	std::string sql = "SELECT * FROM book";
	 mysql_query(myfd,sql.c_str());

	MYSQL_RES* result = mysql_store_result(myfd);
	int lines = mysql_num_rows(result);
	int cols  = mysql_num_fields(result);

	MYSQL_FIELD *fields = mysql_fetch_fields(result);
	std::cout << "<table border=\"1\">"<< std::endl;
	int i = 0;
	for(; i < cols; i++)
	{
		std::cout << "<th>"<<fields[i].name<<"</th>" << '\t';
	}
	
	std::cout << std::endl;
	MYSQL_ROW my_row;

	//<table border="1">
	//<tr>
	//<td>row 1, cell 1</td>
	//<td>row 1, cell 2</td>
	//</tr>
	//<tr>
	//<td>row 2, cell 1</td>
	//<td>row 2, cell 2</td>
	//</tr>
	//</table>	

	//std::cout << "<table border=\"1\">"<< std::endl;
	for(i=0; i < lines; i++ )
	{
		std::cout<< "<tr>" << std::endl;
		my_row = mysql_fetch_row(result);
		int j = 0;
		for(; j < cols; j++)
		{
			std::cout <<"<td>"<< my_row[j] <<"</td>";
		}
		std::cout << std::endl;
		std::cout << "</tr>" << std::endl;
	}
	std::cout << "</table>"<< std::endl;
	free(result);	
	
}

void myclose(MYSQL *myfd)
{
	mysql_close(myfd);
}
