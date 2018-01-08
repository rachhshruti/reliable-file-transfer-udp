#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstdlib>
#include <unistd.h>
#include <string.h>
#include<fstream>
#include <sstream>
#include <signal.h>
using namespace std;
/**
 * This class is an implementation of the reliable UDP Client which requests the 
 * UDP Server for a file. It sends the cumulative acknowledgement for correctly
 * received packets in the window.
 * 
 * @author Shruti Rachh
 */
bool timeout;
class UDPClient
{
	private:
	int clientSocket;
   	unsigned int len;
   	struct sockaddr_in serverAddress,clientAddress;
  	struct hostent *host;
	struct reliableUDPData
	{
		uint32_t sequenceNumber;
		uint32_t ackNumber;
		bool ackFlag;
		char data[1450];
	}udpSegment;	
	public:
	void displayError(const char *errorMsg);
	void createSocket();
	void getServerInfo(char* hostname);
	void setServerAddress(int portNo);
	void createRequest(string filename);
	int sendRequest();
	void readResponse(int windowSize,char* outputFile);
	void sendAck(reliableUDPData segment);
	void writeToFile(reliableUDPData* response,int receiveBufferInd,char* filename);	
};

/**
 * This method is used to display the error message.
 * @param
 * 	errorMsg: The error message that is to be displayed.
*/
void UDPClient::displayError(const char *errorMsg)
{
	cerr<<"Error: "<<errorMsg<<endl;
	exit(1);
}

/**
 * This method is used to create the client socket and sets the 
 * clientSocket class variable.
 */
void UDPClient::createSocket()
{
	clientSocket=socket(AF_INET,SOCK_DGRAM,0);
	if (clientSocket < 0)
  	{
		displayError("The server socket could not be opened!");
	}
}

/**
 * This method is used to get the IP address of the server
 * based on the hostname provided by the client.
 * @param
 *	hostname: the server hostname
 */
void UDPClient::getServerInfo(char* hostname)
{
	host=gethostbyname(hostname);
	if(host==NULL)
	{
		displayError("There is no such host!");
	}
}

/**
 * This method is used to set the server address.
 * @param
 *	portNo: the port number on which server is listening.
 */
