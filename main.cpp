#include<iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string>
#include <unistd.h>
#include <ifaddrs.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace std;
#define STDIN 0
#define PORT "10000"
#define MAXDATASIZE 100
#define MINDATASIZE 100

int serverSockFd, fdmaxClient; 
int peerConnectionsCount;
int trackList;

int fileLength = 0;
int fileLengthPut = 0;
string token[4]={};
int fileSizeC2= 0;
int numOfBytesRead = 0;
int numOfBytesReadPut = 0;
int remainingBytes = 0;
int remainingBytesPut = 0;
int position=0;
int positionPut = 0;
bool recvFile;
bool getIsSet = false;
bool isPutSet= false;
bool firstTime = false;
bool firstTimePut = false;
bool gotFileLength = false;
string fileNameGlobal;
string fileNameGlobalPut;
size_t readBytesC2;
fd_set master;
fd_set masterClientRead;
fd_set masterClientWrite;
char * listeningPort;
string listeningPortNum;
int trackListClient;
struct ServerIpList{
    int connectionId;
    int socketFd;
    string ipAddress;
    string portNumber;
    string hostName;
};
struct ClientPeerList{
    int connectionId;
    int socketFd;
    string ipAddress;
    string portNumber;
    string hostName;
};

ServerIpList serverList[20] ;
ServerIpList clientList[20] ;
ClientPeerList clientPeerIpList[20] ;
void serverListDisplay()
{
    cout << "id:     Host Name     IP Address         PortNo." << endl;
    for (int list=0; list <trackList; list++)
    {
        cout << serverList[list].connectionId << "       ";
        cout << serverList[list].hostName<< "           ";
        cout << serverList[list].ipAddress<< "            ";
        cout << serverList[list].portNumber<< endl;
    }
}
void quitServer()
{
    for (int list=0; list <trackList; list++)
    {
        FD_CLR(serverList[list].socketFd, &master);
        close(serverList[list].socketFd);

        for (int i = list; i <trackList; i++)
            serverList[i] = serverList[i+1];
        memset(&serverList[list], 0, sizeof(serverList));
        peerConnectionsCount--;
    }
    exit(1);
}
string getHostName(string ipAddress)
{
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;

    if (inet_pton(AF_INET, ipAddress.c_str(), &sa.sin_addr) <= 0)
        cout << "inet_pton error";

    char hostname[1000];

    int x;
    if ((x=getnameinfo((struct sockaddr*)&sa, sizeof sa,
                    hostname, sizeof hostname, NULL, NULL, NI_NAMEREQD)) != 0) {
        cout <<"getnameinfo error";
        cout << gai_strerror(x) << endl;
    }
    return hostname;

}
void help()
{
    cout << "Below are the available command options and their usage:" <<endl;
    cout << "1. HELP     - Displays information about availble command options." <<endl;
    cout << "2. CREATOR  - Displays the students full name, UBIT name and UB email address." <<endl;
    cout << "3. DISPLAY  - Displays the IP Address of this process and port on which this process is listening for incoming connections." <<endl;
    cout << "4. REGISTER <server IP> <port number> - This command is used to register with the server. The arguments to the command are server IP Address and port number." <<endl;
    cout << "5. CONNECT <destination> <port number> - This command is used to establish a connection between the current client and any other registered clients." <<endl;
    cout << "6. LIST - This command displays the numbered list of all the connections this process is part of." <<endl;
    cout << "7. TERMINATE <connection id> - This command will terminate the connection listed under the connection id column of LIST command output." <<endl;
    cout << "8. QUIT - This command closes all connections and terminates this process." <<endl;
    cout << "9. GET <connection id> <filename> - This command is used to download a file from one host specified which is mapped to connection id(from LIST output) argument." <<endl;
    cout << "10. PUT <connection id> <filename> - This command is used to put the file specified in the filename argument to the host on the connection that has connectio id specified in the argument " <<endl;
}
void creator()
{
    cout << "Full Name : Anirudh Yellapragada" << endl;
    cout << "UBIT Name : ayellapr" << endl;
    cout << "UB email  : ayellapr@buffalo.edu" << endl;
}
void display(string port)
{
    char arr[32];
    gethostname(arr, 32);
    cout <<  "The local IP Address of this machine is :" << arr <<endl;
    cout <<  "The Listening port of this machine is :" << port <<endl;
}
void serverProgram(const char* portNum)
{
    fd_set read_fds;

    int fdmax;
    int sockFd, new_fd;
    struct addrinfo hints, *servinfo;
    struct sockaddr_storage their_addr;
    int rv, numBytes,i,j;
    int bufsize = 1024;
    char buffer[bufsize];

    char buf[MAXDATASIZE];
    socklen_t sin_size;
    char * bufin;
    bool isExit = false;
    string input;
    string options[] = {"help", "creator", "display", "quit"};
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    int yes = 1;
    string buffpl;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int ite;
    int optionGiven;
    rv = getaddrinfo(NULL, portNum, &hints, &servinfo);
    if(rv!=0)
        cout<< "error getting info"<<endl;

    sockFd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(sockFd == -1)
        cout<<"error creating socket" << endl;

    setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if(bind(sockFd,servinfo->ai_addr, servinfo->ai_addrlen ) == -1)
    {
        close(sockFd);
        cout<<"error binding" << endl;
    }
    freeaddrinfo(servinfo); //
    if(listen(sockFd, 10) == -1)
        cout << "errore listening" << endl;
    FD_SET(STDIN, &master);
    FD_SET(sockFd, &master);
    fdmax = sockFd;

    for(;;)
    {
        read_fds = master;
        if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
        {
            cout << "Error : Doing select" << endl;
        }
        for(i=0; i<=fdmax; i++)
        {
            if(FD_ISSET(i, &read_fds))
            {
                if( i != sockFd && i !=0 ) 
                {
                    if(recv(i, buf, sizeof(buf), 0) <= 0)
                    {
                        cout << "Socket closed from the other end "<<i<<endl;
                        for (int list=0; list <trackList; list++)
                        {
                            int i2=0;
                            if(serverList[list].socketFd == i)
                            {
                                FD_CLR(i, &master);
                                close(i);

                                for (i2 = list; i2 <trackList; i2++)
                                {
                                    serverList[i2] = serverList[i2+1];
                                    serverList[i2].connectionId = i2+1;
                                }
                                memset(&serverList[i2], 0, sizeof(serverList));
                                trackList--;
                            }
                        }
                    }
                    else
                    {
                        buffpl.clear();
                        listeningPortNum = buf;
                        serverList[trackList].portNumber = listeningPortNum;
                        for(int db = 0; db<=trackList; db++){
                            cout << "New connection in server IP List is :"<< serverList[db].connectionId<<"   "<<serverList[db].ipAddress<<"   "<<serverList[db].portNumber<<"   "<<serverList[db].hostName<< endl;}
                        trackList++;
                        for(int db = 0; db<trackList; db++)
                        {

                            buffpl += serverList[db].hostName +" "+serverList[db].ipAddress+" " + serverList[db].portNumber+" "; 
                        }
                        for( j=1; j<=fdmax; j++)
                        {
                            if(FD_ISSET(j, &master))
                            {
                                if(j!= sockFd )
                                {
                                    //cout << "bufpl" << buffpl << endl;
                                    const char * buffplo = buffpl.c_str();
                                    if(send(j, buffplo, strlen(buffplo), 0 )==-1) 
                                    {
                                        cout << "error in sending brother " << endl;
                                    }
                                }
                            }
                        }
                    }
                }
                else if ( i == sockFd )
                {
                    sin_size = sizeof(their_addr);
                    new_fd = accept(sockFd, (struct sockaddr *)&their_addr, &sin_size);
                    if(new_fd == -1 )
                        cout << "error in accepting" << endl;
                    if(new_fd >0) 
                    {
                        struct sockaddr_in sa;
                        char str[INET_ADDRSTRLEN];


                        // now get it back and print it
                        inet_ntop(AF_INET, &(((struct sockaddr_in *)&their_addr)->sin_addr), str, INET_ADDRSTRLEN);

                        serverList[trackList].connectionId = trackList+1;
                        serverList[trackList].ipAddress = str;
                        serverList[trackList].hostName = getHostName(serverList[trackList].ipAddress);
                        serverList[trackList].socketFd = new_fd;
                    }

                    FD_SET(new_fd, &master);
                    if(new_fd > fdmax)
                        fdmax = new_fd;
                    cout << "Connected to the socket : " << new_fd << endl; 
                }
                else if(i == 0)
                {
                    getline(cin,input);

                    for(ite =0; ite <=3 ; ite++)
                    {
                        if(input.find(options[ite]) == 0)
                        {
                            optionGiven = ite;
                        }

                    }
                    string delimiter = " ";

                    size_t pos = 0;
                    string token[4]={};
                    int pobey = 1;
                    while((pos = input.find(delimiter)) != string::npos)
                    {
                        token[pobey] = input.substr(0,pos);
                        input.erase(0,pos+delimiter.length());
                        token[pobey+1] = input.substr(0,100);
                        pobey++;
                    }
                    switch(optionGiven)
                    {

                        case 0 : help();
                                 break;
                        case 1 : creator();
                                 break;
                        case 2 : display(portNum);
                                 break;
                        case 3 : quitServer();
                                 break;
                    }
                    input.clear();
                }
            }
        }
    }
}
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void connectBro(string ipAddress, string portNum)
{
    int bufsize = 1024;
    bool alreadyConnected;
    char buffer[bufsize];
    char *buf;
    int rv2;
    int i,j,connec;
    int yes=1;
    int sockFdOne;
    struct addrinfo addressInfo, *servInfo;
    memset(&addressInfo, 0, sizeof(addressInfo));
    addressInfo .ai_family = AF_INET;
    addressInfo.ai_socktype = SOCK_STREAM;
    bool peerExists = false;
    for(int x= 0; x <trackListClient ; x++)
    {
        if((ipAddress.compare(clientList[x].ipAddress) == 0) )
        {
            peerExists = true;
        }
    }
    for (int list=0; list <peerConnectionsCount; list++)
    {
        if(((ipAddress.compare(clientPeerIpList[list].ipAddress) == 0) ||  (ipAddress.compare(clientPeerIpList[list].hostName) == 0) )&& portNum.compare(clientPeerIpList[list].portNumber ) ==0)
        {
            alreadyConnected = true;
        }
    }
    if(alreadyConnected)
        cout << "You are already connected to the destinaton mentioned" << endl;
    if(!peerExists)
        cout << "The destination mentioned is not in the Server-IP-List" << endl;

    if(peerExists && !alreadyConnected)
    {
        char *y = new char[ipAddress.length() + 1]; // or
        char *z = new char[portNum.length() + 1]; // or
        std::strcpy(y, ipAddress.c_str());
        std::strcpy(z, portNum.c_str());

        rv2 = getaddrinfo(y, z, &addressInfo, &servInfo);
        sockFdOne = socket(servInfo->ai_family, servInfo->ai_socktype, servInfo->ai_protocol);
        if(sockFdOne < 0)
        {
            cout << "ERROR creating a socket" << endl;
        }

        if (setsockopt(sockFdOne, SOL_SOCKET, SO_REUSEADDR, &yes,
                    sizeof(int)) == -1) {
            cout << "setsockopt error" << endl;
            exit(1);
        }

        connec = connect(sockFdOne, servInfo->ai_addr, servInfo->ai_addrlen);
        if(connec == -1){
            cout  <<"error connecting"<<endl;
            close(sockFdOne);
        }
        else
        {
cout << "You are connected now to the peer" << endl;
            clientPeerIpList[peerConnectionsCount].connectionId = peerConnectionsCount+1;
            clientPeerIpList[peerConnectionsCount].hostName = getHostName(ipAddress);
            clientPeerIpList[peerConnectionsCount].ipAddress = ipAddress;
            clientPeerIpList[peerConnectionsCount].portNumber = portNum;
            clientPeerIpList[peerConnectionsCount].socketFd = sockFdOne;

            peerConnectionsCount++;
        }

        if(send(sockFdOne, listeningPort, sizeof(listeningPort) ,0)==-1)

        FD_SET(sockFdOne, &masterClientRead);
        if(fdmaxClient < sockFdOne)
            fdmaxClient = sockFdOne;

    }

}
void get(string connectionId, string fileName)
{
    bool sendRequest = false;
    int indexTwo;
    int connecId = atoi(connectionId.c_str());
    for (int list=0; list <peerConnectionsCount; list++)
    {
        if(clientPeerIpList[list].connectionId == connecId)
        {
            if(clientPeerIpList[list].socketFd == serverSockFd)
                cout << "Error :  Cannot download a file from server" << endl;
            else
            {
                cout << "Doing a send to request the file"<< endl;
                sendRequest = true; 
                indexTwo = list;
            }
        }
    }
    if (sendRequest == true)
    {
        string buffer;
        string  file = fileName;
        buffer = "get " + file;

        const char * buffplo = buffer.c_str();
        if(send(clientPeerIpList[indexTwo].socketFd, buffplo, strlen(buffplo) ,0)==-1)
            cout << "Error in sending" << endl;
    }
}



