#include<stdio.h>
#include<conio.h>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include"stdafx.h"
#include"targetver.h"
#define BUFF_SIZE 1024

SOCKET clients[1024];//conn Socket to connect between client and server
int g_client = 0;//the number of the conn Sock

SOCKET listenSock;
SOCKET connSock;

sockaddr_in serverAddr;
sockaddr_in clientAddr;
sockaddr_in new_clientAddr;

char send_buffer[BUFF_SIZE];
char receive_buffer[BUFF_SIZE];

int bytes;
int n;

DWORD WINAPI ClientThread(LPVOID arg);

void main(int argc,char **argv) {
	//Init Winsock Version
	WSADATA wsaData;
	DWORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Error Code: %d", WSAGetLastError());
	}

	//Init listen socket and bind address for its
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//TCP Socket
	//sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(5000);
	serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
		printf("Error Code: %d", WSAGetLastError());
	}

	//listen
	listen(listenSock, 10);

	//Waiting for client's connection
	while (1) {
		//Id of Thread
		DWORD Id;
		//sockaddr_in clientAdrr;
		int clientAddrLen = sizeof(clientAddr);

		//accept connection
		connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);
		clients[g_client] = connSock;
		g_client++;
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ClientThread, (LPVOID)connSock, 0, &Id);
	}
}

