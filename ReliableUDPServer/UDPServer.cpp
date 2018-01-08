#include <iostream>
#include <sys/types.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include<fstream>
#include<map>
#include <sstream>
#include <sys/time.h> 
#include<cmath>
#include<ctime>
using namespace std;
/**
 * This class is an implementation of the reliable UDP Server which 
 * transfers the file requested by the UDP Client.
 * It retransmits the lost packets or if the acknowledgement is not 
 * received before the server times out. It calculates timeout interval
 * based on the RTT values for adaptive retransmission.
 * It provides congestion control by adjusting the number of segments 
 * the sender can send based on the size of the congestion window.
 * 
 * @author Shruti Rachh
 */
int mss=1450;
class UDPServer
{
	private:
	int serverSocket;
   	socklen_t clientSockLen;
   	struct sockaddr_in serverAddress;
   	struct sockaddr_in clientAddress;
   	char buff[10000];
	int fileSize;	
	struct reliableUDPData
	{
		uint32_t sequenceNumber;
		uint32_t ackNumber;
		bool ackFlag;
		char data[1450];
	}segment;
	struct timeval sampleRTT,estimatedRTT,devRTT,timeoutInterval;
	public:
  	void displayError(const char *errorMsg);	
	void createSocket();
	void bindAddress(int portNumber);
	void setClientSockLength();
	int receiveRequest();
	char* getRequestedContent();
	int getFileSize(const char *filename);	
	void createSegments(char *fileContent,int windowSize);
	void slidingWindow(reliableUDPData *senderBuffer, int senderBufferLen,int windowSize);
	struct timeval calculateTimeout(struct timeval t1, struct timeval t2);	
	UDPServer::reliableUDPData setHeader(int seqNo, int ackNo, int flag, char *datagram);
	void sendSegment(reliableUDPData seg);	
	UDPServer::reliableUDPData receiveAck();	
};

/**
 * This method is used to display the error message.
 * @param
 * 	errorMsg: The error message that is to be displayed.
*/
void UDPServer::displayError(const char *errorMsg)
{
	cerr<<"Error: "<<errorMsg<<endl;
	exit(1);
}

/**
 * This method is used to create the server socket and sets the 
 * serverSocket class variable.
 */
void UDPServer::createSocket()
{
	serverSocket=socket(AF_INET, SOCK_DGRAM, 0);
   	if (serverSocket < 0) 
	{
		displayError("The server socket could not be opened!");
	}
}

/**
 * This method is used to bind the server socket to an address.
 * @param
 * 	portNumber: The port number on which the server will listen for incoming connections.
 */
void UDPServer::bindAddress(int portNumber)
{
	int leng=sizeof(serverAddress);
	bzero(&serverAddress,leng);
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(portNumber);     	
	serverAddress.sin_addr.s_addr = INADDR_ANY;
     	
	if (bind(serverSocket, (struct sockaddr *) &serverAddress,leng) < 0) 
  	{      
		displayError("There is some problem while binding the server socket to an address!");
	}
}

void UDPServer::setClientSockLength()
{
	clientSockLen = sizeof(struct sockaddr_in);
}

/**
 * This method receives the request sent by the client.
 * @return the number of bytes received.
 */
int UDPServer::receiveRequest()
{
    int noOfCharacters = recvfrom(serverSocket,&segment,sizeof(segment),0,(struct sockaddr *)&clientAddress,&clientSockLen);	       	
	if (noOfCharacters < 0) 
	{
		displayError("There is some problem in receiving the request!");
	}
	return noOfCharacters;
}

/**
 * Gets the size of the requested file.
 * @param filename
 * 	the filename whose size is to be calculated.
 * @return the file size
 */
int UDPServer::getFileSize(const char *filename)
{
    ifstream file;
    file.open(filename, ios_base::binary);
    file.seekg(0,ios_base::end);
    int size = file.tellg();
    file.close();
    return size;
}

/**
 * This method gets the requested file contents.
 * It sets the status code to either 200 or 404 depending 
 * on whether the file is present or not on the server's 
 * directory.
 * @param filename
 *	The name of the file requested.
 * @return 
 * 	the file content
 */
char* UDPServer::getRequestedContent()
{
	fileSize=getFileSize(segment.data);
	char *fileContent;
	fileContent=new char[fileSize];	
	ifstream readFile;
	readFile.open(segment.data);		
	if(readFile.is_open())
	{
		readFile.read(fileContent, fileSize);
	}
	else
	{
		displayError("File Not Found");
	}
	return fileContent; 	
}

/**
 * This method divides the file into segments based on the allowed segment size.
 * @param fileContent
 * 	The content of the file.
 * @param windowSize
 * 	the size of the window
 */
