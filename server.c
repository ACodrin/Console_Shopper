#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 6001

struct client{
    int user_id;
    int selected;
    int count;
    int connected;
    char user_name[1024];
};

struct produs{
    char nume[50];
    char categ[20];
    int stoc;
};

struct data{
    int count;
    int cart[80];
};

static void *thread(void *);
void raspunde(int);

int s_sd, c_sd;
struct sockaddr_in s_addr, c_addr;
socklen_t addr_size;
pthread_t th[100];
int cl_count=0;
char s_send[1024], s_recv[1024];
int n;

FILE* prod;
FILE* dat;
int prod_count, categ_count;
char categorii[5][20];
struct produs produse[40];
struct data arr_data[6];
struct client clienti[100];

int main()
{

    prod=fopen("produse.txt", "r");
    fscanf(prod, "%d", &prod_count);
    for(int i=0; i<prod_count; i++)
    {
        fscanf(prod, "%s %s %d", produse[i].nume, produse[i].categ, &produse[i].stoc);
        for(int j=0; j<strlen(produse[i].nume); j++)
            if(produse[i].nume[j]=='_')
                produse[i].nume[j]=' ';
        if(categ_count==0 || strcmp(categorii[categ_count-1], produse[i].categ)!=0)
        {
            strcpy(categorii[categ_count], produse[i].categ);
            categ_count++;
        }
    }
    fclose(prod);

    dat=fopen("userdata.txt", "r");
    for(int i=1; i<6; i++)
    {
        fscanf(dat, "%d", &arr_data[i].count);
        for(int j=0; j<arr_data[i].count; j++)
            fscanf(dat, "%d", &arr_data[i].cart[j]);
    }
    fclose(dat);

    s_sd = socket(AF_INET, SOCK_STREAM, 0);
    if (s_sd < 0)
    {
        perror("Eroare la socket");
        exit(1);
    }
    printf("Server TCP creat\n");

    memset(&s_addr, '\0', sizeof(s_addr));
    s_addr.sin_family=AF_INET;
    s_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    s_addr.sin_port=htons(PORT);

    n = bind(s_sd, (struct sockaddr*)&s_addr, sizeof(s_addr));
    if (n < 0)
    {
        perror("Eroare la bind");
        exit(1);
    }
    printf("Conectat la portul: %d\n", PORT);

    listen(s_sd, 5);
    while(1)
    {
        printf("Astept clientul...\n");

        addr_size = sizeof(c_addr);
        c_sd = accept(s_sd, (struct sockaddr*)&c_addr, &addr_size);
        cl_count++;
        printf("[client %d] - Client conectat\n", cl_count);
        pthread_create(&th[cl_count], NULL, &thread, (void *)(intptr_t)c_sd);
    }
    
    printf("Serverul se va inchide\n");
    return 0;
}

static void *thread(void * arg)
{
	printf ("[client %d] - Astept mesajul...\n", cl_count);
	fflush (stdout);
	pthread_detach(pthread_self());
	raspunde((intptr_t)arg);
	close ((intptr_t)arg);
	return(NULL);
};

