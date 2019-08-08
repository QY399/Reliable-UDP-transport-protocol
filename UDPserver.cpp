#include <stdlib.h>
#include <time.h>
#include <WinSock2.h>
#include <fstream>

#pragma comment(lib,"ws2_32.lib")
#define SERVER_PORT 12340    //�˿ں�
#define SERVER_IP "0.0.0.0"  //IP��ַ
const int BUFFER_LENGTH = 1026;//��������С������̫���� UDP ������ ֡�а�����ӦС�� 1480 �ֽڣ� 

const int SEND_WIND_SIZE = 10;
//���ʹ��ڴ�СΪ 10��GBN ��Ӧ���� W + 1 <= N��W Ϊ���ʹ��ڴ�С��N Ϊ���кŸ�����
//����ȡ���к� 0...19 �� 20 �� 
//��������ڴ�С��Ϊ 1����Ϊͣ-��Э�� 
const int SEQ_SIZE = 20;
//���кŵĸ������� 0~19 ���� 20 ��
//���ڷ������ݵ�һ���ֽ����ֵΪ 0�������ݻᷢ�� ʧ��
//��˽��ն����к�Ϊ 1~20���뷢�Ͷ�һһ��Ӧ 
BOOL ack[SEQ_SIZE];//�յ� ack �������Ӧ 0~19 �� ack
int curSeq;        //��ǰ���ݰ��� seq
int curAck;        //��ǰ�ȴ�ȷ�ϵ� ack
int totalSeq;      //�յ��İ�������
int totalPacket;   //��Ҫ���͵İ�����

//************************************ 
// Method: getCurTime 
// FullName: getCurTime 
// Access: public 
// Returns: void 
// Qualifier: ��ȡ��ǰϵͳʱ�䣬������� ptime �� 
// Parameter: char * ptime 
//************************************ 
void getCurTime(char *ptime)
{
	char buffer[128];
	memset(buffer, 0, sizeof(buffer));
	time_t c_time;
	struct tm *p;
	time(&c_time);
	p = localtime(&c_time);
	sprintf_s(buffer, "%d/%d/%d %d:%d:%d", p->tm_year + 1900, p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);//�����ݸ�ʽ��������ַ���
	strcpy_s(ptime, sizeof(buffer), buffer);   //ϵͳ�İ�ȫ������������Ŀ���ڴ棬Ŀ���ڴ��ʵ�ʴ�С��Դ�ڴ棩�ڶ����������ڼ���Ƿ���װ��Դ�ڴ��������
}

//************************************ 
// Method: seqIsAvailable 
// FullName: seqIsAvailable 
// Access: public 
// Returns: bool 
// Qualifier: ��ǰ���к� curSeq �Ƿ���� 
//************************************ 
bool seqIsAvailable()
{
	int step;
	step = curSeq - curAck;
	step = step >= 0 ? step : step + SEQ_SIZE; //���к��Ƿ��ڵ�ǰ���ʹ���֮��
	if (step >= SEND_WIND_SIZE){
		return false;
	}
	if (ack[curSeq]){
		return true;
	}
	return false;
}

//************************************ 
// Method: timeoutHandler 
// FullName: timeoutHandler 
// Access: public 
// Returns: void 
// Qualifier: ��ʱ�ش��������������������ڵ�����֡��Ҫ�ش� 
//************************************ 
void timeoutHandler()
{
	printf("Timer out error.\n");
	int index;
	for (int i = 0; i < SEND_WIND_SIZE; ++i){
		index = (i + curAck) % SEQ_SIZE;
		ack[index] = TRUE;
	}
	totalSeq -= SEND_WIND_SIZE;
	curSeq = curAck;
}

