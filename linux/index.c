#include <stdio.h>
#include "socket.h"
//缓存区大小，16M
//const unsigned int BUFFSIZE = 16777216;
const unsigned int BUFFSIZE = BUFSIZ;

/**
 * mysql 连接结构体
 */
struct mysql_connect_struct
{
    char address[15];
    int port;
};

/**
 * 获取一个mysql写结构
 * @return [description]
 */
struct mysql_connect_struct mysql_write_struct(){
    struct mysql_connect_struct mysql = {"127.0.0.1", 3306};
    return mysql;
}

/**
 * 创建一个mysql连接客户端
 * @param  mysql [description]
 * @return       [description]
 */
int mysql_client(struct mysql_connect_struct mysql){
    int client_sockfd;
    struct sockaddr_in remote_addr; //服务器端网络地址结构体  
    memset(&remote_addr,0,sizeof(remote_addr)); //数据初始化--清零  
    remote_addr.sin_family=AF_INET; //设置为IP通信  
    remote_addr.sin_addr.s_addr=inet_addr(mysql.address);//服务器IP地址  
    remote_addr.sin_port=htons(mysql.port); //服务器端口号  
      
    /*创建客户端套接字--IPv4协议，面向连接通信，TCP协议*/  
    if((client_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)  
    {  
        perror("socket");
        exit(0);
    }  
      
    /*将套接字绑定到服务器的网络地址上*/  
    if(connect(client_sockfd,(struct sockaddr *)&remote_addr,sizeof(struct sockaddr))<0)  
    {  
        perror("connect");
        exit(0);
    }
    printf("connected to mysqld\n");
    
    return client_sockfd;
}

/**
 * mysql传输解析方法
 * @param  buf    [description]
 * @param  bufLen [description]
 * @return        [description]
 */
char* parse_mysql(char buf[], int bufLen){

    if(bufLen > 3){
        printf("%d\n", buf[0]);
        printf("%d\n", buf[1]);
        printf("%d\n", buf[2]);
        int len = (buf[0] * 65536) + (buf[1] * 16 * 16) + buf[2];
        //printf("message len : %d\n", len);
        return "";
    }else{
        return "";
    }
}

/**
 * 主函数
 * @param  argc [description]
 * @param  argv [description]
 * @return      [description]
 */
int main(int argc, char *argv[])  
{  
    int server_sockfd;//服务器端套接字
    struct client_info client;
    int pid;//process id
    unsigned char buf[BUFFSIZE];  //数据传送的缓冲区

    server_sockfd = socket_server(8000);
      
    /*接收客户端的数据并将其发送给客户端--recv返回接收到的字节数，send返回发送的字节数*/  
    while(1){
        /*等待客户端连接请求到达*/
        client = socket_client(server_sockfd);

        if((pid = fork()) > 0){
            //fork success print pid and close client_sockfd
            printf("fork child %d\n", pid);
            close(client.fd);
            continue;
        }else if(pid == -1){
            //fork failed
            perror("fork");
            return 1;
        }
        //child process

        //print client ip addr
        printf("accept client %s\n", client.ip);

        //write tcp client
        struct mysql_connect_struct write = mysql_write_struct();

        int writeClientfd = mysql_client(write);
        char *buffer = NULL;
        size_t len;
        while((len = socket_recv(writeClientfd, &buffer, BUFFSIZE)) > 0){

            if(buffer == NULL){
                printf("wait mysqld response\n");
                continue;
            }

            printf("mysqld response : %s\n", buffer);
            printf("mysqld response len: %d\n", len);

            char* sql = parse_mysql(buffer, len);

            socket_send(client.fd, buffer, len, BUFFSIZE);

            printf("send to client success\n");

            //free malloc
            free(buffer);
            buffer = NULL;
            len = 0;

            while((len = socket_recv(client.fd, &buffer, BUFFSIZE)) > 0){

                if(buffer == NULL){
                    printf("wait client response\n");
                    continue;
                }

                printf("client response : %s\n", buffer);
                printf("client response len: %d\n", len);

                sql = parse_mysql(buffer, len);

                send(writeClientfd, buffer, len, BUFFSIZE);

                printf("send to mysqld success\n");

                //free malloc
                free(buffer);
                buffer = NULL;
                len = 0;

                break;
            }
        }
        printf("client close %s\n", client.ip);
        close(writeClientfd);
        break;
    }  
    close(client.fd);  
    close(server_sockfd);  
    return 0;  
}