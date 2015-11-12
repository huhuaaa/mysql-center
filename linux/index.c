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
 * 获取一个mysql写服务器信息结构
 * @return [description]
 */
struct mysql_connect_struct mysql_write_struct(){
    struct mysql_connect_struct mysql = {"127.0.0.1", 3306};
    return mysql;
}

/**
 * 获取一个mysql读服务器信息结构
 * @return [description]
 */
struct mysql_connect_struct mysql_read_struct(){
    struct mysql_connect_struct mysql = {"192.168.1.135", 3306};
    return mysql;
};

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

    if(bufLen > 4){
        size_t len = buf[0] + buf[1] * 10 + buf[2] * 100;
        printf("message len : %d\n", len);
        printf("message number : %d\n", buf[3]);
        printf("commend id : %d\n", buf[4]);
        char *commend = malloc(len);
        memset(commend, 0, len);
        memcpy(commend, buf + 5, len - 1);
        //memcpy(commend + len - 1, "\0", 1);
        printf("commend : %s\n", commend);
        free(commend);
        return "";
    }else{
        return "";
    }
}
//接受客户端，一次接收完成，大于16M的命令不管
size_t mysql_client_recv(int fd, char **buffer){
    char *header;
    char *body;
    size_t bodyLen;
    size_t len = 0;
    int ret = socket_recv(fd, &header, 4);
    if(ret > 0){
        bodyLen = header[0] + header[1] * 10 + header[2] * 100;
        printf("number : %d\n", header[3]);
        ret = socket_recv(fd, &body, bodyLen);
        if(ret > 0){
            *buffer = malloc(bodyLen + 4);
            memcpy(*buffer, header, 4);
            memcpy(*buffer + 4, body, bodyLen);
            len = ret + 4;
            free(body);
        }
        free(header);
    }
    return len;
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

        //write mysql struct
        struct mysql_connect_struct write = mysql_write_struct();
        //read mysql struct
        struct mysql_connect_struct read = mysql_read_struct();

        int writeClientfd = mysql_client(write);
        int readClientfd = mysql_client(read);
        int select_fd = readClientfd;
        select_fd = writeClientfd > select_fd ? writeClientfd : select_fd;
        select_fd = client.fd > select_fd ? client.fd : select_fd;

        char *buffer = NULL;//数据传送的缓冲区
        size_t len;//用于存取buffer的长度

        //超时设置
        struct timeval timeout = {0, 0};
        fd_set fds;
        int ret;

        //非阻塞方式
        while(1){
            FD_ZERO(&fds); //每次循环都要清空集合，否则不能检测描述符变化
            FD_SET(client.fd, &fds); //添加描述符
            FD_SET(writeClientfd, &fds);
            FD_SET(readClientfd, &fds);

            ret = select(select_fd + 1, &fds, NULL, NULL, &timeout);
            if(ret < 0){
                perror("select");
            }else if(ret > 0){
                if(FD_ISSET(readClientfd, &fds)){
                    //这种方法一次读取的数据太少，那么增加了读写次数效率降低
                    //len = mysql_client_recv(writeClientfd, &buffer);
                    //采用读取固定最大字节，如果没有达到那么缓冲区内有多少取多少
                    len = socket_recv(readClientfd, &buffer, BUFFSIZE);
                    if(len > 0){
                        //printf("mysqld response : %s\n", buffer);
                        //printf("mysqld response len: %d\n", len);

                        socket_send(client.fd, buffer, len, BUFFSIZE);

                        //printf("send to client success\n");

                        //free malloc
                        free(buffer);
                        buffer = NULL;
                        len = 0;
                    }
                }

                if(FD_ISSET(writeClientfd, &fds)){
                    len = socket_recv(writeClientfd, &buffer, BUFFSIZE);
                    if(len > 0){
                        //printf("mysqld response : %s\n", buffer);
                        //printf("mysqld response len: %d\n", len);

                        socket_send(client.fd, buffer, len, BUFFSIZE);

                        //printf("send to client success\n");

                        //free malloc
                        free(buffer);
                        buffer = NULL;
                        len = 0;
                    }
                }

                if(FD_ISSET(client.fd, &fds)){
                    len = mysql_client_recv(client.fd, &buffer);
                    if(len > 0){
                        //printf("client response : %s\n", buffer);
                        //printf("client response len: %d\n", len);

                        char *sql = parse_mysql(buffer, len);

                        send(readClientfd, buffer, len, BUFFSIZE);

                        //printf("send to mysqld success\n");

                        //free malloc
                        free(buffer);
                        buffer = NULL;
                        len = 0;
                    }
                }
            }else{
                //printf("mysqld timeout\n");
            }
        }
    }  
    close(client.fd);  
    close(server_sockfd);  
    return 0;  
}