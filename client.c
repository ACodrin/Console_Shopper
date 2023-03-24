#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 6001

int main(int argc, char *argv[]){
    int sd;
    struct sockaddr_in addr;
    socklen_t addr_size;
    char c_send[512], c_recv[512];
    int n;
    bool out=0;

    if (argc != 2)
    {
        printf ("Sintaxa: ./client <adresa_server>\n");
        return -1;
    }

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0){
        perror("Eroare la socket");
        exit(-1);
    }
    printf("Socketul pentru serverul TCP a fost creat\n");

    memset(&addr, '\0', sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=inet_addr(argv[1]);
    addr.sin_port=htons(PORT);

    if(connect(sd, (struct sockaddr*)&addr, sizeof(addr))==-1)
    {
        perror("Eroare la conectare");
        exit(-1);
    }
    else
        printf("Conectat la server\n");

    while(1){
        bzero(c_send, 512);
        fgets(c_send, sizeof(c_send), stdin);
        c_send[strlen(c_send)-1]='\0';
        send(sd, c_send, strlen(c_send), 0);

        bzero(c_recv, 512);
        recv(sd, c_recv, sizeof(c_recv), 0);
        printf("%s\n", c_recv);
        if(strcmp(c_recv, "O zi buna\n")==0)
            break;
    }
    close(sd);
    printf("Deconectat de la server\n");

    return 0;

}