//************************************ 
// Method: ackHandler 
// FullName: ackHandler 
// Access: public 
// Returns: void 
// Qualifier: �յ� ack���ۻ�ȷ�ϣ�ȡ����֡�ĵ�һ���ֽ� 
//���ڷ�������ʱ����һ���ֽڣ����кţ�Ϊ 0��ASCII��ʱ����ʧ�ܣ���˼�һ �ˣ��˴���Ҫ��һ��ԭ 
// Parameter: char c 
//************************************ 
void ackHandler(char c)
{
	unsigned char index = (unsigned char)c - 1;//���к� - 1
	printf("server:::Recv a ack of %d\n", index + 1);//��ŷ��ͷ��ͽ��շ�Ӧ��ͳһ�����͵���+1���ģ����շ�Ack+1���ģ��������ӡû��Ҫ��ԭ
	if (curAck <= index){
		for (int i = curAck; i <= index; ++i){
			ack[i] = TRUE;
		}
		curAck = (index + 1) % SEQ_SIZE;
	}
	else{  //ack�������ֵ�����õ� curAck �����
		for (int i = curAck; i < SEQ_SIZE; ++i){
			ack[i] = TRUE;
		}
		for (int i = 0; i <= index; ++i){
			ack[i] = TRUE;
		}
		curAck = index + 1;
	}
}

