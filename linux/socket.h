#include <stdlib.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>
const int SOCKET_ERROR = -1;

struct client_info{
	int fd;
	char *ip;
};

/**
 * 创建本地socket服务
 * @param  port [description]
 * @return      [description]
 */
int socket_server(size_t port){
    size_t server_sockfd;
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET; //设置为IP通信  
    my_addr.sin_addr.s_addr = INADDR_ANY;//服务器IP地址--允许连接到所有本地地址上  
    my_addr.sin_port = htons(port); //服务器端口号
    if((server_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)  
    {    
        perror("socket");
        exit(0);
    }
    /*将套接字绑定到服务器的网络地址上*/  
    if(bind(server_sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) < 0)  
    {  
        perror("bind");
        exit(0);
    }

    //开启监听，监听队列长度为128
    listen(server_sockfd, 128);

    return server_sockfd;
}

struct client_info socket_client(int fd){
	struct client_info client;
	int client_sockfd;//客户端套接字
    struct sockaddr_in remote_addr; //客户端网络地址结构体 
    int sin_size = sizeof(struct sockaddr_in);
    int client_fd = accept(fd, (struct sockaddr *)&remote_addr, &sin_size);
	if(client_fd < 0){
		perror("accept");
		exit(0);
	}
	client.fd = client_fd;
	client.ip = inet_ntoa(remote_addr.sin_addr);
	return client;
}

/**
 * socket接收方法
 * @param  fd     [description]
 * @param  buffer [description]
 * @param  len    [description]
 * @return        [description]
 */
size_t socket_recv(size_t fd, char **buffer, size_t len){
    int ret = 0;
    size_t index = 0;
    char buf[len];
    char* tmp;
    while(1){
        printf("copy recv start\n");
        ret = recv(fd, buf, len, 0);
        if(ret > 0){
            printf("copy recv success\n");
            tmp = malloc(index + ret);
            if(*buffer != NULL){
                memcpy(tmp, *buffer, index);
                printf("tmp : %s\n", tmp);
                free(*buffer);
                *buffer = NULL;
            }
            memcpy(tmp + index, buf, ret);
            printf("tmp and buf : %s\n", tmp);
            *buffer = tmp;

            index += ret;
        }else if(ret == 0){
            printf("close recv\n");
            break;
        }else if(ret == SOCKET_ERROR){
            perror("recv error");
            close(fd);
            break;
        }
        if(ret < len){
            break;
        }
        printf("copy recv continue, had buffer : %s\n", *buffer);
    }
    printf("copy recv end\n");
    return index;
}

/**
 * socket数据发送方法
 * @param  fd     [description]
 * @param  buffer [description]
 * @param  len    [description]
 * @return        [description]
 */
size_t socket_send(size_t fd, char *buffer, size_t len, size_t send_len){
    int result;
    size_t index = 0;
    size_t buflen;
    printf("copy send start, totle len : %d\n", len);
    while(1){
        buflen = len - index > send_len ? send_len : len - index;
        printf("copy send start, bufLen : %d\n", buflen);
        result = send(fd, buffer + index, buflen, 0);
        if(result > 0){
            printf("copy send success, result : %d\n", result);
            index += buflen;
        } else if(result == 0){
            printf("close send\n");
            break;
        }else if(result == SOCKET_ERROR){
            perror("send error");
            close(fd);
            break;
        }
        if(index >= len){
            break;
        }
        printf("copy send continue\n");
    }
    printf("copy send end\n");
    return len;
}