void raspunde(int c_sd)
{
    int id_cl=cl_count;
    char s_command[10][1024];
    clienti[id_cl].user_id=0;
    clienti[id_cl].selected=-1;
    clienti[id_cl].count=-1;
    clienti[id_cl].connected=1;
    while(1)
    {
        bzero(s_recv, 1024);
        recv(c_sd, s_recv, sizeof(s_recv), 0);

        {
        int ctrl=0, j=0;
        for(int i=0; i<=strlen(s_recv); i++)
        {
            if(s_recv[i]==' ' || s_recv[i]=='\0')
            {
                s_command[ctrl][j]='\0';
                if(s_recv[i-1]!=' ')
                    ctrl++;
                j=0;
            }
            else
            {
                s_command[ctrl][j]=s_recv[i];
                j++;
            }
        }
        if(j!=0)
        {
            s_command[ctrl][j+1]='\0';
            ctrl++;
        }
        switch(ctrl)
        {
            case 1:
                printf("[client %d] - %s\n", id_cl, s_command[0]);
                break;
            case 2:
                printf("[client %d] - %s %s\n", id_cl, s_command[0], s_command[1]);
                break;
            case 3:
                printf("[client %d] - %s %s %s\n", id_cl, s_command[0], s_command[1], s_command[2]);
                break;
            default:
                printf("[client %d] - Comanda este prea lunga/scurta\n", id_cl);
                break;
        }
        }

        if(strcmp(s_command[0], "help")==0 && s_command[1][0]=='\0')
        {
            bzero(s_send, 1024);
            strcpy(s_send, "login <user> <password>\nlogout\nshow_all\nshow_categories\nshow_category <category_name>\nshow_prod_info_by_id <product_id>\nshow_prod_info_by_name <category_name>\nselect <product_id>\nset <product_count>\nadd_to_cart\nshow_cart\ndelete_cart\nexit\n");
            send(c_sd, s_send, strlen(s_send), 0);
        }
        else if(strcmp(s_command[0], "login")==0 && s_command[3][0]=='\0')
        {
            if(clienti[id_cl].user_id!=0)
            {
                bzero(s_send, 1024);
                strcpy(s_send, "Un user este deja conectat\n");
                send(c_sd, s_send, strlen(s_send), 0);
            }
            else
            {
                int pos=0;
                while(s_command[2][pos]!='\0')
                {
                    s_command[2][pos]=s_command[2][pos]-5+pos;
                    pos++;
                }

                FILE* usr;
                usr=fopen("conturi.txt", "r");
                int id, already_conn=0;
                char user[1024], pass[1024];
                    
                while(fscanf(usr, "%s %s %d", user, pass, &id)!=-1)
                {
                    if(strcmp(s_command[1], user)==0 && strcmp(s_command[2], pass)==0)
                    {
                        for(int i=1; i<=cl_count; i++)
                        {
                            if(clienti[i].connected==1 && i!=id_cl && clienti[i].user_id==id)
                            {
                                already_conn=1;
                                bzero(s_send, 1024);
                                strcpy(s_send, "Un client este deja conectat cu acest cont\n");
                                send(c_sd, s_send, strlen(s_send), 0);
                                break;
                            }
                        }
                        if(already_conn==0)
                        {
                            bzero(s_send, 1024);
                            strcpy(s_send, "Bine ati revenit, ");
                            strcpy(s_send+strlen(s_send), user);
                            strcpy(s_send+strlen(s_send), "\n");
                            strcpy(clienti[id_cl].user_name, user);
                            send(c_sd, s_send, strlen(s_send), 0);
                            clienti[id_cl].user_id=id;
                            break;
                        }
                    }
                }
                fclose(usr);
                if(clienti[id_cl].user_id==0 && already_conn==0)
                {
                    bzero(s_send, 1024);
                    strcpy(s_send, "Datele introduse pentru login sunt incorecte\n");
                    send(c_sd, s_send, strlen(s_send), 0);
                }
                already_conn=0;
            }
        }
        else if(strcmp(s_command[0], "logout")==0 && s_command[1][0]=='\0')
        {
            if(clienti[id_cl].user_id!=0)
            {
                bzero(s_send, 1024);
                strcpy(s_send, "La revedere, ");
                strcpy(s_send+strlen(s_send), clienti[id_cl].user_name);
                strcpy(s_send+strlen(s_send), "\n");
                clienti[id_cl].user_id=0;
                clienti[id_cl].selected=-1;
                bzero(clienti[id_cl].user_name, 1024);
                send(c_sd, s_send, strlen(s_send), 0);
            }
            else
            {
                bzero(s_send, 1024);
                strcpy(s_send, "Nu este nici un cont conectat\n");
                send(c_sd, s_send, strlen(s_send), 0);
            }
        }
        else if(strcmp(s_command[0], "exit")==0 && s_command[1][0]=='\0')
        {
            clienti[id_cl].user_id=0;
            bzero(clienti[id_cl].user_name, 1024);
            bzero(s_send, 1024);
            strcpy(s_send, "O zi buna\n");
            send(c_sd, s_send, strlen(s_send), 0);
            break;
        }
        else if(strcmp(s_command[0], "show_all")==0 && s_command[1][0]=='\0')
        {
            bzero(s_send, 1024);
            for(int i=0; i<prod_count; i++)
            {
                int len=0, aux=i;
                char str[10];
                if(i==0)
                {
                    str[0]='0';
                    len=1;
                }
                else
                {
                    while(aux)
                    {
                        len++;
                        aux/=10;
                    }
                    aux=i;
                    int j=1;
                    while(aux)
                    {
                        str[len-j]='0'+aux%10;
                        aux/=10;
                        j++;
                    }
                }
                str[len]=' ';
                str[len+1]='\0';
                strcpy(s_send+strlen(s_send), str);
                strcpy(s_send+strlen(s_send), produse[i].nume);
                strcpy(s_send+strlen(s_send), "\n");
            }
            send(c_sd, s_send, strlen(s_send), 0);
        }
        else if(strcmp(s_command[0], "show_categories")==0 && s_command[1][0]=='\0')
        {
            bzero(s_send, 1024);
            for(int i=0; i<categ_count; i++)
            {
                strcpy(s_send+strlen(s_send), categorii[i]);
                strcpy(s_send+strlen(s_send), "\n");
            }
            send(c_sd, s_send, strlen(s_send), 0);
        }
        else if(strcmp(s_command[0], "show_category")==0 && s_command[2][0]=='\0')
        {
            int i;
            for(i=0; i<categ_count; i++)
            {
                if(strcmp(s_command[1], categorii[i])==0)
                {
                    bzero(s_send, 1024);
                    for(int j=0; j<prod_count; j++)
                        if(strcmp(produse[j].categ, categorii[i])==0)
                        {
                            int len=0, aux=j;
                            char str[10];
                            if(j==0)
                            {
                                str[0]='0';
                                len=1;
                            }
                            else
                            {
                                while(aux)
                                {
                                    len++;
                                    aux/=10;
                                }
                                aux=j;
                                int k=1;
                                while(aux)
                                {
                                    str[len-k]='0'+aux%10;
                                    aux/=10;
                                    k++;
                                }
                            }
                            str[len]=' ';
                            str[len+1]='\0';
                            strcpy(s_send+strlen(s_send), str);
                            strcpy(s_send+strlen(s_send), produse[j].nume);
                            strcpy(s_send+strlen(s_send), "\n");
                        }
                    send(c_sd, s_send, strlen(s_send), 0);
                    break;
                }
            }
            if(i==categ_count)
            {
                bzero(s_send, 1024);
                strcpy(s_send, "Categorie invalida\n");
                send(c_sd, s_send, strlen(s_send), 0);
            }
        }
        else if(strcmp(s_command[0], "show_prod_info_by_id")==0 && s_command[2][0]=='\0')
        {
            int id=0, p=10;

            for(int poz=0; poz<strlen(s_command[1]); poz++)
                id=p*id+s_command[1][poz]-'0';

            if(id<=-1 && id>=prod_count)
            {
                bzero(s_send, 1024);
                strcpy(s_send, "ID produs invalid\n");
                send(c_sd, s_send, strlen(s_send), 0);
            }
            else
            {
                char str_id[5], str_stoc[5];
                int len=0, aux=id, k=1;
                char str[10];

                while(aux)
                {
                    len++;
                    aux/=10;
                }
                aux=id;
                while(aux)
                {
                    str_id[len-k]='0'+aux%10;
                    aux/=10;
                    k++;
                }
                str_id[len]=' ';
                str_id[len+1]='\0';

                len=0;
                aux=produse[id].stoc;
                k=1;
                while(aux)
                {
                    len++;
                    aux/=10;
                }
                aux=produse[id].stoc;
                while(aux)
                {
                    str_stoc[len-k]='0'+aux%10;
                    aux/=10;
                    k++;
                }
                str_stoc[len]=' ';
                str_stoc[len+1]='\0';

                bzero(s_send, 1024);
                strcpy(s_send, "ID: ");
                strcpy(s_send+strlen(s_send), str_id);
                strcpy(s_send+strlen(s_send), "\nNume: ");
                strcpy(s_send+strlen(s_send), produse[id].nume);
                strcpy(s_send+strlen(s_send), "\nCategorie: ");
                strcpy(s_send+strlen(s_send), produse[id].categ);
                strcpy(s_send+strlen(s_send), "\nStoc: ");
                strcpy(s_send+strlen(s_send), str_stoc);
                strcpy(s_send+strlen(s_send), "\n");
                send(c_sd, s_send, strlen(s_send), 0);
            }
        }
        else if(strcmp(s_command[0], "show_prod_info_by_name")==0 && s_command[2][0]=='\0')
        {
            int ok=0;

            for(int i=0; i<strlen(s_command[1]); i++)
                if(s_command[1][i]=='_')
                    s_command[1][i]=' ';

            for(int i=0; i<prod_count; i++)
            {
                if(strcasecmp(s_command[1], produse[i].nume)==0)
                {
                    ok=1;
                    char str_id[5], str_stoc[5];
                    int len=0, aux=i, k=1;
                    char str[10];

                    while(aux)
                    {
                        len++;
                        aux/=10;
                    }
                    aux=i;
                    while(aux)
                    {
                        str_id[len-k]='0'+aux%10;
                        aux/=10;
                        k++;
                    }
                    str_id[len]=' ';
                    str_id[len+1]='\0';

                    len=0;
                    aux=produse[i].stoc;
                    k=1;
                    while(aux)
                    {
                        len++;
                        aux/=10;
                    }
                    aux=produse[i].stoc;
                    while(aux)
                    {
                        str_stoc[len-k]='0'+aux%10;
                        aux/=10;
                        k++;
                    }
                    str_stoc[len]=' ';
                    str_stoc[len+1]='\0';

                    bzero(s_send, 1024);
                    strcpy(s_send, "ID: ");
                    strcpy(s_send+strlen(s_send), str_id);
                    strcpy(s_send+strlen(s_send), "\nNume: ");
                    strcpy(s_send+strlen(s_send), produse[i].nume);
                    strcpy(s_send+strlen(s_send), "\nCategorie: ");
                    strcpy(s_send+strlen(s_send), produse[i].categ);
                    strcpy(s_send+strlen(s_send), "\nStoc: ");
                    strcpy(s_send+strlen(s_send), str_stoc);
                    strcpy(s_send+strlen(s_send), "\n");
                    send(c_sd, s_send, strlen(s_send), 0);
                }
            }

            if(ok==0)
            {
                bzero(s_send, 1024);
                strcpy(s_send, "Nume produs invalid\n");
                send(c_sd, s_send, strlen(s_send), 0);
            }
        }
        else if(strcmp(s_command[0], "select")==0 && s_command[2][0]=='\0')
        {
            if(clienti[id_cl].user_id!=0)
            {
                switch(strlen(s_command[1]))
                {
                    case 1:
                        if('0'<=s_command[1][0] && s_command[1][0]<='9')
                        {
                            clienti[id_cl].selected=s_command[1][0]-'0';
                            if(produse[clienti[id_cl].selected].stoc>0)
                            {
                                clienti[id_cl].count=1;
                                bzero(s_send, 1024);
                                strcpy(s_send, "Ati selectat: ");
                                strcpy(s_send+strlen(s_send), produse[clienti[id_cl].selected].nume);
                                strcpy(s_send+strlen(s_send), "\n");
                                send(c_sd, s_send, strlen(s_send), 0);
                            }
                            else
                            {
                                clienti[id_cl].selected=-1;
                                bzero(s_send, 1024);
                                strcpy(s_send, "Acest produs nu mai este in stoc\n");
                                send(c_sd, s_send, strlen(s_send), 0);
                            }
                        }
                        else
                        {
                            bzero(s_send, 1024);
                            strcpy(s_send, "ID produs invalid\n");
                            send(c_sd, s_send, strlen(s_send), 0);
                        }
                        break;
                    case 2:
                        if('1'<=s_command[1][0]&&s_command[1][0]<='9' && '0'<=s_command[1][1] && s_command[1][1]<='9')
                        {
                            clienti[id_cl].selected=s_command[1][1]-'0'+10*(s_command[1][0]-'0');
                            if(clienti[id_cl].selected<prod_count)
                            {
                                if(produse[clienti[id_cl].selected].stoc>0)
                                {
                                    clienti[id_cl].count=1;
                                    bzero(s_send, 1024);
                                    strcpy(s_send, "Ati selectat: ");
                                    strcpy(s_send+strlen(s_send), produse[clienti[id_cl].selected].nume);
                                    strcpy(s_send+strlen(s_send), "\n");
                                    send(c_sd, s_send, strlen(s_send), 0);
                                }
                                else
                                {
                                    clienti[id_cl].selected=-1;
                                    bzero(s_send, 1024);
                                    strcpy(s_send, "Acest produs nu mai este in stoc\n");
                                    send(c_sd, s_send, strlen(s_send), 0);
                                }
                            }
                            else
                            {
                                bzero(s_send, 1024);
                                strcpy(s_send, "ID produs invalid\n");
                                send(c_sd, s_send, strlen(s_send), 0);
                            }
                        }
                        else
                        {
                            bzero(s_send, 1024);
                            strcpy(s_send, "ID produs invalid\n");
                            send(c_sd, s_send, strlen(s_send), 0);
                        }
                        break;
                    default:
                        bzero(s_send, 1024);
                        strcpy(s_send, "ID produs invalid\n");
                        send(c_sd, s_send, strlen(s_send), 0);
                        break;
                }
            }
            else
            {
                bzero(s_send, 1024);
                strcpy(s_send, "Pentru a putea selecta un produs trebuie sa va conectati\n");
                send(c_sd, s_send, strlen(s_send), 0);
            }
        }
        else if(strcmp(s_command[0], "set")==0 && s_command[2][0]=='\0')
        {
            if(clienti[id_cl].user_id!=0)
            {
                if(clienti[id_cl].selected>-1)
                    switch(strlen(s_command[1]))
                    {
                        case 1:
                            if('1'<=s_command[1][0] && s_command[1][0]<='9')
                            {
                                int check=s_command[1][0]-'0';
                                if(check<=produse[clienti[id_cl].selected].stoc)
                                {
                                    clienti[id_cl].count=check;
                                    bzero(s_send, 1024);
                                    strcpy(s_send, "Ati selectat: ");
                                    strcpy(s_send+strlen(s_send), s_command[1]);
                                    strcpy(s_send+strlen(s_send), " x ");
                                    strcpy(s_send+strlen(s_send), produse[clienti[id_cl].selected].nume);
                                    strcpy(s_send+strlen(s_send), "\n");
                                    send(c_sd, s_send, strlen(s_send), 0);
                                }
                                else
                                {
                                    bzero(s_send, 1024);
                                    strcpy(s_send, "Produse insuficiente in stoc\n");
                                    send(c_sd, s_send, strlen(s_send), 0);
                                }
                            }
                            else
                            {
                                bzero(s_send, 1024);
                                strcpy(s_send, "Numar invalid\n");
                                send(c_sd, s_send, strlen(s_send), 0);
                            }
                            break;
                        case 2:
                            if('1'<=s_command[1][0]&&s_command[1][0]<='9' && '0'<=s_command[1][1] && s_command[1][1]<='9')
                            {
                                int check=s_command[1][1]-'0'+10*(s_command[1][0]-'0');
                                if(check<=produse[clienti[id_cl].selected].stoc)
                                {
                                    clienti[id_cl].count=check;
                                    bzero(s_send, 1024);
                                    strcpy(s_send, "Ati selectat: ");
                                    strcpy(s_send+strlen(s_send), s_command[1]);
                                    strcpy(s_send+strlen(s_send), " x ");
                                    strcpy(s_send+strlen(s_send), produse[clienti[id_cl].selected].nume);
                                    strcpy(s_send+strlen(s_send), "\n");
                                    send(c_sd, s_send, strlen(s_send), 0);
                                }
                                else
                                {
                                    bzero(s_send, 1024);
                                    strcpy(s_send, "Produse insuficiente in stoc\n");
                                    send(c_sd, s_send, strlen(s_send), 0);
                                }
                            }
                            else
                            {
                                bzero(s_send, 1024);
                                strcpy(s_send, "Numar invalid\n");
                                send(c_sd, s_send, strlen(s_send), 0);
                            }
                            break;
                        default:
                            bzero(s_send, 1024);
                            strcpy(s_send, "Numar invalid\n");
                            send(c_sd, s_send, strlen(s_send), 0);
                            break;
                    }
                else
                {
                    bzero(s_send, 1024);
                    strcpy(s_send, "Mai intai selectati un produs\n");
                    send(c_sd, s_send, strlen(s_send), 0);
                }
            }
            else
            {
                bzero(s_send, 1024);
                strcpy(s_send, "Pentru a putea seta numarul de produse trebuie sa va conectati\n");
                send(c_sd, s_send, strlen(s_send), 0);
            }
        }
        else if(strcmp(s_command[0], "add_to_cart")==0 && s_command[1][0]=='\0')
        {
            if(clienti[id_cl].user_id!=0)
            {
                if(clienti[id_cl].selected>-1)
                {
                    int found=-1;
                    for(int i=0; i<arr_data[clienti[id_cl].user_id].count; i+=2)
                        if(arr_data[clienti[id_cl].user_id].cart[i]==clienti[id_cl].selected)
                        {
                            found=i;
                            break;
                        }
                    if(found!=-1)
                        arr_data[clienti[id_cl].user_id].cart[found+1]=clienti[id_cl].count;
                    else
                    {
                        arr_data[clienti[id_cl].user_id].cart[arr_data[clienti[id_cl].user_id].count]=clienti[id_cl].selected;
                        arr_data[clienti[id_cl].user_id].cart[arr_data[clienti[id_cl].user_id].count+1]=clienti[id_cl].count;
                        arr_data[clienti[id_cl].user_id].count+=2;
                    }
                    clienti[id_cl].selected=-1;
                    clienti[id_cl].count=0;

                    dat=fopen("userdata.txt", "w");
                    for(int i=1; i<6; i++)
                    {
                        fprintf(dat, "%d ", arr_data[i].count);
                        for(int j=0; j<arr_data[i].count; j+=2)
                            fprintf(dat, "%d %d ", arr_data[i].cart[j], arr_data[i].cart[j+1]);
                        fprintf(dat, "\n");
                    }
                    fclose(dat);

                    bzero(s_send, 1024);
                    if(found==-1)
                        strcpy(s_send, "Produsul a fost adaugat in cos\n");
                    else
                        strcpy(s_send, "Produsul era deja in cos asa ca am modificat doar cantitatea\n");
                    send(c_sd, s_send, strlen(s_send), 0);
                }
                else
                {
                    bzero(s_send, 1024);
                    strcpy(s_send, "Mai intai selectati un produs\n");
                    send(c_sd, s_send, strlen(s_send), 0);
                }
            }
            else
            {
                bzero(s_send, 1024);
                strcpy(s_send, "Pentru a putea adauga un produs in cos trebuie sa va conectati\n");
                send(c_sd, s_send, strlen(s_send), 0);
            }
        }
        else if(strcmp(s_command[0], "show_cart")==0 && s_command[1][0]=='\0')
        {
            if(clienti[id_cl].user_id!=0)
            {
                if(arr_data[clienti[id_cl].user_id].count>0)
                {
                    bzero(s_send, 1024);
                    strcpy(s_send, "Cosul vostru contine:\n");
                    for(int i=0; i<arr_data[clienti[id_cl].user_id].count; i+=2)
                    {
                        strcpy(s_send+strlen(s_send), " ");
                        strcpy(s_send+strlen(s_send), produse[arr_data[clienti[id_cl].user_id].cart[i]].nume);
                        strcpy(s_send+strlen(s_send), " x ");
                        int len=0, aux=arr_data[clienti[id_cl].user_id].cart[i+1];
                        char str[10];
                        while(aux)
                        {
                            len++;
                            aux/=10;
                        }
                        aux=arr_data[clienti[id_cl].user_id].cart[i+1];
                        int k=1;
                        while(aux)
                        {
                            str[len-k]='0'+aux%10;
                            aux/=10;
                            k++;
                        }
                        str[len]=' ';
                        str[len+1]='\0';
                        strcpy(s_send+strlen(s_send), str);
                        strcpy(s_send+strlen(s_send), "\n");
                    }
                    send(c_sd, s_send, strlen(s_send), 0);
                }
                else
                {
                    bzero(s_send, 1024);
                    strcpy(s_send, "Nu aveti produse in cosul de cumparaturi\n");
                    send(c_sd, s_send, strlen(s_send), 0);
                }
            }
            else
            {
                bzero(s_send, 1024);
                strcpy(s_send, "Pentru a putea vedea cosul de cumparaturi trebuie sa va conectati\n");
                send(c_sd, s_send, strlen(s_send), 0);
            }
        }
        else if(strcmp(s_command[0], "delete_cart")==0 && s_command[1][0]=='\0')
        {
            if(clienti[id_cl].user_id!=0)
            {
                if(arr_data[clienti[id_cl].user_id].count>0)
                {
                    arr_data[clienti[id_cl].user_id].count=0;

                    dat=fopen("userdata.txt", "w");
                    for(int i=1; i<6; i++)
                    {
                        fprintf(dat, "%d ", arr_data[i].count);
                        for(int j=0; j<arr_data[i].count; j+=2)
                            fprintf(dat, "%d %d ", arr_data[i].cart[j], arr_data[i].cart[j+1]);
                        fprintf(dat, "\n");
                    }
                    fclose(dat);

                    bzero(s_send, 1024);
                    strcpy(s_send, "Cosul de cumparaturi a fost sters\n");
                    send(c_sd, s_send, strlen(s_send), 0);
                }
                else
                {
                    bzero(s_send, 1024);
                    strcpy(s_send, "Cosul de cumparaturi este deja gol\n");
                    send(c_sd, s_send, strlen(s_send), 0);
                }
            }
            else
            {
                bzero(s_send, 1024);
                strcpy(s_send, "Pentru a putea sterge cosul de cumparaturi trebuie sa va conectati\n");
                send(c_sd, s_send, strlen(s_send), 0);
            }
        }
        else
        {
            bzero(s_send, 1024);
            strcpy(s_send, "Comanda invalida\nDaca sunt nelamuriri, utilizati comanda \"help\"");
            send(c_sd, s_send, strlen(s_send), 0);
        }

        bzero(s_command, 1024*10);
    }
    close(c_sd);
    clienti[id_cl].connected=0;
    printf("[client %d] - Client deconectat\n", id_cl);
}