void put(string connectionId, string fileName, int socketFd)
{
    bool sendRequest = false;
    int indexTwo;
    isPutSet = true;
    if( atoi(connectionId.c_str()) != 0)
    {

        for (int list=0; list <peerConnectionsCount; list++)
        {
            if(clientPeerIpList[list].connectionId ==atoi(connectionId.c_str()))
            {
                if(clientPeerIpList[list].socketFd == serverSockFd)
                    cout << "Error :  Cannot download a file from server" << endl;
                else
                {
                    sendRequest = true;
                    indexTwo = list;
                }
            }
        }
        if (sendRequest == true)
        {
            cout << "Sending data .." << endl;
            FD_SET(clientPeerIpList[indexTwo].socketFd, &masterClientWrite);
            string buffer;
            char memBlockPut[1024];
            streampos size;
            ifstream file;
            if(!firstTimePut)
                fileNameGlobalPut = fileName;
            file.open("okay2.txt", ios::in|ios::binary|ios::ate);
            fileLengthPut = file.tellg();
            memset(memBlockPut, 0, 1024);
            if (file.is_open())
            {
                file.seekg (positionPut, ios::beg);
                if(numOfBytesReadPut < fileLengthPut)
                {
                    file.seekg (positionPut, ios::beg);
                    if(firstTimePut)
                    {
                        if(remainingBytesPut<10)
                            file.read (memBlockPut, remainingBytesPut);
                        else
                            file.read (memBlockPut, 10);
                    }
                    positionPut =  file.tellg();
                    numOfBytesReadPut = numOfBytesReadPut + file.gcount();
                    remainingBytesPut = fileLengthPut-numOfBytesReadPut ;
                    string buffer2;
                    stringstream ss;
                    ss << fileLengthPut;
                    buffer2 = "fileLength " + ss.str();
                    const char * buffplo = buffer2.c_str();
                    if(firstTimePut)
                    {
                        if(remainingBytesPut<10 && remainingBytesPut>0)

                        {
                            if(send(clientPeerIpList[indexTwo].socketFd, memBlockPut, remainingBytesPut,0)==-1)
                                cout << "Error in sending" << endl;
                        }
                        else
                        {
                            if(send(clientPeerIpList[indexTwo].socketFd, memBlockPut, 10,0)==-1)
                                cout << "Error in sending" << endl;
                        }
                    }

                    if(!firstTimePut)
                    {
                        if(send(clientPeerIpList[indexTwo].socketFd, buffplo, strlen(buffplo) ,0)==-1)
                            cout << "Error in sending file" << endl;
                        firstTimePut = true;
                    }
                    file.close();
                }
                else
                {
                    FD_CLR(clientPeerIpList[indexTwo].socketFd, &masterClientWrite);
                    file.close();
                    numOfBytesReadPut = 0;
                    positionPut = 0;
                    fileLengthPut = 0;
                    fileNameGlobalPut.clear();
                    isPutSet = false;
                    firstTimePut = false;
                    cout << "File " << fileName << " has been sent" << endl;
                }
            }

        }
    }
    else
    {
        FD_SET(socketFd, &masterClientWrite);
        string toBeSentData;
        char memBlock[1024];
        streampos size;
        ifstream file;
        if(!firstTime)
            fileNameGlobal = fileName;
        char *y = new char[fileName.length() + 1]; // or
        std::strcpy(y, fileName.c_str());
        char ento[10] ;
        std::strcpy(ento, fileName.c_str());
        file.open(ento, ios::in|ios::binary|ios::ate);
        file.seekg (0, ios::end);
        fileLength = file.tellg();
        cout << "fileLength" << fileLength << endl;
        memset(memBlock, 0, 1024);
        if (file.is_open())
        {
            if(numOfBytesRead < fileLength)
            {
                file.seekg (position, ios::beg);
                if(firstTime){
                    if(remainingBytes<10)
                        file.read (memBlock, remainingBytes);
                    else
                        file.read (memBlock, 10);
                }
                position =  file.tellg();
                numOfBytesRead = numOfBytesRead + file.gcount();
                remainingBytes = fileLength-numOfBytesRead ;
                string buffer2;
                stringstream ss;
                ss << fileLength;
                buffer2 = "fileLength " + ss.str();
                const char * buffplo = buffer2.c_str();
                if(firstTime)
                {
                    if(send(socketFd, memBlock, 10 ,0)==-1)
                        cout << "Error in sending" << endl;
                }

                if(!firstTime)
                {
                    if(send(socketFd, buffplo, strlen(buffplo) ,0)==-1)
                        cout << "Error in sending file" << endl;
                    firstTime = true;
                }
                file.close();
            }
            else
            {
                FD_CLR(socketFd, &masterClientWrite);
                file.close();
                numOfBytesRead = 0;
                getIsSet = false;
                position = 0;
                fileLength = 0;
                fileNameGlobal.clear();
                firstTime = false;
                cout << "File " << fileName << " has been sent" << endl;
            }
        }
    }
}