int main(int argc, char*argv[]){     //�����׽��ֿ⣨���룡����
	WORD wVersionRequested;
	WSADATA wsaData; //�׽��ּ��ش�����ʾ
	int err;  //�汾 2.2
	wVersionRequested = MAKEWORD(2, 2); //���� dll �ļ� Socket ��
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {    //�Ҳ��� winsock.dll s
		printf("WSAStartup failed with error: %d\n", err);
		return -1;
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2){   //LOBYTE:�õ�һ��16bit����ͣ����ұߣ��Ǹ��ֽ�;HIBYTE:�õ�һ��16bit����ߣ�����ߣ��Ǹ��ֽ�    //��LOBYTE, HIBYTEӦ����32bit��ʱ, ʵ������ʱӦ������32bit���ĺ�16bit.
		printf("Could not find a usable version of Winsock.dll\n");
		WSACleanup();
	}
	else{
		printf("The Winsock 2.2 dll was found okay\n");
	}
	SOCKET sockServer = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //�����׽���Ϊ������ģʽ
	int iMode = 1; // 1����������0������
	ioctlsocket(sockServer, FIONBIO, (u_long FAR*) &iMode);//������ʱ����
	
	SOCKADDR_IN addrServer; //��������ַ
	addrServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//����:addrServer.sin_addr.S_un.S_addr = inet_addr(SERVER_IP); 
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(SERVER_PORT);   //�˿ں�
	unsigned short seq;//�������к� 
	unsigned short recvSeq;//���մ��ڴ�СΪ 1����ȷ�ϵ����к� 
	unsigned short waitSeq;//�ȴ������к� 
	err = bind(sockServer, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
	if (err){
		err = GetLastError();
		printf("Could not bind the port %d for socket.Error code is %d\n", SERVER_PORT, err);
		WSACleanup(); 
		return -1;
	}
	SOCKADDR_IN addrClient; //�ͻ��˵�ַ
	int length = sizeof(SOCKADDR);
	char buffer[BUFFER_LENGTH]; //���ݷ��ͽ��ջ�����
	ZeroMemory(buffer, sizeof(buffer)); //��ʼ���������������������
	std::ifstream icin; 
	icin.open("test.txt");
	char data[1024 * 113];
	ZeroMemory(data, sizeof(data));
	icin.read(data, 1024 * 113);
	icin.close();
	totalPacket = sizeof(data) / 1024;
	int recvSize;
	for (int i = 0; i < SEQ_SIZE; ++i){
		ack[i] = TRUE;
	}
	while (true){  //���������ܣ���û���յ����ݣ�����ֵΪ-1
		recvSize = recvfrom(sockServer, buffer, BUFFER_LENGTH, 0, ((SOCKADDR*)&addrClient),&length);
		if (recvSize < 0){
			Sleep(200);
			continue;
		}
		printf("recv from client:%s\n", buffer);
		if (strcmp(buffer, "-time") == 0){
			getCurTime(buffer);
		}
		else if (strcmp(buffer, "-quit") == 0){
			strcpy_s(buffer, strlen("Good bye and happy life!") + 1, "Good bye and happy life!");
		}
		else if (strcmp(buffer, "-testgbn") == 0){
			//���� GBN ���Խ׶�
			//���� server (server ���� 0 ״̬ ) �� client ���� 205 ״̬�루server ���� 1 ״̬��
			//server �ȴ� client �ظ� 200 ״̬�룬����յ� ��server ���� 2 ״̬������ʼ�����ļ���������ʱ�ȴ�����ʱ
			//���ļ�����׶Σ� server ���ʹ��ڴ�С��Ϊ
			ZeroMemory(buffer, sizeof(buffer));
			int recvSize;
			int waitCount = 0;
			printf("Begain to test GBN protocol,please don't abort the process\n");
			//����һ�����ֽ׶�
			//���ȷ�������ͻ��˷���һ�� 205 ��С��״̬�루�Զ��壩 ��ʾ������׼�����ˣ����Է�������
			//�ͻ����յ� 205 ֮��ظ�һ�� 200 ��С��״̬�룬��ʾ�ͻ���׼�����ˣ� ���Խ�������
			//�������յ� 200 ״̬��֮�󣬾Ϳ�ʼʹ�� GBN ��������
			printf("Shake hands stage running\n");
			int stage = 0;
			bool runFlag = true;
			while (runFlag){
				switch (stage){
				case 0: //���� 205 �׶�
					buffer[0] = 205;
					sendto(sockServer, buffer, strlen(buffer) + 1, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
					Sleep(100);
					stage = 1;
					break;
				case 1: //�ȴ����� 200 �׶Σ�û���յ��������+1����ʱ������˴Ρ����ӡ����ȴ��ӵ�һ����ʼ
					recvSize = recvfrom(sockServer, buffer, BUFFER_LENGTH, 0, ((SOCKADDR*)&addrClient), &length);
					if (recvSize < 0){
						++waitCount;
						if (waitCount > 20){
							runFlag = false;
							printf("TImeout error\n");
							break;
						}
						Sleep(500);
						continue;
					}
					else{
						if ((unsigned char)buffer[0] == 200){ //�ͻ��˷���200״̬�룬���ӽ����ɹ�����ʼ��ȫ�����ݲ��������ݴ���׶�
							printf("Shake hands successfully, begin a file transfer\n");
							printf("File size is %dB, each packet is 1024B and packet total num is %d\n", sizeof(data), totalPacket);
							curSeq = 0;
							curAck = 0;
							totalSeq = 0;
							waitCount = 0;
							stage = 2;
							recvSeq = 0;
							waitSeq = 1;
						}
					}
					break;
				case 2://���ݴ���׶�
					if (seqIsAvailable()){
						//���͸��ͻ��˵����кŴ� 1 ��ʼ
						buffer[0] = curSeq + 1; //seq
						ack[curSeq] = FALSE;
						//���ݷ��͵Ĺ�����Ӧ���ж��Ƿ������
						//Ϊ�򻯹��̴˴���δʵ�� - - - - - - - - - - - - - - - -�������һ���
						buffer[1] = recvSeq;
						memcpy(&buffer[2], data + 1024 * totalSeq, 1024);
						printf("server:::send a packet with a seq of %d, ack of %d \n", curSeq + 1, buffer[1]);
						sendto(sockServer, buffer, BUFFER_LENGTH, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
						++curSeq;
						curSeq %= SEQ_SIZE;
						++totalSeq;
						Sleep(500);
					}   //�ȴ� Ack�� ��û���յ����򷵻�ֵΪ-1��������+1
					recvSize = recvfrom(sockServer, buffer, BUFFER_LENGTH, 0, (SOCKADDR*)&addrClient, &length);
					if (recvSize < 0){
						waitCount++; //20 �εȴ� ack ��ʱ�ش�'
						if (waitCount > 20){
							timeoutHandler();
							waitCount = 0;
						}
					}
					else{
						//�յ� �ͻ��� ack �Ϳͻ��˷��͵���Ϣ
						seq = (unsigned short)buffer[1];
						if (waitSeq - seq == 0)//�յ������к����������ȴ������кţ����� ACK������
						{
							recvSeq = seq;
							++waitSeq;
							if (waitSeq == 20){
								waitSeq = 1;
							}
							printf("server:::Recv a client packet of %d\n", seq);//��ӡ���Կͻ������ݵ����
							ackHandler(buffer[0]);
						}
						else
						{//û�н��յ�����
							printf("server:::Recv a client packet of %d\n", seq);//��ӡ���Կͻ������ݵ����
							ackHandler(buffer[0]);
						}
						Sleep(500);
						break;
					}
				}
			}
		}
		sendto(sockServer, buffer, strlen(buffer) + 1, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
		Sleep(500);
	}
	//�ر��׽��֣�ж�ؿ�
	closesocket(sockServer);
	WSACleanup();
	return 0;
}