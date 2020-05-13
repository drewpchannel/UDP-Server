#include <iostream>
#include <WS2tcpip.h>  //win sockets lib, this is 2.0 Beej: WSACleanup() when you’re all through with the sockets library

#pragma comment (lib, "ws2_32.lib")

using namespace std;  //saves us from using std::

string ClientIPsList[100];

void AddClient(string NewClientIP)
{
	//in future find array length dynamicly
	for (int i = 0; i < 100; i++)
	{
		//possible add in a check for double IPs
		if (ClientIPsList[i] == "")
		{
			cout << "Null Found, trying to add: " << ClientIPsList[i] << endl;
			ClientIPsList[i] = NewClientIP;
			cout << "IP added to list: " << ClientIPsList[i] << endl;
			break;
		}
		else
		{
			//needs to be checked if working as expected to check for duplicates
			if (ClientIPsList[i] == NewClientIP)
			{
				break;
			}
		}
	}
}

void SendToClient(string ClientIP, char buf[2048])
{
	WSADATA data;
	WORD version = MAKEWORD(2, 2);
	int wSOk = WSAStartup(version, &data);
	if (wSOk != 0) {
		fprintf(stderr, "WSAStartup failed.\n");
		exit(1);
	}

	sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(12773); //Send to Clients port

	inet_pton(AF_INET, ClientIP.c_str(), &server.sin_addr);
	SOCKET out = socket(AF_INET, SOCK_DGRAM, 0);
	cout << "trying to send: " << buf << endl;
	int sendOK = sendto(out, buf, strlen(buf), 0, (sockaddr*)&server, sizeof(server));

	if (sendOK == SOCKET_ERROR)
	{
		cout << "Send didn't work: " << WSAGetLastError;
	}

	closesocket(out);

	WSACleanup();
}

void GoThroughClients(char buf[2048])
{
	for (int i = 0; i < 100; i++)
	{
		if (ClientIPsList[i] == "")
		{
			break;
		}
		else
		{
			SendToClient(ClientIPsList[i], buf);
		}
	}
}

//This won't work with NATS yet, maybe bind username to IP or something
int main()
{
	//Startup winsock
	WSADATA data;
	WORD version = MAKEWORD(2, 2);
	int wSOk = WSAStartup(version, &data);
	if (wSOk != 0) {
		fprintf(stderr, "WSAStartup failed.\n");
		exit(1);
	} //needs the data behind WSADATA data; info found in docs for ws2_32

	//Bind socket to IP addy \ port
	SOCKET in = socket(AF_INET, SOCK_DGRAM, 0);

	sockaddr_in serverHint;
	serverHint.sin_addr.S_un.S_addr = ADDR_ANY; // Us any IP address available on the machine
	serverHint.sin_family = AF_INET; // Address format is IPv4
	serverHint.sin_port = htons(12774); // catch, Convert from little to big endian.  HOST PORT

	if (bind(in, (sockaddr*)&serverHint, sizeof(serverHint)) == SOCKET_ERROR) //sockaddress (points to struct? & is like obj.serverHint?)
	{
		cout << "can't bind socket " << WSAGetLastError;
	}

	sockaddr_in client;
	int clientLength = sizeof(client);
	ZeroMemory(&client, clientLength);  //replaces everything in memory as 0

	char buf[2048];

	//enter loop, wait for messages
	while (true) {
		ZeroMemory(buf, 2048);  //clears buffer when trying to receive again

		//receive from this socket, to this buffer, this length, flags, take client information and put it in &client
		int bytesIn = recvfrom(in, buf, 2048, 0, (sockaddr*)&client, &clientLength);
		if (bytesIn == SOCKET_ERROR) 
		{
			cout << "Error rec from client";
		}
		char ClientIP[256];
		ZeroMemory(ClientIP, 256);

		//convert number to address
		inet_ntop(AF_INET, &client.sin_addr, ClientIP, 256);  //INT family comes from the struct ws2
		//check if its a new client, intro message
		char CheckJoinMsg[] = "serverjoin";

		if (strcmp(buf, CheckJoinMsg) == 0)
		{
			cout << "server join found" << endl;
			AddClient(ClientIP);
		}

		GoThroughClients(buf);

		cout << "Messaged recieved from: " << ClientIP << " : " << buf << endl;
	}

	//close socket
	closesocket(in);

	//shutdown winsock
	WSACleanup();
}