void registers(string ipAddress, string portNum)
{
    string delimiter = ".";
    bool notAHostName;
    bool everythingIsFine;

    size_t pos5 = 0;
    string token5[4]={};
    int pobey5 = 1;
    int bufsize = 1024;
    char buffer[bufsize];

    char *buf;
    int rv1;

    int i,j;
    int new_fd, rv, connec, numBytes;
    struct addrinfo addressInfo, *servInfo;
    memset(&addressInfo, 0, sizeof(addressInfo));
    addressInfo .ai_family = AF_INET;
    addressInfo.ai_socktype = SOCK_STREAM;
    char *y = new char[ipAddress.length() + 1]; // or
    char *z = new char[portNum.length() + 1]; // or
    std::strcpy(y, ipAddress.c_str());
    std::strcpy(z, portNum.c_str());
    int yes =1;

    rv = getaddrinfo(y, z, &addressInfo, &servInfo);
    serverSockFd = socket(servInfo->ai_family, servInfo->ai_socktype, servInfo->ai_protocol);
    if(serverSockFd < 0)
    {
        cout << "ERROR creating a socket" << endl;
    }

    if (setsockopt(serverSockFd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
        cout << "setsockopt error" << endl;
        exit(1);
    }

    connec = connect(serverSockFd, servInfo->ai_addr, servInfo->ai_addrlen);
    if(connec == -1){
        cout  <<"error connecting"<<endl;
        close(serverSockFd);

    }
    else
    {
        clientPeerIpList[peerConnectionsCount].connectionId = peerConnectionsCount+1;
        if(notAHostName) 
            clientPeerIpList[peerConnectionsCount].hostName = getHostName(ipAddress);
        else
            clientPeerIpList[peerConnectionsCount].hostName = ipAddress;

        clientPeerIpList[peerConnectionsCount].ipAddress = ipAddress;
        clientPeerIpList[peerConnectionsCount].portNumber = portNum;
        clientPeerIpList[peerConnectionsCount].socketFd = serverSockFd;
        peerConnectionsCount++;
    }
    if(send(serverSockFd, listeningPort, sizeof(listeningPort) ,0)==-1)

        cout << "error in sending  " << endl;

    FD_SET(serverSockFd, &masterClientRead);
    if(fdmaxClient < serverSockFd)
        fdmaxClient = serverSockFd;

}

void listDisplay()
{
    cout << "peerConnectionsCount: " << peerConnectionsCount << endl;
    if(peerConnectionsCount !=0)
    {
        cout << "id:     Host Name     IP Address         PortNo." << endl;
    }
    else
    {
        cout << "There are no connections. Use HELP command." << endl;
    }
    for (int list=0; list <peerConnectionsCount; list++)
    {
        cout << clientPeerIpList[list].connectionId << "       ";
        cout << clientPeerIpList[list].hostName<< "           ";
        cout << clientPeerIpList[list].ipAddress<< "            ";
        cout << clientPeerIpList[list].portNumber<< endl;
    }
}
void terminate(string connectionId)
{
    int conn = atoi(connectionId.c_str());
    for (int list=0; list <peerConnectionsCount; list++)
    {
        int i2=0;
        if(clientPeerIpList[list].connectionId == conn)
        {
            FD_CLR(clientPeerIpList[list].socketFd, &masterClientRead);
            close(clientPeerIpList[list].socketFd);
            cout << "closing socket :" << clientPeerIpList[list].socketFd <<endl;

            for ( i2 = list; i2 <peerConnectionsCount  ; i2++){
                clientPeerIpList[i2] = clientPeerIpList[i2+1];
                clientPeerIpList[i2].connectionId = i2+1;
            }
            memset(&clientPeerIpList[i2], 0, sizeof(ClientPeerList));
            peerConnectionsCount--;
            cout << "Number of peer connections count is " <<peerConnectionsCount << endl;
        }
    }
}
void quit()
{
    for (int list=0; list <peerConnectionsCount; list++)
    {
        FD_CLR(clientPeerIpList[list].socketFd, &masterClientRead);
        close(clientPeerIpList[list].socketFd);

        for (int i = list; i <peerConnectionsCount  ; i++)
            clientPeerIpList[i] = clientPeerIpList[i+1];
        memset(&clientPeerIpList[list], 0, sizeof(ClientPeerList));
        peerConnectionsCount--;
        cout << "peerConnectionsCount" <<peerConnectionsCount << endl;
    }
    exit(1);
}
void newClientProgram(char *edok)
{
    fd_set read_fds;
    fd_set write_fds;
    int sockFd, new_fd;
    struct addrinfo hints, *servinfo;
    struct sockaddr_storage their_addr;
    char buf[MAXDATASIZE];
    int rv, numBytes,i,j;
    int bufsize = 1024;
    char buffer[bufsize];
    string input;

    string options[] = {"help", "creator", "display", "register", "connect", "list", "terminate", "quit", "get","put"};
    int optionGiven = 0;
    int ite;

    socklen_t sin_size;
    bool isExit = false;

    FD_ZERO(&masterClientRead);
    FD_ZERO(&masterClientWrite);
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    rv = getaddrinfo(NULL, edok, &hints, &servinfo);
    if(rv!=0)
        cout<< "error getting info"<<endl;

    sockFd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(sockFd == -1)
        cout<<"error creating socket" << endl;
    //cout << "sockFd s" << sockFd <<endl;
    setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    //cout << "sockFd s" << sockFd <<endl;
    if(bind(sockFd,servinfo->ai_addr, servinfo->ai_addrlen ) == -1)
    {
        close(sockFd);
        cout<<"error binding" << endl;
    }
    freeaddrinfo(servinfo);
    sin_size = sizeof(their_addr);
    if(listen(sockFd, 10) == -1)
        cout << "errore listening" << endl;
    //cout << "No error" << endl;
    FD_SET(STDIN, &masterClientRead);
    FD_SET(sockFd, &masterClientRead);
    fdmaxClient = sockFd;

    for(;;)
    {
        int selec;
        read_fds = masterClientRead;
        write_fds = masterClientWrite;
        selec = select(fdmaxClient+1, &read_fds, &write_fds, NULL, NULL) ;
        if(selec == -1)
        {
            cout << "Error : doing select" << endl;
        }
        for(i=0; i<=fdmaxClient; i++)
        {
            //    cout << "doing select for i " <<i<< endl;
            if(FD_ISSET(i, &read_fds))
            {
                if ( i == sockFd )
                {
                    if(peerConnectionsCount <= 3)
                    {

                        new_fd = accept(sockFd, (struct sockaddr *)&their_addr, &sin_size);

                        cout << "Connected to the socket : " << new_fd << endl;
                        struct sockaddr_in sa;
                        char str[INET_ADDRSTRLEN];

                        inet_ntop(AF_INET, &(((struct sockaddr_in *)&their_addr)->sin_addr), str, INET_ADDRSTRLEN);
                        clientPeerIpList[peerConnectionsCount].connectionId = peerConnectionsCount+1;
                        clientPeerIpList[peerConnectionsCount].socketFd = new_fd;
                        clientPeerIpList[peerConnectionsCount].hostName = getHostName(str);
                        clientPeerIpList[peerConnectionsCount].ipAddress = str;
                        FD_SET(new_fd, &masterClientRead);
                        if(new_fd > fdmaxClient)
                            fdmaxClient= new_fd;
                        cout << "Connected to the socket : " << new_fd << endl;
                        if (peerConnectionsCount == 3)
                        {
                            FD_CLR(i, &masterClientRead);
                            close(i);
                        }
                    }
                }
                else if(i == 0)
                {
                    getline(cin,input);
                    transform(input.begin(), input.end(), input.begin(), ::tolower);

                    for(ite =0; ite <=9 ; ite++)
                    {
                        if(input.find(options[ite]) == 0)
                        {
                            optionGiven = ite;
                        }
                    }
                    string delimiter = " ";

                    size_t pos = 0;
                    int pobey = 1;
                    while((pos = input.find(delimiter)) != string::npos)
                    {
                        token[pobey] = input.substr(0,pos);
                        input.erase(0,pos+delimiter.length());
                        token[pobey+1] = input.substr(0,100);
                        pobey++;
                    }

                    switch(optionGiven)
                    {
                        case 0 : help();
                                 break;
                        case 1 : creator();
                                 break;
                        case 2 : display(edok);
                                 break;
                        case 3 : registers(token[2],token[3]);
                                 break;
                        case 4 : connectBro(token[2], token[3]);
                                 break;
                        case 5 : listDisplay();
                                 break;
                        case 6 : terminate(token[2]); 
                                 break;
                        case 7 : quit();
                                 break;
                        case 8 : get(token[2], token[3]);
                                 break;
                        case 9 : put(token[2], token[3], 0);
                                 break;
                    }
                    input.clear();
                }
                else
                {
                    string delimiter = " ";
                    size_t pos2 = 0;
                    string token2[20]={};
                    int pobey2 = 1;
                    memset(buf, 0, MAXDATASIZE);
                    if(recv(i, buf, sizeof(buf), 0) <= 0)
                    {
                        cout << "Error: in receiving from i "<<i<<endl;
                        for (int list=0; list <peerConnectionsCount; list++)
                        {
                            int i2=0;
                            if(clientPeerIpList[list].socketFd == i)
                            {
                                FD_CLR(i, &masterClientRead);
                                close(i);

                                for (i2 = list; i2 <peerConnectionsCount  ; i2++)
                                {
                                    clientPeerIpList[i2] = clientPeerIpList[i2+1];
                                    clientPeerIpList[i2].connectionId = i2+1;
                                }
                                memset(&clientPeerIpList[i2], 0, sizeof(ClientPeerList));
                                peerConnectionsCount--;
                                cout << "peerConnectionsCount error in reeiving" <<peerConnectionsCount << endl;
                            }
                        }
                    }
                    else
                    {
                        if( i == serverSockFd )
                        {
                            trackListClient = 0;
                            string numIter = buf;
                            //cout << "numIter is " << numIter;
                            cout << "Server IP List is" << endl ;
                            while((pos2 = numIter.find(delimiter)) != string::npos)
                            {
                                token2[pobey2] = numIter.substr(0,pos2);
                                numIter.erase(0,pos2+delimiter.length());
                                token2[pobey2+1] = numIter.substr(0,100);
                                pobey2++;
                            }

                            for(int ok =1; ok < pobey2 || trackListClient==0; ok=ok+3)
                            {
                                {
                                    clientList[trackListClient].connectionId = trackListClient+1;
                                    clientList[trackListClient].portNumber = token2[ok+2];
                                    clientList[trackListClient].ipAddress = token2[ok+1];
                                    clientList[trackListClient].hostName = token2[ok];
                                    trackListClient++;
                                }
                            }
                            for(int pk =0; pk<((pobey2-1)/3);pk++)
                            {
                                //cout << "trackListClient is " << trackListClient << endl;
                                cout <<  clientList[pk].connectionId  << "   ";
                                cout <<  clientList[pk].hostName<< "   ";
                                cout <<  clientList[pk].ipAddress<< "   ";
                                cout <<  clientList[pk].portNumber<< "   "<<endl;
                            }
                        }
                        else
                        {
                            string delimiter4 = " ";
                            size_t pos4 = 0;
                            string token4[4]={};
                            int pobey4 = 1;
                            string input4 =  buf;
                            while((pos4 = input4.find(delimiter)) != string::npos)
                            {
                                token4[pobey4] = input4.substr(0,pos4);
                                input4.erase(0,pos4+delimiter.length());
                                token4[pobey4+1] = input4.substr(0,100);
                                pobey4++;
                            }
                            cout << "token4[1]" << token4[1] << endl;
                            cout << "token4[2]" << token4[2] << endl;

                            cout << "Success in connecting to portnumber : " << buf <<endl;

                            if(token4[1].compare("get") == 0)
                            {
                                cout << "Found get" <<endl;
                                cout << "Calling put function : cause is Get" << endl;
                                put("0", token4[2], i);
                                getIsSet = true;
                            }
                            else if((token4[1].compare("fileLength") == 0) || recvFile)
                            {
                                cout << "Found fileLength" <<endl;
                                if(!recvFile)
                                    fileSizeC2 = atoi(token4[2].c_str());
                                cout << "fileSizeC2" <<fileSizeC2<<endl;
                                recvFile = true;
                                fstream myfile;
                                if( readBytesC2 < fileSizeC2 )
                                {
                                    cout <<" readBytesC2 != fileSizeC2" <<readBytesC2 <<endl;
                                    if(gotFileLength)
                                    {
                                        cout << "opening example.txt" <<endl;
                                        myfile.open ("example.txt", ios::app);
                                        myfile << buf;
                                        readBytesC2 = readBytesC2 + strlen(buf);
                                        cout << "readBytesC2" <<readBytesC2<<endl;
                                        myfile.close();
                                    }

                                    gotFileLength = true;
                                }
                                else
                                {
                                    cout <<" readBytesC2 == fileSizeC2" << readBytesC2<<endl;
                                    recvFile = false;
                                    readBytesC2 = 0;
                                    myfile.close();
                                }
                            }
                            else
                            {
                                string listeningPortNum =  buf;
                                clientPeerIpList[peerConnectionsCount].portNumber = listeningPortNum; 
                                peerConnectionsCount++;
                            }
                        }
                    }

                }
            }
            if(FD_ISSET(i, &write_fds))
            {
                if(getIsSet)
                    put("0", " ", i);
                else
                {
                    cout << "entering here " << endl;
                    put(token[2], token[3], 0);
                    isPutSet= false;
                }

            }
        }

    }
}
int main(int argc, char* argv[])
{
    cout << "HELP" << endl;
    cout << "CREATOR" << endl;
    cout << "DISPLAY" << endl;
    cout << "REGISTER" << endl;
    cout << "CONNECT" << endl;
    cout << "LIST" << endl;
    cout << "TERMINATE" << endl;
    cout << "QUIT" << endl;
    cout << "GET" << endl;
    cout << "PUT" << endl;
    unsigned short int portNum;
    char* port = argv[2];
    portNum = (unsigned short) strtoul(port, NULL ,0);

    listeningPort = argv[2];

    if (*argv[1] == 's')
    {
        serverProgram(argv[2]);
    }
    else if(*argv[1] == 'c') 
    {
        newClientProgram(argv[2]);
    }
    else
    {
        cout << "Wrong input entered by user" << endl;
    }
    return 0;   
}
