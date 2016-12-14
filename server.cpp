/**
    Handle multiple socket connections with select and fd_set on Linux
     
    Silver Moon ( m00n.silv3r@gmail.com)
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
#include <fstream>
#define TRUE   1
#define FALSE  0
#define PORT 8888

#define ONLINE 1
#define BUSY 2
#define CALLING 3
#define PENDING 4
#define DISCONNECTED 5

#define NUM_MAX_CLIENT 100

#include<pthread.h> 
#include <string>
using namespace std;
//Struct
	
void *timeout_handler(void *user_store);
char *print_time();
struct Client
{
	char name[50];
	long key;
	int period;
	int status;
	int socket;
	int key_connected;
	int timeout;
	int index;
};
int num_client=0;
int map_id;
struct Client Object_new(char _name[], long _key, int _period,int _status, int _socket) { 
	struct Client p;
	strcpy(p.name,_name);
	p.key = _key;
	p.period = _period;
	p.status = _status;
	p.socket = _socket;
	return p;
};

const char *IMAGE_FILE = "image_data.txt";
struct Image
{
	// imageName_theme_uploader_extension_imageSize_note_fileSize
	char name[50];
	char theme[50];
	char uploader[50];
	char extension[10];
	char imageSize[20];
	char note[50];
	long fileSize;

	Image() {};
	Image(char* _name, char* _theme, char* _uploader, char* _extension, char* _imageSize, char* _note, long _fileSize) {
		strcpy(name,_name);
		strcpy(theme,_theme);
		strcpy(uploader,_uploader);
		strcpy(extension,_extension);
		strcpy(imageSize,_imageSize);
		strcpy(note,_note);
		fileSize = _fileSize;
	}
	void writeToFile(ofstream& fout);
};
Image readImageFromFile(ifstream& fin);
void readAllImage(ifstream& fin, Image* all, int &num);

//Functions
char *action_form_buffer(char buffer[]);
void user_validate(int num_client,char temp2[],struct Client* user_store,long key3,int new_socket);
char* cmd_help();		// list of command
char* cmd_activate();	// activate after deactive
char* cmd_connect();		// connect "key"
char* cmd_accept();		// accept a call
char* cmd_end();			// end/reject a call
char* cmd_deactive();	// incognito mode
char* cmd_disconnect(); // or ctrl + c
void checkstatus(struct Client *user_store);
bool check_name(char x[], char y[]);
void parseStringCommand(char *buffer, char (*command)[50], int &numCommand);
void write_file(ostream&fout,char* name, long pass,struct Client *user_store);
void parseStringSignUp(char *buffer, char *username, char *pass);
int flag9=0;

int main(int argc , char *argv[])
{
	//tri
		struct Client user_store[100];
	int l;
	l=0;
	
	
	char buf[1024];
//	FILE *file;
	size_t nread;
	
	char name2[50];
	long key2;
	int period2;
	int status2;
	int socket2;

	int z,flag;
	z=0;
	flag=10;
	
	

	
    FILE* file = fopen("data_client.txt", "r"); /* should check the result */
    char line[256];

    while (fgets(line, sizeof(line), file)) {
        /* note that fgets don't strip the terminating \n, checking its
           presence would allow to handle lines longer that sizeof(line) */
       // printf("%s", line);
		if (flag ==10)
		{
			num_client = atoi(line);
			//user_store =(struct Client*) malloc(num_client*sizeof(struct Client));
			flag = 0;
		}
		else if (z==2)
		{
			strcpy(name2,line);
			name2[strlen(line)-1] = '\0';
		}
		else if (z==1)
			key2 =(long) atoi(line);
		else if (z==3)
			period2 = atoi(line);
		else if (z == 4)
		{
			socket2 = atoi(line);
		}
		else
		{
			status2 = atoi(line);
			struct Client temp = Object_new(name2,key2,period2,status2,socket2);
			user_store[l] = temp;
			l++;
			z=0;
		}	
		z++;		
    }
    /* may check feof here to make a difference between eof and io failure -- network
       timeout for instance */

    fclose(file);
	
	int h;
	for (h=0; h<num_client; h++)
	{
		printf("%s \n %d \n %d \n",user_store[h].name,user_store[h].period,user_store[h].status);
	}
	//end of tri
    int opt = TRUE;
    int master_socket , addrlen , new_socket , client_socket[30] , max_clients = 30 , activity, i , j , valread , sd;
    int max_sd;
    struct sockaddr_in address;
	char action[10];
	char action2[100];
    char buffer[1025];  //data buffer of 1K
	char server_message[2000];
    
    //set of socket descriptors
    fd_set readfds;
      
    //a message
    char *message="WELCOME \n";
  
    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++) 
    {
        client_socket[i] = 0;
    }
      
    //create a master socket
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
  
    //set master socket to allow multiple connections , this is just a good habit, it will work without thisv
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
  
    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    //bind the socket to localhost port 8888
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);
     
    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
      
    //accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");
     
    while(TRUE) 
    {
        //clear the socket set
        FD_ZERO(&readfds);
  
        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;
         
        //add child sockets to set
        for ( i = 0 ; i < max_clients ; i++) 
        {
            //socket descriptor
            sd = client_socket[i];
             
            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET( sd , &readfds);
             
            //highest file descriptor number, need it for the select function
            if(sd > max_sd)
                max_sd = sd;
        }
  
        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
		printf("activity = %d \n",activity);
        if ((activity < 0) && (errno!=EINTR)) 
        {
            printf("select error");
        }
          
        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(master_socket, &readfds)) 
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
          
            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
        
            //send new connection greeting message
            if( send(new_socket, message, strlen(message), 0) != strlen(message) ) 
            {
                perror("send");
            }
              
            puts("Welcome message sent successfully");
              
            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++) 
            {
                //if position is empty
                if( client_socket[i] == 0 )
                {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);

                    break;
                }
            }
        }
          
        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++) 
        {
            sd = client_socket[i];
            if (FD_ISSET( sd , &readfds)) 
            {
				printf("i_in1= %d \n",i);
                //Check if it was for closing , and also read the incoming message
                if ((valread = read( sd , buffer, 1024)) == 0)
                {
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                      
                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    client_socket[i] = 0;
                }
                else
                {
					printf("i_in2= %d \n",i);
                    //set the string terminating NULL byte on the end of the data read
						buffer[valread] = '\0';
					//2--take action from buffer
					for(j=0;j<valread;j++)
					{
						if(j==0)
						{
							action[0]=buffer[0];
						}
						else if(buffer[j]==' ' || buffer[j]=='\0' || buffer[j]=='_')
						{
							printf("j ===== %d \n",j);
							action[j]='\0';
							break;
						}
						else
							action[j] = buffer[j];	
					}
					strcpy(action2,buffer);
					printf("action = %s \n",action);
					if(strcmp(action,"_Validate")==0)
					{
						//tri
						printf("%s \n",buffer);
						char temp2[50];//name
						char temp3[50];//key
						h=10;
						j=0;
						long key3;
						if (buffer[0] == '_' && buffer[1] == 'V') // accept the protocol "Validate Username_Key"
						{
							while(buffer[h]!= '_')
							{
								temp2[j] = buffer[h];
								j++;h++;
							}
							temp2[h] = '\0';
							h++;
							j=0;
							for (h; h<strlen(buffer); h++)
							{
								temp3[j] = buffer[h];
								j++;
							}
							temp3[strlen(buffer)] ='\0';
							key3 = (long) atoi(temp3);
							printf("%s \n%ld \n",temp2,key3);
					//		void user_validate(int num_client,char[] temp2,struct (Client*) user_store,long key3)
							user_validate(num_client,temp2,user_store,key3,new_socket);
							printf("status= %d\n",user_store[2].status);
							bzero(temp2,strlen(temp2));
							bzero(temp3,strlen(temp3));
						}
						//end of tri
					}
					else
					{
						
						for(j=0;j<num_client;j++)
						{
							if(user_store[j].socket == client_socket[i])
							{
								map_id=j;
								break;
							}
						}
						printf("i_in33= %d \n",i);
							//strcpy(action,"help");
						//2--
						printf("i_in333= %d \n",i);
						//1--First message: validate user in first connection(do only one time)
							if(action=="validate")
							{
								
							}//1--
							else
							{
								
								printf("i_in4= %d \n",i);
							//3--Process commands
								//if(user_store[i]->status == BUSY) // in a conversation
								if(0)
									;//send message to connected user
								else
								{
									printf("i_in5= %s \n",action);

									if(strcmp(action,"_help")==0)
									{
										//message="You're go to help area\n";
										printf("num client = %d\n",map_id);
										//message = cmd_help(map_id, message,user_store);		// list of command
									}
									else if(strcmp(action,"GetOnline") == 0)
									{
										checkstatus(user_store);
									}
									else if(strcmp(action,"Upload") ==0 )
									{
										// Upload_imageName_theme_uploader_extension_imageSize_note_fileSize
										// Upload_toan1_theme_uploader_extension_imageSize_note_1234
										// Upload_toan2_theme_uploader_extension_imageSize_note_1234
										// Upload_toan3_theme_uploader_extension_imageSize_note_1234
										int numCommand = 0;
										char command[20][50];
										parseStringCommand(buffer, command, numCommand);
										
										char imageName[50];
										strcpy(imageName, command[1]);
										printf("%s\n", imageName);

										char theme[50];
										strcpy(theme, command[2]);
										printf("%s\n", theme);

										char uploader[50];
										strcpy(uploader, command[3]);
										printf("%s\n", uploader);

										char extension[10];
										strcpy(extension, command[4]);
										printf("%s\n", extension);

										char imageSize[20];
										strcpy(imageSize, command[5]);
										printf("%s\n", imageSize);

										char note[50];
										strcpy(note, command[6]);
										printf("%s\n", note);

										char size[50];
										strcpy(size, command[7]);
										long fileSize = atol(size);
										printf("%d\n", fileSize);

										Image newImage(imageName,theme,uploader,extension,imageSize,note,fileSize);								
										Image allImage[100];
										int numImage;
										ifstream fin;
										fin.open(IMAGE_FILE);
										readAllImage(fin, allImage, numImage);

										allImage[numImage] = newImage;
										numImage++;
										fin.close();

										ofstream fout;
										fout.open(IMAGE_FILE, std::ios::out | std::ios::trunc);
										fout << numImage << endl;
										for (int i = 0; i < numImage; ++i) {
											allImage[i].writeToFile(fout);
										}
										fout.close();
									}
									else if (strcmp(action, "SignUp") == 0)
									{
										// SignUp_username_password
										char newUser[50], pass[50];
										parseStringSignUp(buffer, newUser, pass);
										int pwd = atoi(pass);
										fstream fout;
										fout.open("data_client.txt", std::ios::out | std::ios::trunc);
										write_file(fout, newUser, pwd, user_store);
										fout.close();

										printf("Signed up %s %d\n", newUser, pwd);

										struct Client temp = Object_new(newUser, pwd, 0, 0, 0);
										user_store[num_client] = temp;
										num_client++;

										send(user_store[map_id].socket, "9", strlen("9"), 0);
									}
									else
									{
										if (flag9 == 1)
										{
											printf("go to send message back111\n");
											int k=0;
											for (k; k<num_client; k++)
											{
												if (user_store[k].key == user_store[map_id].key_connected)
												{
													printf("go to send message back222\n");
													send(user_store[k].socket,action2,strlen(action2),0);
													break;
												}
											}
											printf("go to send message back333\n");
										}
										else
										{
											printf("go to send message back333 %d\n",flag9);
											message="You're go to unrecognize command area\n";
											printf("go to send message back\n");
											//strcpy(buffer,message);
											printf("copy message to buffer successfull n");
											message=NULL;
											//free(message);
											printf("assign message = null\n");
											//send(client_socket[i] , buffer , strlen(buffer) , 0 );
											printf("send message back successful\n");
										}
										//--unrecognize command, print back something
									}
									//--send the message return from those func above
									
								}
								
							//3--
							}
					}
					bzero(action,strlen(action));
					
					
					
					
				
                }// end of else
            }//end of fd_isset
        }// end of for
    }// end of while
      
    return 0;
} //end of main
char *action_form_buffer(char buffer[])
{
	char *message;
	strcpy(message,"help");
	printf("mess = %s \n",message);
	return message;
};

