#include <stdlib.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>
const int SOCKET_ERROR = -1;

/**
 * socket客户端信息结构
 */
struct client_info{
	int fd;
	char *ip;
};

/**
 * 创建本地socket服务
 * @param  port     端口号
 * @return int      返回socket服务ID
 */
int socket_server(size_t port){
    int server_sockfd;
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET; //设置为IP通信  
    my_addr.sin_addr.s_addr = INADDR_ANY;//服务器IP地址--允许连接到所有本地地址上  
    my_addr.sin_port = htons(port); //服务器端口号
    if((server_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)  
    {    
        perror("socket");
    }
    /*将套接字绑定到服务器的网络地址上*/  
    if(bind(server_sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) < 0)  
    {  
        perror("bind");
    }

    //开启监听，监听队列长度为128
    listen(server_sockfd, 128);

    return server_sockfd;
}

/**
 * 等待客户端连接，客户端连接后返回客户端连接信息
 * @param  fd                   连接的服务器端
 * @return struct client_info   客户端信息
 */
struct client_info socket_client(int fd){
	struct client_info client;
	int client_sockfd;//客户端套接字
    struct sockaddr_in remote_addr; //客户端网络地址结构体 
    int sin_size = sizeof(struct sockaddr_in);
    int client_fd = accept(fd, (struct sockaddr *)&remote_addr, &sin_size);
	if(client_fd < 0){
		perror("accept");
	}
	client.fd = client_fd;
	client.ip = inet_ntoa(remote_addr.sin_addr);
	return client;
}

/**
 * socket接收方法
 * @param  fd     套接字
 * @param  buffer 内容指针地址
 * @param  len    读取长度
 * @return size_t
 */
size_t socket_recv(int fd, char **buffer, size_t len){
    int ret = 0;
    char buf[len];
    //printf("copy recv start\n");
    ret = recv(fd, buf, len, 0);
    if(ret > 0){
        //printf("copy recv success\n");
        *buffer = malloc(ret);
        memcpy(*buffer, buf, ret);
    }
    //printf("copy recv end\n");
    return ret;
}

/**
 * socket数据发送方法
 * @param  fd     socket套接字
 * @param  buffer 内容指针
 * @param  len    长度
 * @return size_t
 */
size_t socket_send(int fd, char *buffer, size_t len, size_t send_len){
    int result;
    size_t index = 0;
    size_t buflen;
    //printf("copy send start, totle len : %d\n", len);
    while(1){
        buflen = len - index > send_len ? send_len : len - index;
        //printf("copy send start, bufLen : %d\n", buflen);
        result = send(fd, buffer + index, buflen, 0);
        if(result > 0){
            //printf("copy send success, result : %d\n", result);
            index += buflen;
        } else if(result == 0){
            //printf("close send\n");
            break;
        }else if(result == SOCKET_ERROR){
            perror("send error");
            close(fd);
            break;
        }
        if(index >= len){
            break;
        }
        //printf("copy send continue\n");
    }
    //printf("copy send end\n");
    return len;
}