void UDPServer::createSegments(char *fileContent,int windowSize)
{
	int noOfSegments=fileSize/mss;
	char *seg=new char[mss];	
	int seqNo=0;
	int ackNo=segment.sequenceNumber+1;
	int ackFlag=0;
	int i,j,k;
	int senderBufferLen=noOfSegments+1;
	reliableUDPData *senderBuffer=new reliableUDPData[senderBufferLen];
	for(j=0;j<noOfSegments;j++)
	{
		for(i=j*mss, k=0;i<(j+1)*mss && k<mss;i++,k++)
		{
			seg[k]=fileContent[i];
		}
		senderBuffer[j]=setHeader(seqNo,ackNo,ackFlag,seg);
		seqNo++;
	}
	int rem=fileSize%mss;
	for( int s=0;s<rem;s++)
	{
		seg[s]=fileContent[++i];
	}
	senderBuffer[j]=setHeader(seqNo,ackNo,ackFlag,seg);
	slidingWindow(senderBuffer,senderBufferLen,windowSize);	
}

/**
 * This method implements the sliding window protocol
 * @param senderBuffer
 *	pointer to array containing the UDP data segments that need to be sent to the client
 * @param senderBufferLen
 *	the number of the segments to be sent
 * @param windowSize
 *	the window size in bytes that can be received by the client
 */
void UDPServer::slidingWindow(reliableUDPData *senderBuffer, int senderBufferLen,int windowSize)
{
	int firstUnAck=0,nxtSeqNo=0,dupAckCnt;
	reliableUDPData ack;
	cout<<"No of segments to be sent: "<<senderBufferLen<<endl;
	int cwnd=1;
	int ssthresh=64000;
	int segmentSize=sizeof(senderBuffer[0]);
	int noOfSegmentsInWin=windowSize/segmentSize;
	struct timeval t1, t2;
	cout<<"No of segments in window "<<noOfSegmentsInWin<<endl;
	int dropPercent=60;
	int noOfPacketsToDrop=(dropPercent*noOfSegmentsInWin)/100;
	cout<<"No of packets to drop "<<noOfPacketsToDrop<<endl;
	int *packetsToDrop=new int[noOfPacketsToDrop];
	estimatedRTT.tv_sec=0;
	estimatedRTT.tv_usec=0;
	devRTT.tv_sec=0;
	devRTT.tv_usec=0;
	timeoutInterval.tv_sec=2;
	timeoutInterval.tv_usec=0;
	fd_set fds;
	int val=1;
	while(nxtSeqNo<senderBufferLen)
	{
		
		if(firstUnAck==0 && nxtSeqNo==0)
		{
			srand(time(NULL));
			for(int i=0;i<noOfPacketsToDrop;i++)
			{
				
				packetsToDrop[i]=rand()%(noOfSegmentsInWin-1)+1;
				cout<<"Sequence numbers to drop "<<packetsToDrop[i]<<endl;
			}
		}
		int minimumSize=(cwnd<noOfSegmentsInWin)?cwnd:noOfSegmentsInWin;		
		gettimeofday(&t1, NULL);
		while(nxtSeqNo<firstUnAck+minimumSize && nxtSeqNo<senderBufferLen)
		{	
			if(nxtSeqNo==senderBufferLen-1)
			{
				senderBuffer[nxtSeqNo].ackFlag=1;
			}
			bool flag=true;
			for(int i=0;i<noOfPacketsToDrop;i++)
			{
				if(nxtSeqNo==packetsToDrop[i])
				{
					flag=false;
				}
			}
			if(flag)
			{
				
				sendSegment(senderBuffer[nxtSeqNo]);
			}	
			nxtSeqNo++;
		}
		dupAckCnt=0;
		FD_ZERO(&fds);
		FD_CLR(serverSocket,&fds);
		FD_SET(serverSocket,&fds);
		val=select(serverSocket+1,&fds,NULL,NULL,&timeoutInterval);
		if(val==0)
		{
			ssthresh=(cwnd*segmentSize)/2;			
			cwnd=1;
			timeoutInterval.tv_sec=2*timeoutInterval.tv_sec;
			timeoutInterval.tv_usec=2*timeoutInterval.tv_usec;
			continue;
		}
		if(val==-1)
		{
			displayError("There is some problem in receiving the segment!");
		}
		if(FD_ISSET(serverSocket,&fds) && val==1)
		{
			ack=receiveAck();
			gettimeofday(&t2, NULL);
			if(ack.ackNumber<nxtSeqNo)
			{
				dupAckCnt++;
				while(dupAckCnt<3)
				{
					ack=receiveAck();
					dupAckCnt++;
				}
				for(int i=0;i<noOfPacketsToDrop;i++)
				{
					if(ack.ackNumber==packetsToDrop[i])
					{
						packetsToDrop[i]=-1;
					}
				}
			}
			if(dupAckCnt<3 && minimumSize==cwnd)
			{
				if((cwnd*segmentSize)>=ssthresh)
				{
					cout<<"Congestion Avoidance "<<endl;
					cwnd=cwnd+1;
					cout<<"Next Sequence Number "<<ack.ackNumber<<endl;
				}
				else
				{
					cout<<"Slow Start "<<endl;
					cwnd=cwnd*2;
				}
				timeoutInterval=calculateTimeout(t1,t2);
			}
			firstUnAck=ack.ackNumber;
			nxtSeqNo=ack.ackNumber;			
		}
			
	}
	cout<<"File Sent Successfully "<<endl;
}