void user_validate(int num_client,char temp2[],struct Client* user_store,long key3,int new_socket)
{
	int i = 0;
	int success_flag = 0;
	int socket_to_send =0;
	for (i; i<num_client; i++)
	{
		if (check_name(temp2,user_store[i].name) && key3 == user_store[i].key)
		{		
			user_store[i].socket = new_socket;
			user_store[i].period = 7;
			user_store[i].status = 1;
			success_flag=1;
			break;
		}
	}
	if(success_flag==1)
	{
		char b[] = "1\n";
		send(new_socket , b , strlen(b) , 0 );
	}
	else
	{
		char b[] = "2\n";
		send(new_socket , b , strlen(b) , 0 );
	}
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

char* cmd_help(int id, char *server_message, struct Client *user_store)
{
	printf("go to help \n");
	char message[1000];
	server_message = message;
	if(user_store[id].status==ONLINE)
		{
			
			printf("go to help1\n");
			strcpy(server_message, "Volcano Team: Here are some helpful command: \n");
			strcat(server_message, "		Type \"help\" to see our service! \n");
			strcat(server_message, "		Type \"connect \"name\"\" to start connect to your friend! \n");
			strcat(server_message, "		    ->\"accept\" to accept a call invitation! \n");
			strcat(server_message, "		    ->\"end\" to end or reject a call! \n");
			strcat(server_message, "		Type \"disconnect\" to disconnect\n");
			
		}
		else
		{
			strcpy(server_message, "Volcano Team: Please hang up first! \n");
		}
		printf("go to help2\n");
	return server_message;
};		// list of command

void checkstatus(struct Client *user_store)
{
	char temp[1000];
	strcat(temp,"ListOnline");
	int i=0;
	
	for (i; i<num_client; i++)
	{
		if (user_store[i].status == 1 && i != map_id)
		{
			
			//sprintf(tempHolder,"%s ", user_store[i].name);
			char temp2[] = "_";
			
			
			
			strcat(temp, temp2);
			strcat(temp, user_store[i].name);
			
		}
	}
	char temp3[] = "\n";
	strcat(temp, temp3);
	send(user_store[map_id].socket,temp,strlen(temp),0);
	bzero(temp,strlen(temp));
}

void parseStringCommand(char *buffer, char (*command)[50], int &numCommand)
{
	string str = buffer;
	int flag = 1;
	int length = 0;
	for (int i = 1; i < str.length(); ++i) {
		if (str[i] == '_') {
			strncpy(command[numCommand], str.substr(flag, length).c_str(), length);
			numCommand++;
			flag += length + 1;
			length = 0;
		}
		else if (i == str.length() - 1) {
			strncpy(command[numCommand], str.substr(flag, length + 1).c_str(), length + 1);
			numCommand++;
		}
		else {
			length++;
		}
	}
}

void write_file(ostream&fout,char* name, long pass,struct Client *user_store)
{
	int temp = num_client + 1;
	int i=0;
	fout << temp << endl;
	for (i; i< num_client ;i++)
	{
		fout << user_store[i].key << endl
			 << user_store[i].name << endl
			 << 0 << endl
			 << 0 << endl
			 << 0 << endl;
	}
	fout << pass << endl
		 << name << endl
		 << 0 << endl
		 << 0 << endl
		 << 0 << endl;
}

void parseStringSignUp(char *buffer, char *username, char *pass) {	
	char *start = strchr(buffer, '_');
	int total = strlen(start);
	string str = start;
	int flag = 1;
	int length = 0;
	for (int i = 1; i < total; ++i) {
		if (str[i] == '_') {
			strncpy(username, str.substr(flag, length).c_str(), length);
			flag += length + 1;
			length = 0;
		}
		else if (i == total - 1) {
			strncpy(pass, str.substr(flag, length + 1).c_str(), length + 1);
		}
		else {
			length++;
		}
	}
}

void Image::writeToFile(ofstream& fout) {
	fout << name << endl;
	fout << theme << endl;
	fout << uploader << endl;
	fout << extension << endl;
	fout << imageSize << endl;
	fout << note << endl;
	fout << fileSize << endl;
}

Image readImageFromFile(ifstream& fin) 
{
	Image image;
	fin >> image.name;
	fin >> image.theme;
	fin >> image.uploader;
	fin >> image.extension;
	fin >> image.imageSize;
	fin >> image.note;
	fin >> image.fileSize;
	return image;
}

void readAllImage(ifstream& fin, Image* allImage, int &numImage) 
{
	fin >> numImage;
	for (int i = 0; i < numImage; ++i) {
		allImage[i] = readImageFromFile(fin);
	}
}