DWORD WINAPI ClientThread(LPVOID arg) {
	printf("Accept a connection from client IP %s port %d ID: %d \n",
		inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), GetCurrentThreadId());

	//Respond with welcome message
	char *welcome = "\n220 Welcome to Alan's FTP site== \r\n\n";
	send(connSock, welcome, strlen(welcome), 0);
	SOCKET connSock = (SOCKET)arg;
	char command[1024];
	bool checkUserLogin = false;
	bool checkUser = false;

	while (true) {
		n = 0;
		while (true) {
			memset(receive_buffer, 0, 1024);
			memset(command, 1024, 0);
			recv(connSock, receive_buffer, 1023, 0);
			if (strncmp(receive_buffer, "USER: ", 6) == 0 || strncmp(receive_buffer, "PASS: ", 6) == 0 ||
				strncmp(receive_buffer, "SYST: ", 6) == 0 || strncmp(receive_buffer, "PORT: ", 6) == 0 ||
				strncmp(receive_buffer, "STOR: ", 6) == 0 || strncmp(receive_buffer, "RETR: ", 6) == 0 ||
				strncmp(receive_buffer, "LIST: ", 6) == 0 || strncmp(receive_buffer, "NLST: ", 6) == 0 ||
				strncmp(receive_buffer, "QUIT: ", 6) == 0) {
				strcpy(command, receive_buffer + 6);
				if (strlen(command) > 0) {
					while (command[strlen(command) - 1] == '\n'
						|| command[strlen(command) - 1] == '\r'
						|| command[strlen(command) - 1] == '\t'
						|| command[strlen(command) - 1] == ' ') {
						command[strlen(command) - 1] = 0;
					}
					break;
				}
				else {
					//re-send
					char *error = "202 Command not implemented, superfluous at this site. \r\n";
					send(connSock, error, strlen(error), 0);
				}
			}
			else {
				//re-send
				char *error = "202 Command not implemented, superfluous at this site. \r\n";
				send(connSock, error, strlen(error), 0);
			}
		}//end command

		//received command by server
		//USER
		if (strncmp(receive_buffer, "USER: ", 6) == 0)
		{
			//memset(send_buffer, 0, 1024);
			sprintf(send_buffer, "331 User name okay, need password \r\n");
			bytes = send(connSock, send_buffer, strlen(send_buffer), 0);
			checkUser = true;
			if (bytes < 0) break;
		}

		//PASS
		if (strncmp(receive_buffer, "PASS: ", 6) == 0)
		{
			//memset(send_buffer, 0, 1024);
			if (checkUser) {
				sprintf(send_buffer, "230 Public login sucessful \r\n");
				bytes = send(connSock, send_buffer, strlen(send_buffer), 0);
				checkUserLogin = true;
			}
			else {
				sprintf(send_buffer, "231 User logged out; service terminated\r\n");
				bytes = send(connSock, send_buffer, strlen(send_buffer), 0);
			}
			if (bytes < 0) break;
		}

		//SYST: require system type
		if (strncmp(receive_buffer, "SYST: ", 6) == 0)
		{
			if (checkUserLogin) {
				system("cd RemoteHost&&ver > syst.txt");

				FILE *fin = fopen("RemoteHost/syst.txt", "r");//open tmp.txt file
															  //(send_buffer, 0, 1024);
				sprintf(send_buffer, "150 Transfering... \r\n");//send_buffer will pointer to string in sprintf
				bytes = send(connSock, send_buffer, strlen(send_buffer), 0);
				char temp_buffer[80];
				while (!feof(fin))
				{
					fgets(temp_buffer, 78, fin);//read content of file tmp.txt and save in temp_buffer
					sprintf(send_buffer, "%s", temp_buffer);//send_buffer will pointer to string in temp_buffer
					send(connSock, send_buffer, strlen(send_buffer), 0);
				}
				fclose(fin);
				sprintf(send_buffer, "226 File transfer completed... \r\n");
				bytes = send(connSock, send_buffer, strlen(send_buffer), 0);
			}
			else
			{
				sprintf(send_buffer, "231 User logged out; service terminated\r\n");
				bytes = send(connSock, send_buffer, strlen(send_buffer), 0);
			}
			if (bytes < 0) break;
		}

		//PORT: Specifies an address and port to which the server should connect
		// PORT 20: data transfer(default)
		// PORT 21: data connection(default)
		if (strncmp(receive_buffer, "PORT: ", 6) == 0)
		{
			if (checkUserLogin) {
				SOCKET data_connSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				unsigned char new_port[2];// specific by client
				int new_ip[4];//specific by client
				int dst_port;// for sockaddr_in
				char dst_ip[40];// for sockaddr_in
				sscanf(receive_buffer, "PORT: %d,%d,%d,%d,%d,%d", &new_ip[0], &new_ip[1], &new_ip[2], &new_ip[3], &new_port[0], &new_port[1]);

				//sockaddr_in new_clientAddr;
				dst_port = new_port[0] * 256 + new_port[1];
				sprintf(dst_ip, "%d.%d.%d.%d", new_ip[0], new_ip[1], new_ip[2], new_ip[3]);
				new_clientAddr.sin_family = AF_INET;
				new_clientAddr.sin_port = htons(dst_port);
				new_clientAddr.sin_addr.S_un.S_addr = inet_addr(dst_ip);

				//connect to new client
				if (connect(data_connSock, (sockaddr*)&new_clientAddr, sizeof(new_clientAddr))) {
					printf("Try connection in %s:%d", inet_ntoa(new_clientAddr.sin_addr), ntohs(new_clientAddr.sin_port));
					sprintf(send_buffer, "425 Something is wrong, can't start the active connection... \r\n");
					bytes = send(connSock, send_buffer, sizeof(send_buffer), 0);
				}
				else {
					printf("Data connection to client created (active connection) \n");
					sprintf(send_buffer, "200 OK \r\n");
					bytes = send(connSock, send_buffer, sizeof(send_buffer), 0);
				}
			}
			else {
				sprintf(send_buffer, "231 User logged out; service terminated\r\n");
				bytes = send(connSock, send_buffer, strlen(send_buffer), 0);
			}

		}

		//STOR: Accept the data and to store the data as a file at the server site
		if (strncmp(receive_buffer, "STOR: ", 6) == 0)
		{	
			if (checkUserLogin) {
				//printf("%s", command); ->Log.d("myLog",command);
				//"command" is file name that is sent to server
				if (fopen(command, "r") == NULL) {
					sprintf(send_buffer, "450 Requested file action not taken. \r\n");
					send(connSock, send_buffer, sizeof(send_buffer), 0);
				}
				else {
					sprintf(send_buffer, "150 Transfering... \r\n");
					bytes = send(connSock, send_buffer, strlen(send_buffer), 0);

					//handle file
					FILE *fp = fopen(command, "r");
					char ch;
					FILE *new_file = fopen("RemoteHost/SuperMatch.txt", "w");
					while ((ch = fgetc(fp)) != EOF) {
						//printf("%c", ch); ->Log
						fputc(ch, new_file);
					}
					fclose(new_file);
					fclose(fp);

					sprintf(send_buffer, "226 File transfer completed... \r\n");
					bytes = send(connSock, send_buffer, strlen(send_buffer), 0);
				}
			}
			else {
				sprintf(send_buffer, "231 User logged out; service terminated\r\n");
				bytes = send(connSock, send_buffer, strlen(send_buffer), 0);
			}
		}

		//RETR: download a copy of a file on the server
		if (strncmp(receive_buffer, "RETR: ", 6) == 0)
		{
			if (checkUserLogin) {
				//printf("%s", command); ->Log.d("myLog",command);
				//"command" is file name that is sent to client
				if (fopen(command, "r") == NULL) {
					sprintf(send_buffer, "450 Requested file action not taken. \r\n");
					send(connSock, send_buffer, sizeof(send_buffer), 0);
				}
				else {
					sprintf(send_buffer, "150 Transfering... \r\n");
					bytes = send(connSock, send_buffer, strlen(send_buffer), 0);

					//handle file
					FILE *fp = fopen(command, "r");
					char ch;
					FILE *new_file = fopen("LocalHost/GroupBy.txt", "w");
					while ((ch = fgetc(fp)) != EOF) {
						//printf("%c", ch); ->Log
						fputc(ch, new_file);
					}
					fclose(new_file);
					fclose(fp);

					sprintf(send_buffer, "226 File transfer completed... \r\n");
					bytes = send(connSock, send_buffer, strlen(send_buffer), 0);
				}
			}
			else {
				sprintf(send_buffer, "231 User logged out; service terminated\r\n");
				bytes = send(connSock, send_buffer, strlen(send_buffer), 0);
			}
		}

		//LIST OR NLST: display file in remote host (server)
		if (strncmp(receive_buffer, "LIST: ", 6) == 0|| strncmp(receive_buffer, "NLST: ", 6) == 0)
		{
			if (checkUserLogin) {
				system("cd RemoteHost&&dir > allFile.txt");
				FILE *fin = fopen("RemoteHost/allFile.txt", "r");//open allFile.txt file
																 //(send_buffer, 0, 1024);
				sprintf(send_buffer, "150 Transfering... \r\n");//send_buffer will pointer to string in sprintf
				bytes = send(connSock, send_buffer, strlen(send_buffer), 0);
				char temp_buffer[1024];
				while (!feof(fin))
				{
					fgets(temp_buffer, 1023, fin);//read content of file allFile.txt and save in temp_buffer
					sprintf(send_buffer, "%s", temp_buffer);//send_buffer will pointer to string in temp_buffer
					send(connSock, send_buffer, strlen(send_buffer), 0);
				}
				fclose(fin);
				sprintf(send_buffer, "226 File transfer completed... \r\n");
				bytes = send(connSock, send_buffer, strlen(send_buffer), 0);
			}
			else {
				sprintf(send_buffer, "231 User logged out; service terminated\r\n");
				bytes = send(connSock, send_buffer, strlen(send_buffer), 0);
			}

			if (bytes < 0) break;
		}

		//QUIT: Disconnect
		if (strncmp(receive_buffer, "QUIT: ", 6) == 0)
		{
			sprintf(send_buffer, "221 Connection closed by the FTP client\r\n");
			bytes = send(connSock, send_buffer, sizeof(send_buffer), 0);
			if (bytes < 0)break;
			closesocket(connSock);
			printf("Disconnect from Id: %d\n", GetCurrentThreadId());
		}
	}
	
}