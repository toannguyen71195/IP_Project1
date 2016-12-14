/*
    C ECHO client example using sockets
*/
#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
 
#include <stdbool.h>
#include<pthread.h>   //for threading , link with lpthread
#include <time.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/wait.h>
void login(char username[], char password[]);
void get_message(char message[],char username[], char password[]);
bool check_name(char x[], char y[]);
void get_message_status(char message[]);
void get_message_upimage(char message[]);
int main(int argc , char *argv[])
{
    int sock;
    char id[1000];
    struct sockaddr_in server;
    char message[1000] , server_reply[10000],test[1];
	char username[1000];
	char password[1000];
	char error[10];
	int flag =0;
	int share_id=-1;
	int *share_pid_pointer=&share_id;
    pid_t p1;
	pid_t main_process = getpid();
	share_pid_pointer = mmap(NULL, sizeof *share_pid_pointer, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    //Create socket
	int option = 1;
    sock = socket(AF_INET , SOCK_STREAM , 0);
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	printf("1 sock = %d \n",sock);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( 8888 );
 
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
     
    puts("Connected\n");
    int t=0; 
	int i=0;
    //keep communicating with server
    p1=fork();
	//printf("main process = %d, p1= %d \n",main_process,p1);
 if(p1==0){
	 
    while(1)
    {
		//printf("p1 alive pid = %d \n",p1);
		
	if(flag==0){
	printf("Register your Nickname: ");
	login(username,password);
	get_message(message,username,password);
	
	//gets(message);
	for(i=0;i<=20;i++)
	{
		if(message[i]==' ')
		{
			flag=1;
			
			break;
		}
		else if(i>=20)
		{
			printf("Invalid name, Your name is more than 20 character!\n");
			break;
		}
		else if(message[i]=='\0')
		{
			printf("Your Name is: %s \n",message);
	//message=id;
			flag=1;
			break;
		}
		
	}
	
	}
	else{
		//printf("go to else\n");
		bzero(message, strlen(message));
		
        gets(message);
		if (check_name("login",message))
		{
			login(username,password);
			get_message(message,username,password);
		}
		else if (check_name("checkstatus",message))
			get_message_status(message);
		else if (check_name("upimage",message))
			get_message_upimage(message);
		
			
        //fgets(message,sizeof(message),stdin);
		//strtok(message, "\n");
		

	}
		if(flag==1)
        if( send(sock , message , strlen(message) , 0) < 0)
        {
			//printf("go to fail  \n");
            puts("Send failed");
            return 1;
        }
		//printf("go out\n");
        //Receive a reply from the server

    }
  }
 else{
	 (*share_pid_pointer)=p1;
   while(1){
	 //printf("p1 alive pid = %d \n",p1);
		//sleep(1);
	if( recv(sock , server_reply , 2000 , 0) <= 0)
        {
            puts("recv failed");
            break;
        }
		printf("server_reply: %s\n",server_reply);
        puts(server_reply);
		for(i=0;i<10;i++)
				{
					if(server_reply[i]==32 || server_reply[i]=='\0')
						break;
					else
							error[i] = server_reply[i];
					if(strcmp(error,"_EXIST_")==0)
					{
						printf("in.p1= %d\n",p1);
						kill(p1, SIGTERM);
						break;
					}
					
				}
		if (server_reply[0] == '1')
		{
			printf("Now you can choose of one these functions by typing the function name:\n");
			printf("1.checkstatus\n");
			printf("2.upimage\n");
		}
		else if (server_reply[0] == '2')
		{
			printf("you must login again by typing 'login' \n" );
		}
		else if (check_name("ListOnline_quach_tq",server_reply)==0)
		{
			printf("fuck\n");
		}
		bzero(server_reply,strlen(server_reply));
	
    }
  } 
	printf("out.p1= %d\n",p1);
	printf("main process = %d, p1= %d, share id pointer = %d \n",main_process,p1,(*share_pid_pointer));
	
	printf("1\n");
	
	printf("2\n");
	
	
	//free(sock);
	printf("close sock \n");
	//printf("2 sock = %d \n",sock);
	
	kill((*share_pid_pointer), SIGTERM);
	shutdown(sock, SHUT_RDWR);
	close(sock);
	printf("Socket Closed\n");
	fflush(stdout);
    return 0;
}
void login(char username[], char password[])
{
	printf(" input your username:");
	gets(username);
	printf(" input your password:");
	gets(password);
}
void get_message(char message[],char username[], char password[])
{
	char validate[] = "_Validate_";
	strcat(validate,username);
	char temp[] ="_";
	strcat(validate,temp);
	strcat(validate,password);
	strcpy(message,validate);
	
}
bool check_name(char x[], char y[])
{
	int i = 0;
	for (i; i< strlen(x); i++)
	{
		if (x[i] != y[i])
			return false;
	}
	return true;
}
void get_message_status(char message[])
{
	char status[] = "_checkstatus";
	strcpy(message,status);
	
	
}
void get_message_upimage(char message[])
{	
	char image_name[50], title[50], person_upload[50], note[50],type_image[50],size[50];
	printf(" input name of image:");
	gets(image_name);
	printf("input title:");
	gets(title);
	printf("input person upload");
	gets(person_upload);
	printf("input note of image");
	gets(note);
	printf("type of image");
	gets(type_image);
	printf("size of image");
	gets(size);
	char temp ="_upimage_ádsad_";
	char temp2 = "_";
	strcat(temp,image_name);

	
	
}