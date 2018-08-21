#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#define MAX 1024
#define HOME_PAGE  "index.html"
#define PAGE_404 "wwwroot/404.html"

static void usage(const char *proc)
{
	printf("\nUsage:\n\t%s port\n\n",proc);
}

//检测换行
int  get_line(int sock,char line[],int num)
{
	assert(line);
	assert(num > 0);
	
	int i = 0;
	char c = 'A';
	while(c != '\n'&& i < num -1)
	{
		//从sock里面读，对到c，一次读一个字符，阻塞方式
		ssize_t s = recv(sock,&c,1,0);
		if(s > 0)
		{
			if(c == '\r')
			{
				recv(sock,&c,1,MSG_PEEK);//窥探模式MSG_PEEK加入后只读取字符,不拿走
					if(c == '\n')
					{
						recv(sock,&c,1,0);
					}
					else
					{
						c = '\n';
					}
			}
			//\n  non \n   
			line[i++] = c;
		}
	}
	line[i] = '\0';
	return i;
}


void clear_header(int sock)//头部信息清理，
{
	char line[MAX];	
	do	
	{
		get_line(sock,line,sizeof(line));
		printf("%s",line);
	}while(strcmp(line,"\n"));  //没有读到空行一直继续
}


void show_404(int sock)
{
	char line[1024];
	sprintf(line,"HTTP/1.0 404 Not Found\r\n");
	send(sock,line,strlen(line),0);
	sprintf(line,"Content-Type: text/html;charset=IOS-8859-1\r\n");   //响应的类型
	send(sock,line,strlen(line),0);
	sprintf(line,"\r\n");
	send(sock,line,strlen(line),0);	

	struct stat st;
	stat(PAGE_404,&st);
	int fd = open(PAGE_404,O_RDONLY);
	sendfile(sock,fd,NULL,st.st_size);	
	close(fd);
}

void echo_error(int sock,int code)
{
	switch(code)
	{
		case 400:
		//show_404();
			break;
		case 403:
		//show_404();
			break;
		case 404:
		show_404(sock);
			break;
		case 500:
		//show_404();
			break;
		case 503:
		//show_404();
			break;
		default:
			break;
	}
}


int  echo_www(int sock,char *path,int size)
{
	int fd = open(path,O_RDONLY);
	if(fd < 0)
	{
		return 404;
	}
	//到这里  请求行  报头 空行  没有正文
	//以http请求格式响应的，以http响应
	clear_header(sock);   //把所有的请求处理完
	printf("11");
	//构建响应
	char line[MAX];
	sprintf(line,"HTTP/1.0 200 ok\r\n");
	//可以先仍出去，让传这，http是基于tcp的，tcp是基于字节流的，把数据已发，并没有直接发送出去，而是发送到发送缓冲区里
	send(sock,line,strlen(line),0);//状态行发出去了
	sprintf(line,"Content-Type: text/html;charset=IOS-8859-1\r\n");   //响应的类型
	send(sock,line,strlen(line),0);
	sprintf(line,"\r\n");
	send(sock,line,strlen(line),0);	

	sendfile(sock,fd,NULL,size);
	close(fd);	
	return 200;
}

int  exe_cgi(int sock,char *  method,char* path,char* query_string)
{
	char line[MAX];
	char method_env[MAX/10];
	char query_string_env[MAX];
	char content_length_env[MAX/10];
	int content_length = -1;
	if(strcasecmp(method,"GET") == 0){
		clear_header(sock);	
	}else{//POST
		do{
			  get_line(sock,line,sizeof(line));//这里读的是报头字段
				if(strncmp(line,"Content-Length: ",16) == 0)
				{
					content_length = atoi(line+16);//字符串地址+16开始的字符串，转为数字
				}
		  }while(strcmp(line,"\n"));
			if(content_length == -1)
			{
				return 400;//客户端的问题
			}
	}

	//站在子进程的角度
	int input[2];
	int output[2];

	pipe(input); //管道创建
	pipe(output); 
	
	//path 是一个确定指定路径下的程序  //怎样执行
	//理解exec，线程调用
	pid_t id = fork();
	if(id < 0)
	{
		return 503;//服务器忙
	}
	else if(id == 0)
	{//child   //写是1，读是0  子进程是读的，父进程是要写的
		close(sock);
		close(input[1]);
		close(output[0]);
		
		sprintf(method_env,"METHOD=%s",method);
		putenv(method_env);
		
		if(strcasecmp(method,"GET") == 0)
		{
			sprintf(query_string_env,"QUERY_STRING=%s",query_string);
			putenv(query_string_env);
		}
		else
		{
			sprintf(content_length_env,"CONTENT_LENGTH=%d",content_length);
			putenv(content_length_env);
		}
	
		dup2(input[0],0);
		dup2(output[1],1);

		execl(path,path,NULL); //一个个是要执行谁，怎么执行，直接名称执行，不需要其他参数，所以第三个NULL；
		exit(1);
	}
	else
	{//进程等待
		close(input[0]);
		close(output[1]);

		char c;
		if(strcasecmp(method,"POST") == 0)
		{
			char c;
			int i = 0;
			for(; i< content_length; i++)
			{
				recv(sock,&c,1,0);
				write(input[1],&c,1);
				
			}
		}
		sprintf(line,"HTTP/1.0 200 OK\r\n");
		send(sock,line,strlen(line),0);
		sprintf(line,"Content-Type: text/html;charset=IOS-8859-1\r\n");   
		send(sock,line,strlen(line),0);
		sprintf(line,"\r\n");
		send(sock,line,strlen(line),0);
	
		while(read(output[0],&c,1) > 0)
		{
			send(sock,&c,1,0);
		}
		waitpid(id,NULL,0);  //子进程的id，退出结果不关心NULL,等待方式，这是一个线程wait，不会影响其他线程
	}
	return 200;
}	