void UDPClient::setServerAddress(int portNo)
{
	bzero((char *) &serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	bcopy((char *)host->h_addr,(char *)&serverAddress.sin_addr.s_addr,host->h_length);
	serverAddress.sin_port = htons(portNo);
}

/**
 * This method is used to create the request.
 * @param
 *	filename: the name of the file that is to be requested.
 * @param
 *	receiveWindow: the number of bytes it can receive at a time without sending back acknowledgement.
 * @return
 *	the request string to be sent
 */
void UDPClient::createRequest(string filename)
{
	udpSegment.sequenceNumber=0;
	udpSegment.ackNumber=0;
	udpSegment.ackFlag=0;
	string fileStr=filename;
	strcpy(udpSegment.data,fileStr.c_str());
}

/**
 * This method is used to send the request to the server.
 * @param
 *	request: the request to be sent.
 * @return
 * 	the number of characters written.
 */
int UDPClient::sendRequest()
{
	len=sizeof(struct sockaddr_in);
	int no=sendto(clientSocket,&udpSegment,sizeof(udpSegment),0,(const struct sockaddr *)&serverAddress,len);	
	if (no<0)
   	{
		displayError("There is problem while sending request!");
	}
	return no;
}

/**
 * This method receives the file segments from the server and sends the cumulative acknowledgement
 * for correctly received segments and duplicate acknowledgement for lost packet.
 * @param windowSize
 *	the window size in bytes that can be received by the client.
 * @param outputFile
 *	the requested file name.
 * 
 */
void UDPClient::readResponse(int windowSize,char* outputFile)
{
	reliableUDPData *receiveBuffer=new reliableUDPData[3000];
	int nxtExpectedSeqNum=0;
	int receiveBufferInd=0;	
	int winCount=0;
	int noOfSegInWin=windowSize/sizeof(udpSegment);
	int no=1;
	reliableUDPData segment;
	segment.ackFlag=0;
	struct timeval tv;
	fd_set fds;
	int val=1;
	while(segment.ackFlag==0)
	{
		no=1;
		bool outOfOrderFlag=false;
		while(winCount<noOfSegInWin && segment.ackFlag==0)
		{
			FD_ZERO(&fds);
			FD_CLR(clientSocket,&fds);
			FD_SET(clientSocket,&fds);
			tv.tv_sec = 0;
			tv.tv_usec =100000;
			val=select(clientSocket+1,&fds,NULL,NULL,&tv);
			if(val==0)
			{
				break;
			}
			if(val==-1)
			{
				displayError("There is some problem in receiving the segment!");
			}
			if(FD_ISSET(clientSocket,&fds) && val==1)
			{
				no=recvfrom(clientSocket,&segment,sizeof(segment),0,(struct sockaddr *)&clientAddress, &len);
				int seqNo=segment.sequenceNumber;
				cout<<"Received Segment for sequence number "<<seqNo<<endl;					
				if(seqNo==nxtExpectedSeqNum && receiveBufferInd<1000)
				{
					receiveBuffer[receiveBufferInd]=segment;
					nxtExpectedSeqNum=segment.sequenceNumber+1;
					receiveBufferInd++;
					winCount++;
				}
				else
				{	
					outOfOrderFlag=true;
				}
			}
		}
		if(outOfOrderFlag)
		{
			reliableUDPData ack;
			if(receiveBufferInd==0)
			{
				for(int i=0;i<3;i++)
				{			
					ack.sequenceNumber=0;
					ack.ackNumber=0;
					ack.ackFlag=1;
					cout<<"Sending Acknowledgement Number "<<ack.ackNumber<<" for sequence number "<<ack.sequenceNumber<<endl;
					int no=sendto(clientSocket,&ack,sizeof(ack),0,(const struct sockaddr *)&serverAddress,len);	
					if (no<0)
   					{
						displayError("There is problem while sending acknowledgement!");
					}
				}	
				
			}
			else
			{
				for(int i=0;i<3;i++)
				{			
					ack.sequenceNumber=receiveBuffer[receiveBufferInd-1].sequenceNumber;
					ack.ackNumber=receiveBuffer[receiveBufferInd-1].sequenceNumber;					
					sendAck(ack);
				}
			}			
		}
		else
		{
			//sleep(1);
			sendAck(receiveBuffer[receiveBufferInd-1]);
		}
		winCount=0;		
	}
	writeToFile(receiveBuffer,receiveBufferInd,outputFile);
}

/**
 * This method sends back the acknowledgement to the server.
 * @param reliableUDPData
 * 	the segment to be acknowledged
 */
void UDPClient::sendAck(reliableUDPData segment)
{
	reliableUDPData acknowledge;
	acknowledge.ackNumber=segment.sequenceNumber+1;
	acknowledge.sequenceNumber=segment.sequenceNumber;
	acknowledge.ackFlag=1;
	cout<<"Sending Acknowledgement Number "<<acknowledge.ackNumber<<" for sequence number "<<segment.sequenceNumber<<endl;
	int no=sendto(clientSocket,&acknowledge,sizeof(acknowledge),0,(const struct sockaddr *)&serverAddress,len);	
	if (no<0)
   	{
		displayError("There is problem while sending acknowledgement!");
	}
}

/**
 * This method writes the received file content to a file.
 * @param response
 * 	an array containing the file segments
 * @param receiveBufferInd
 *	receive buffer index
 * @param filename 
 *	the name of the file to be written
 */
void UDPClient::writeToFile(reliableUDPData* response,int receiveBufferInd,char* filename)
{
	ofstream writeFile;
  	writeFile.open(filename);
	for(int i=0;i<receiveBufferInd-1;i++)
	{
		writeFile<<response[i].data;
	}
	cout<<"File Transfered Successfully "<<endl;
	writeFile.close();
}

int main(int noOfArguments,char *argumentList[])
{
	UDPClient client;
	/*
         * It checks if all the command-line arguments are provided.
         */	 
	if(noOfArguments<5)
	{
		client.displayError("Invalid arguments!");
	}  
	client.createSocket();
	int portNo=atoi(argumentList[2]);
	client.getServerInfo(argumentList[1]);
	client.setServerAddress(portNo);
	client.createRequest(argumentList[3]);	
	int size=client.sendRequest();
	client.readResponse(atoi(argumentList[4]),argumentList[3]);	
	return 0;
}