/**
 * This method calculates the timeout interval for adaptive retransmission.
 * @param t1
 *	the time when the segment is sent
 * @param t2
 *	the time when the acknowledgement is received
 * @return
 *	the timeout interval 
 */
struct timeval UDPServer::calculateTimeout(struct timeval t1, struct timeval t2)
{
	
	double alpha=0.125,beta=0.25;
	sampleRTT.tv_sec=t2.tv_sec-t1.tv_sec;
	sampleRTT.tv_usec=t2.tv_usec-t1.tv_usec;
       	estimatedRTT.tv_sec=((1-alpha)*estimatedRTT.tv_sec + (alpha*sampleRTT.tv_sec));
	estimatedRTT.tv_usec=((1-alpha)*estimatedRTT.tv_usec + (alpha*sampleRTT.tv_usec));
       	devRTT.tv_sec=((1-beta)*devRTT.tv_sec + beta*(abs(sampleRTT.tv_sec-estimatedRTT.tv_sec)));
	devRTT.tv_usec=((1-beta)*devRTT.tv_usec + beta*(abs(sampleRTT.tv_usec-estimatedRTT.tv_usec)));
       	timeoutInterval.tv_sec=estimatedRTT.tv_sec+4*devRTT.tv_sec;
	timeoutInterval.tv_usec=estimatedRTT.tv_usec+4*devRTT.tv_usec;
	return timeoutInterval;
}

/**
 * This method sets the UDP header and data.
 * @param seqNo
 *	the sequence number of the segment
 * @param ackNo
 *	the acknowledgement number of the segment
 * @param flag
 *	the acknowledgment flag set to true if the sender is sending an acknowledgment, false otherwise
 * @param datagram
 *	the data to be sent
 */
UDPServer::reliableUDPData UDPServer::setHeader(int seqNo, int ackNo, int flag, char *datagram)
{
	reliableUDPData udpData;
	udpData.sequenceNumber=seqNo;
	udpData.ackNumber=ackNo;
	udpData.ackFlag=flag;
	strcpy(udpData.data,datagram);
	return udpData;
}

/**
 * This method sends the UDP segment to the client.
 * @param seg
 *	the UDP segment containing the header and the data.
 */ 
void UDPServer::sendSegment(reliableUDPData seg)
{
	cout<<"Sending packet with sequence number: "<<seg.sequenceNumber<<endl;
	int no=sendto(serverSocket,&seg,sizeof(seg),0,(struct sockaddr *)&clientAddress,clientSockLen);
	if(no<0)
	{
		displayError("There is some problem in sending the segment!");
	}
}

/**
 * This method receives the acknowledgement from the client.
 * @return reliableUDPData
 *	the acknowledgement
 */ 
UDPServer::reliableUDPData UDPServer::receiveAck()
{
	reliableUDPData ack;
	int no=recvfrom(serverSocket,&ack,sizeof(ack),0,(struct sockaddr *)&clientAddress,&clientSockLen);
	if(no<0)
	{
		displayError("There is some problem in receiving the segment!");
	}
	cout<<"Received Acknowledgement "<<ack.ackNumber<<" for sequence number "<<ack.sequenceNumber<<endl;	
	return ack; 
}
int main(int noOfArguments,char *argumentList[])
{
	UDPServer server;
	/*
         * It checks if all the command-line arguments are provided.
         */	
	if(noOfArguments<3)
	{
		server.displayError("The client must provide a port number!");
	}
	server.createSocket();
	int portNo=atoi(argumentList[1]);	
	server.bindAddress(portNo);
	server.setClientSockLength();	
	while(1)
	{
		server.receiveRequest();
		char* fileContent=server.getRequestedContent();
		server.createSegments(fileContent,atoi(argumentList[2]));		
	}
	return 0;	
}