void *handler_request(void *arg)
{
	int sock = (int)arg;

	char line[MAX];      //读取一行
	char method[MAX/10];  //方法
	char url[MAX];        //
	char path[MAX];
	int i = 0;
	int j = 0;
	int status_code = 200;   //状态码，HTTP响应 200是成功
	int cgi = 0;
	char *query_string = NULL;  //表示请求的字符串

	
	get_line(sock,line,sizeof(line));  //line-> \r.\r\n.\n->\n
	while(i < sizeof(method)-1 && j< sizeof(line)&& !isspace(line[j]))  //长度小于存放数组长度，以及小于当前行长度，
//并且没有遇到宫格之前都是方法的部分
	{
		method[i] = line[j];
		i++;j++;
	}

	method[i] = '\0';   //方法并没有加'\0',手动添加
	if(strcasecmp(method,"GET") == 0)  //是GET方法 忽略大小写比较
	{
		
	} 
	else if(strcasecmp(method,"POST") == 0)  //是POST方法
	{
		cgi = 1;
	}
	else	                        //其他方法	
	{//坏的请求
		clear_header(sock);        //清除头部信息
		status_code = 400;  //错误码统一写404
		goto end;  //
	}
	//GET  /A/B/C?name=zhangsan&sex=man HTTP/1.1
	while(j < strlen(line) && isspace(line[j]))//
	{
		j++;  //跳过方法后面的空格
	}
	i = 0;  //现在让i指向url
	while(i < sizeof(url)-1 && j < sizeof(line) && !isspace(line[j]))
	{
		url[i] = line[j];
		i++, j++;
	}
	url[i] = '\0';
#ifdef DEBUG
	printf("line: %s\n", line);
	printf("method: %s, url: %s\n", method, url);
#endif


//method, url, cgi
	if(strcasecmp(method,"GET") == 0)
	{
			//怎么证明传参了，将url里面的内容分为两部分
			//？前面的是资源，？后面的是参数
			//将？变为'\0'将url变为两个字符串，前面的
			//
		query_string = url;
		while(*query_string)
		{
			if(*query_string == '?')
			{
				*query_string = '\0';
				query_string++;
				cgi = 1;
				break;
			}
			query_string++;
		}
	}	
	


	//////mthod GET/POST,url, GET(query_string),cgi
	////sprintf(path,"www.root%s",url);  //path(wwwroot/, wwwroot/a/b/c.html)  第二个直接把c资源还给了，第一种
//只////传了一个/，我们不可能把我们的根目录全部给他（是我们
//创////建的根目录）我们会吧首页给还回去

	////if(path[strlen(path)-1] == '/')   //看最后一个字符是否是‘/’
	////	{
	

	//method GET/POST, url, GET(query_string), cgi
	sprintf(path, "wwwroot%s", url); 		//path(wwwroot/, wwwrot/a.html)  把wwwroot和url（？前面的）的字符串拼接，放到path	
	if(path[strlen(path) - 1] == '/') //如果左后一个字符是‘/’,将path与字符串连起来
	{
		strcat(path, HOME_PAGE);
	}

	struct stat st;  //stat提取文件属性，能获取文件属性，说明文件存在
	if(stat(path, &st) < 0)
	{
		clear_header(sock);
		status_code = 404;//资源不存在
		goto end;
	}


	else
	{
		if(S_ISDIR(st.st_mode))
		{
			strcat(path, "/");
			strcat(path, HOME_PAGE);
		}
		else if((st.st_mode & S_IXUSR) ||\
				(st.st_mode & S_IXGRP) ||\
				(st.st_mode & S_IXOTH))
		{
			cgi = 1;
		}
		else
		{
			//DO NOTHING
		}
		
		//method, path, query_string, cgi
		//one line
		if(cgi)
		{
			status_code  = exe_cgi(sock, method, path, query_string);	
		}
		else   
		{
			status_code = echo_www(sock, path, st.st_size);	
		}
	}


	
end:
	if(status_code != 200)
	{	
		//异常
		echo_error(sock, status_code);
	}
	close(sock);
}

int startup(int port)
{
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock < 0)
	{
		perror("socket");
		exit(2);
	}

	int opt = 1;
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
	//填充本地消息
	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = htonl(INADDR_ANY);  //连接来时确定IP
	local.sin_port = htons(port);
	
	if(bind(sock,(struct sockaddr*)&local,sizeof(local)) < 0) 
	{
		perror("bind");
		exit(3);
	}

	if(listen(sock,5) < 0)
	{
		perror("listen");		
		exit(4);
	}	
	
	return sock;
} 
// ./httpd 80
int main(int argc,char* argv[])
{
	if( argc != 2)
	{
		usage(argv[0]);
		return 1;
	}
		
	int listen_sock = startup(atoi(argv[1]));

	signal(SIGPIPE,SIG_IGN);
	for(;;)
	{
		struct sockaddr_in client;
		socklen_t len = sizeof(client);
		int sock = accept(listen_sock,\
			(struct sockaddr*)&client,&len);
		if(sock < 0)
		{
			perror("accept");
			continue;		
		}

		printf("get a new client!\n");
		
		pthread_t tid;	   //创建一个线程ID
		pthread_create(&tid,NULL,handler_request,(void *)sock);
		pthread_detach(tid);  //线程分离
	}
	return 0;
} 


























