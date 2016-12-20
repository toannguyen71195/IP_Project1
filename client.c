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
void get_message_searchimage(char message[]);
void get_message_downimage(char message[]);
void sendImage(char imagePath[], int socket);
void receiveImage(char filePath[], int size, int socket);

char username[1000];
char password[1000];
char global_image_path[200];
long global_image_size;

bool waitforimage = false;

int main(int argc , char *argv[])
{
    int sock;
    char id[1000];
    struct sockaddr_in server;
    char message[1000] , server_reply[10000],test[1];
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
	
	if(p1==0)
	{
		while(1)
		{
			//printf("p1 alive pid = %d \n",p1);
			bool b_upimage = false;
			
			if(flag==0) 
			{
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
			else 
			{
				//printf("go to else\n");
				bzero(message, strlen(message));

				gets(message);
				if (check_name("login",message))
				{
					login(username,password);
					get_message(message,username,password);
				}
				else if (check_name("checkstatus",message))
				{
					get_message_status(message);
				}
				else if (check_name("upimage",message))
				{
					get_message_upimage(message);
					b_upimage = true;
					printf("path %s\nsize %d\n", global_image_path, global_image_size);
				}
				else if (check_name("searchimage",message))
				{
					get_message_searchimage(message);
				}
				else if (check_name("downimage",message))
				{
					get_message_downimage(message);
					// waitforimage = true;
				}
				//fgets(message,sizeof(message),stdin);
				//strtok(message, "\n");
			}
			if(flag==1) 
			{
				if (b_upimage)
				{
					if (global_image_size == 0)
					{
						printf("Invalid image!\n");
					}
					else
					{
						// send image info
						b_upimage = false;						
						if (send(sock , message , strlen(message) , 0) < 0)
						{
							puts("Send failed");
							return 1;
						}
						printf("Send mes %s\n", message);
						printf("path %s\nsize %d\n", global_image_path, global_image_size);
					}
				}
				else 
				{
					if (send(sock , message , strlen(message) , 0) < 0)
					{
						//printf("go to fail  \n");
						puts("Send failed");
						return 1;
					}
					printf("Send mes %s\n", message);
				}
				//printf("go out\n");
				//Receive a reply from the server
			}

		} // end of while
	} // end of if
	else
	{
		(*share_pid_pointer)=p1;
		while(1)
		{
			if (!waitforimage)
			{
			 	//printf("p1 alive pid = %d \n",p1);
				//sleep(1);
				if (recv(sock , server_reply , 1024 , 0) <= 0)
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
					printf("3.searchimage\n");
					printf("4.downimage\n");
				}
				else if (server_reply[0] == '2')
				{
					printf("you must login again by typing 'login' \n" );
				}
				else if (server_reply[0] == '3') // rcv meta success
				{
					printf("Meta data accepted\n");
					char imagePath[200];
					strcpy(imagePath, "test2.jpg");
					printf("sock %d\n", sock);
					sendImage(imagePath, sock);
				}
				else if (server_reply[0] == '4') // rcv meta fail
				{
					printf("Send meta fail\n");
				}
				else if (server_reply[0] == '5') // rcv image success
				{
					printf("Send image success\n");
				}
				else if (server_reply[0] == '6') // rcv image fail
				{
					printf("Send image fail\n");
				}
				else if (server_reply[0] == '7') // rcv image fail
				{
					waitforimage = true;
					printf("Wait image\n");
					printf("ok\n");
				}
				else if (server_reply[0] == '8') // rcv image fail
				{
					printf("not found\n");
				}
				else if (check_name("ListOnline_quach_tq",server_reply)==0)
				{
					printf("fuck\n");
				}
				else if (check_name("List", server_reply)==0)
				{
					//printf("%s\n", server_reply);
				}
			}
			else 
			{ // waitforimage
				printf("Receive image\n");
				char filePath[200];
				strcpy(filePath, "download.jpg");
				int size = 33176;
				receiveImage(filePath, size, sock);
				waitforimage = false;
				printf("Finisehd rcv\n");
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
	// Upload_imageName_theme_uploader_extension_imageSize_note_fileSize
	char image_name[50], theme[50], note[50], type_image[50], size[50], file_size[10], path[200];
	printf("Input name of image: ");
	gets(image_name);
	printf("Input theme: ");
	gets(theme);
	printf("Input note of image: ");
	gets(note);
	printf("Type of image: ");
	gets(type_image);
	printf("Image size: ");
	gets(size);
	printf("Input image path: ");
	gets(path);
	strcpy(global_image_path, path);
	//Get Picture Size
	FILE *picture;
	picture = fopen(global_image_path, "r");
	fseek(picture, 0, SEEK_END);
	long fsize = ftell(picture);
	global_image_size = fsize;
	printf("path %s\nsize %d\n", global_image_path, global_image_size);
	fseek(picture, 0, SEEK_SET);
	
	char tmp_message[500] = "Upload";
	strcat(tmp_message, "_");
	strcat(tmp_message, image_name);
	strcat(tmp_message, "_");
	strcat(tmp_message, theme);
	strcat(tmp_message, "_");
	strcat(tmp_message, username);
	strcat(tmp_message, "_");
	strcat(tmp_message, type_image);
	strcat(tmp_message, "_");
	strcat(tmp_message, size);
	strcat(tmp_message, "_");
	strcat(tmp_message, note);
	strcat(tmp_message, "_");
	sprintf(file_size, "%d", global_image_size);
	strcat(tmp_message, file_size);
	strcpy(message, tmp_message);
}

void get_message_searchimage(char message[])
{
	// Search_theme_uploader
	char theme[50], uploader[50];
	printf("Input theme: ");
	gets(theme);
	printf("Input uploader of image: ");
	gets(uploader);
	char tmp_message[100] = "Search_";
	strcat(tmp_message, theme);
	strcat(tmp_message, "_");
	strcat(tmp_message, uploader);
	strcpy(message, tmp_message);
}
void get_message_downimage(char message[])
{
	char name[50];
	printf("Input name: ");
	gets(name);
	char tmp_message[100] = "Down_";
	strcat(tmp_message, name);
	strcpy(message, tmp_message);
}

void sendImage(char imagePath[], int socket) 
{
	//Get Picture Size
	FILE *picture;
	picture = fopen(imagePath, "r");
	fseek(picture, 0, SEEK_END);
	long fsize = ftell(picture);
	fseek(picture, 0, SEEK_SET);

	printf("path %s\nsize %d\n", imagePath, fsize);
	
	// convert picture
	char file_buffer[fsize];
	fread(file_buffer, 1, fsize, picture);

	printf("sock %d\n", socket);

	char send_buffer[fsize + 6];
	memcpy(send_buffer, "Image_", 6);
	memcpy(send_buffer + 6, file_buffer, fsize);
	
	printf("Send buffer: %s", send_buffer);

	char send_fragment[1024];
	int loop = (fsize+6)/1024;
	for (int i = 0; i < loop; ++i) {
		memcpy(send_fragment, send_buffer + (i*1024), 1024);
		send(socket, send_fragment, 1024,0);
	}
}

void receiveImage(char filePath[], int size, int socket)
{
	int loop = size/1024;
	char file_buffer[size];
	char rbuffer[1024];
	for (int i = 0; i < loop; ++i)
	{
		read(socket, rbuffer, 1024);
		memcpy(file_buffer + i*1024, rbuffer, 1024);
	}

	//Convert it Back into Picture
	printf("Converting Byte Array to Picture\n");
	FILE *image;
	image = fopen(filePath, "wb");
	fwrite(file_buffer, 1, size, image);
	fclose(image);
	printf("Converted\n");
}




