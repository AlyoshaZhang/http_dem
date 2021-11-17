#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>

#define SERVER_PORT 80

//接受http请求行长度
#define REQUEST_LINE_LEN 256

static int debug = 1;

int get_line(int client_sock, char* buf, int size);
void do_http_request(int client_sock);
void do_http_response(int client_sock);

int main(void){
	int sock;			//socket 套接字 
	struct sockaddr_in server_addr;
	
	//创建信箱
	sock =socket(AF_INET, SOCK_STREAM, 0);
	
	//2.清空标签，写上地址和端口号
	bzero(&server_addr, sizeof(server_addr));
	
	server_addr.sin_family = AF_INET;			//AF_INET 网络协议家族		选择协议族
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);		//监听本地所有IP地址
	server_addr.sin_port = htons(SERVER_PORT);			//绑定端号
	
	//实现标签到收信的信箱上	绑定
	bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
	
	//表示同一时用户端可发起连接的数量
	listen(sock, 128);
	
	printf("等待客户端的连接.......\n");
	
	int done = 1;

	while(done){
		struct sockaddr_in client;
		int client_sock, len, i;
		char client_ip[64];
		char buf[256];

		socklen_t client_addr_len;
		client_addr_len = sizeof(client);
		client_sock = accept(sock, (struct sockaddr *)&client, &client_addr_len);

		printf("client ip: %s\t port: %d\n",inet_ntop(AF_INET, &client.sin_addr.s_addr, client_ip, sizeof(client_ip)),ntohs(client.sin_port));

	//处理http请求，读取客户端发送的数据
		do_http_request(client_sock);
		close(client_sock);
		
	}

	
	return 0;

}

//接收 http 请求
/*读取客户端发送的 http 请求 */
void do_http_request(int client_sock){
	
	int len = 0;
	char buf[REQUEST_LINE_LEN];
	char method[64];
	char url[REQUEST_LINE_LEN];	
	char path[512];


/*	do{	
		//1.读取请求行
		len = get_line(client_sock, buf, sizeof(buf));
		printf("read line: %s\n", buf);
	}while(len > 0);
*/

	//1.读取请求行
	len = get_line(client_sock, buf, sizeof(buf));

	int i = 0, j = 0;
	if(len > 0){
		while(!isspace(buf[j]) && i < sizeof(method)-1){
			method[i] = buf[j];
			i++;
			j++;
		}
		
		method[i] = '\0';
		if(debug)
			printf("request method: %s \n", method);
	}else{
	//请求格式有问题，出错处理
	//bad	

	}
	
	//只处理 get 请求
	if(!strncasecmp(method, "GET", i)){
		if(debug){
			printf("method = GET\n");
		}
		
		while(isspace(buf[j++]));		//跳过白空格
		i = 0;
		while(!isspace(buf[j]) && i < sizeof(url)-1){
			url[i] = buf[j];
			i++;
			j++;
		}
		url[i] = '\0';
		if(debug){
			printf("url: %s\n", url);
		}
		
		//定位服务器本地的html文件
		//处理url 中的？
		{
			char* pos = strchr(url,'?');
			if(pos){ 
				*pos = '\0';
				printf("real url: %s\n", url);
			}
		}

		sprintf(path, "./html_docs/%s",url);
		if(debug){
			printf("path: %s\n", path);
		}
		
	}else{//非GET请求，读取http头部，并响应客户端501 Method Not Implemented
		fprintf(stderr, "other request [%s]\n", method);

		//unimplemented(client_sock);	//在响应实现
	}
	
	//继续读取http 头部
	do{
		len = get_line(client_sock, buf, sizeof(buf));
		if(debug){
			printf("read: %s\n", buf);
		}
	}while(len > 0);


	
}

void do_http_response(int client_sock){
	const char* main_header = "HTTP/1.0 200 OK\r\nServer: Dawenwen Server\r\nContent-Type: text/html\r\nConnection: Close\r\n";
	const char* welcome_content = "<html lang=\"zh-CN\">\n\
					<head>\n\
					<meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\">\n\
					<title>This is a test</title>\n\
					</head>\n\
					<body>\n\
					<div align=center height=\"500px\" >\n\
					<br/><br/><br/>\n\
					<h2>大家好，欢迎来到奇牛学院VIP 课！</h2><br/><br/>\n\
					<form action=\"commit\" method=\"post\">\n\
					姓名: <input type=\"text\" name=\"name\" />\n\
					<br/>年龄: <input type=\"password\" name=\"age\" />\n\
					<br/><br/><br/><input type=\"submit\" value=\"提交\" />\n\
					<input type=\"reset\" value=\"重置\" />\n\
					</form>\n\
					</div>\n\
					</body>\n\
					</html>\n";

	//1.送main_header
	int len = write(client_sock, main_header, strlen(main_header));
	if(debug){
		fprintf(stdout, "... do_http_response...\n");
	}
	if(debug){
		fprintf(stdout, "write[%d]: %s", len, main_header);
	}
		
	
	//2.生成Content-Length 行并发送
	char send_buf[64];
	int wc_len = strlen(welcome_content);
	len = snprintf(send_buf, 64, "Content-Length: %d\r\n\r\n", wc_len);
	len = write(client_sock, send_buf, len);
	if(debug){
		fprintf(stdout, "write[%d]: %s\n", len, send_buf);
	}
	
	len = write(client_sock, welcome_content, wc_len);
	if(debug){
		fprintf(stdout, "write[%d]: %s\n", len, welcome_content);
	}
	//3.发送html 文件内容
	

}



int get_line(int client_sock, char* buf, int size){
	char ch = '\0';
	int count = 0;	
	int len = 0;

	while(count < size - 1){
		len = read(client_sock, &ch, 1);
		if(len == 1){
			if(ch == '\r'){
				continue;
			}else if(ch == '\n'){
				buf[count] = '\0';
				break;
			}

			buf[count] = ch;
			count++;
		}else if(len == -1){
			//读取出错
			perror("read failed");
			count = -1;
			break;
		}else if(len == 0){
			//客户端关闭sock 连接
			fprintf(stderr, "client close.\n");
			count = -1;
			break;
		}
	}
	if(buf[count] != '\0'){
		fprintf(stderr, "warnning: not enough buf.\n");
		buf[count] = '\0';
	}
	

	return len;
}























































