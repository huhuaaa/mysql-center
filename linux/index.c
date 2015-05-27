#include <stdio.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>
#include <mysql/mysql.h>

struct mysql_connect_struct
{
    char address[15];
    int port;
};

struct mysql_connect_struct mysql_write_struct(){
    struct mysql_connect_struct mysql = {"127.0.0.1", 3306};
    return mysql;
}

int createClient(struct mysql_connect_struct mysql){
    int client_sockfd;  
    int len;  
    struct sockaddr_in remote_addr; //服务器端网络地址结构体  
    unsigned char buf[BUFSIZ];  //数据传送的缓冲区  
    memset(&remote_addr,0,sizeof(remote_addr)); //数据初始化--清零  
    remote_addr.sin_family=AF_INET; //设置为IP通信  
    remote_addr.sin_addr.s_addr=inet_addr(mysql.address);//服务器IP地址  
    remote_addr.sin_port=htons(mysql.port); //服务器端口号  
      
    /*创建客户端套接字--IPv4协议，面向连接通信，TCP协议*/  
    if((client_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)  
    {  
        perror("socket");  
        return 1;  
    }  
      
    /*将套接字绑定到服务器的网络地址上*/  
    if(connect(client_sockfd,(struct sockaddr *)&remote_addr,sizeof(struct sockaddr))<0)  
    {  
        perror("connect");  
        return 1;  
    }  
    printf("connected to server\n");
    
    return client_sockfd;
}


char* parse_mysql(char buf[]){
    int bufLen = sizeof(buf);
    printf("bufLen : %d\n", bufLen);
    int i = 0;
    while(i < bufLen){
        printf("%d\n", buf[i]);
        i++;
    }
    if(bufLen > 3){
        printf("%d\n", buf[0]);
        printf("%d\n", buf[1]);
        printf("%d\n", buf[2]);
        int len = (buf[0] * 65536) + (buf[1] * 16 * 16) + buf[2];
        printf("message len : %d\n", len);
        //char* parse;
        //memset(&parse, 0, len);
        return "";
    }else{
        return "";
    }
}
  
int main(int argc, char *argv[])  
{  
    int server_sockfd;//服务器端套接字  
    int client_sockfd;//客户端套接字  
    int len;
    int pid;//process id
    struct sockaddr_in my_addr;   //服务器网络地址结构体  
    struct sockaddr_in remote_addr; //客户端网络地址结构体  
    int sin_size;  
    unsigned char buf[BUFSIZ];  //数据传送的缓冲区
    unsigned char *ip;
    memset(&ip,0,15);
    memset(&my_addr,0,sizeof(my_addr)); //数据初始化--清零  
    my_addr.sin_family=AF_INET; //设置为IP通信  
    my_addr.sin_addr.s_addr=INADDR_ANY;//服务器IP地址--允许连接到所有本地地址上  
    my_addr.sin_port=htons(8000); //服务器端口号
      
    /*创建服务器端套接字--IPv4协议，面向连接通信，TCP协议*/  
    if((server_sockfd = socket(PF_INET,SOCK_STREAM,0)) < 0)  
    {    
        perror("socket");  
        return 1;  
    }
   
        /*将套接字绑定到服务器的网络地址上*/  
    if (bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr)) < 0)  
    {  
        perror("bind");  
        return 1;
    }

    /*监听连接请求--监听队列长度为5*/  
    listen(server_sockfd,5);

    sin_size = sizeof(struct sockaddr_in);
      
    /*接收客户端的数据并将其发送给客户端--recv返回接收到的字节数，send返回发送的字节数*/  
    while(1){

        /*等待客户端连接请求到达*/
        if((client_sockfd = accept(server_sockfd,(struct sockaddr *)&remote_addr,&sin_size))<0)  
        {
            perror("accept");  
            return 1;  
        }

        if((pid = fork()) > 0){
            //fork success print pid and close client_sockfd
            printf("fork child %d\n", pid);
            close(client_sockfd);
            continue;
        }else if(pid == -1){
            //fork failed
            perror("fork");
            return 1;
        }
        //child process

        //get client ip
        ip = inet_ntoa(remote_addr.sin_addr);
        //print client ip addr
        printf("accept client %s\n", ip);

        //write tcp client
        struct mysql_connect_struct write = mysql_write_struct();

        int writeClientfd = createClient(write);

        while((len = recv(writeClientfd, buf, BUFSIZ, 0)) > 0){

            printf("mysqld response : %s\n", buf);
            
            char* sql = parse_mysql(buf);

            if(send(client_sockfd, buf, len, 0) < 0){
                perror("write");
                return 1;
            }

            if((len = recv(client_sockfd,buf,BUFSIZ,0)) > 0){
                //buf[len] = "/0";
                printf("client response : %s\n", buf);
                if(send(writeClientfd, buf, len, 0) < 0)  
                {
                    perror("write");
                    return 1;  
                }
            }
        }
        printf("client close %s\n", ip);
        close(writeClientfd);
        break;
    }  
    close(client_sockfd);  
    close(server_sockfd);  
    return 0;  
}