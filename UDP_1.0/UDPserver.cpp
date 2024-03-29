#include <stdlib.h>
#include <time.h>
#include <WinSock2.h>
#include <fstream>

#pragma comment(lib,"ws2_32.lib")
#define SERVER_PORT 12340    //端口号
#define SERVER_IP "0.0.0.0"  //IP地址
const int BUFFER_LENGTH = 1026;//缓冲区大小，（以太网中 UDP 的数据 帧中包长度应小于 1480 字节） 

const int SEND_WIND_SIZE = 10;
//发送窗口大小为 10，GBN 中应满足 W + 1 <= N（W 为发送窗口大小，N 为序列号个数）
//本例取序列号 0...19 共 20 个 
//如果将窗口大小设为 1，则为停-等协议 
const int SEQ_SIZE = 20;
//序列号的个数，从 0~19 共计 20 个
//由于发送数据第一个字节如果值为 0，则数据会发送 失败
//因此接收端序列号为 1~20，与发送端一一对应 
BOOL ack[SEQ_SIZE];//收到 ack 情况，对应 0~19 的 ack
int curSeq;        //当前数据包的 seq
int curAck;        //当前等待确认的 ack
int totalSeq;      //收到的包的总数
int totalPacket;   //需要发送的包总数

//************************************ 
// Method: getCurTime 
// FullName: getCurTime 
// Access: public 
// Returns: void 
// Qualifier: 获取当前系统时间，结果存入 ptime 中 
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
	sprintf_s(buffer, "%d/%d/%d %d:%d:%d", p->tm_year + 1900, p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);//将数据格式化输出到字符串
	strcpy_s(ptime, sizeof(buffer), buffer);   //系统的安全函数，参数（目标内存，目标内存的实际大小，源内存）第二个参数用于检测是否能装下源内存里的数据
}

//************************************ 
// Method: seqIsAvailable 
// FullName: seqIsAvailable 
// Access: public 
// Returns: bool 
// Qualifier: 当前序列号 curSeq 是否可用 
//************************************ 
bool seqIsAvailable()
{
	int step;
	step = curSeq - curAck;
	step = step >= 0 ? step : step + SEQ_SIZE; //序列号是否在当前发送窗口之内
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
// Qualifier: 超时重传处理函数，滑动窗口内的数据帧都要重传 
//************************************ 
void timeoutHandler()
{
	printf("连接超时.\n");
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
// Qualifier: 收到 ack，累积确认，取数据帧的第一个字节 
//由于发送数据时，第一个字节（序列号）为 0（ASCII）时发送失败，因此加一 了，此处需要减一还原 
// Parameter: char c 
//************************************ 
void ackHandler(char c)
{
	unsigned char index = (unsigned char)c - 1;//序列号 - 1
	printf("服务器:::接受到 ack： %d\n", index + 1);//序号发送方和接收方应该统一，发送的是+1过的，接收方Ack+1过的，在这里打印没必要还原
	if (curAck <= index){
		for (int i = curAck; i <= index; ++i){
			ack[i] = TRUE;
		}
		curAck = (index + 1) % SEQ_SIZE;
	}
	else{  //ack超过最大值，重置到 curAck 的左边
		for (int i = curAck; i < SEQ_SIZE; ++i){
			ack[i] = TRUE;
		}
		for (int i = 0; i <= index; ++i){
			ack[i] = TRUE;
		}
		curAck = index + 1;
	}
}

int main(int argc, char*argv[]){     //加载套接字库（必须！！）
	WORD wVersionRequested;
	WSADATA wsaData; //套接字加载错误提示
	int err;  //版本 2.2
	wVersionRequested = MAKEWORD(2, 2); //加载 dll 文件 Socket 库
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {    //找不到 winsock.dll s
		printf("WSAStartup  失败，错误信息： %d\n", err);
		return -1;
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2){   //LOBYTE:得到一个16bit数最低（最右边）那个字节;HIBYTE:得到一个16bit数最高（最左边）那个字节    //当LOBYTE, HIBYTE应用于32bit数时, 实际上这时应该用于32bit数的后16bit.
		printf("找不到可用的 Winsock.dll 版本\n");
		WSACleanup();
	}
	else{
		printf("Winsock 2.2 dll 匹配成功\n");
	}
	SOCKET sockServer = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //设置套接字为非阻塞模式
	int iMode = 1; // 1：非阻塞，0：阻塞
	ioctlsocket(sockServer, FIONBIO, (u_long FAR*) &iMode);//非阻塞时设置
	
	SOCKADDR_IN addrServer; //服务器地址
	addrServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//或者:addrServer.sin_addr.S_un.S_addr = inet_addr(SERVER_IP); 
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(SERVER_PORT);   //端口号
	unsigned short seq;//包的序列号 
	unsigned short recvSeq;//接收窗口大小为 1，已确认的序列号 
	unsigned short waitSeq;//等待的序列号 
	err = bind(sockServer, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
	if (err){
		err = GetLastError();
		printf("无法为套接字绑定端口： %d.错误代码：%d\n", SERVER_PORT, err);
		WSACleanup(); 
		return -1;
	}
	SOCKADDR_IN addrClient; //客户端地址
	int length = sizeof(SOCKADDR);
	char buffer[BUFFER_LENGTH]; //数据发送接收缓冲区
	ZeroMemory(buffer, sizeof(buffer)); //初始化缓冲区并读入测试数据
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
	while (true){  //非阻塞接受，若没有收到数据，返回值为-1
		recvSize = recvfrom(sockServer, buffer, BUFFER_LENGTH, 0, ((SOCKADDR*)&addrClient),&length);
		if (recvSize < 0){
			Sleep(200);
			continue;
		}
		printf("从客户端接收：%s\n", buffer);
		if (strcmp(buffer, "-time") == 0){
			getCurTime(buffer);
		}
		else if (strcmp(buffer, "-quit") == 0){
			strcpy_s(buffer, strlen("生活愉快！再见") + 1, "生活愉快！再见");
		}
		else if (strcmp(buffer, "-testgbn") == 0){
			//进入 GBN 测试阶段
			//首先 server (server 处于 0 状态 ) 向 client 发送 205 状态码（server 进入 1 状态）
			//server 等待 client 回复 200 状态码，如果收到 （server 进入 2 状态），则开始传输文件，否则延时等待至超时
			//在文件传输阶段， server 发送窗口大小设为
			ZeroMemory(buffer, sizeof(buffer));
			int recvSize;
			int waitCount = 0;
			printf("开始测试GBN协议，请不要中止进程\n");
			//加入一个握手阶段
			//首先服务器向客户端发送一个 205 大小的状态码（自定义） 表示服务器准备好了，可以发送数据
			//客户端收到 205 之后回复一个 200 大小的状态码，表示客户端准备好了， 可以接受数据
			//服务器收到 200 状态码之后，就开始使用 GBN 发送数据
			printf("握手阶段运行中\n");
			int stage = 0;
			bool runFlag = true;
			while (runFlag){
				switch (stage){
				case 0: //发送 205 阶段
					buffer[0] = 205;
					sendto(sockServer, buffer, strlen(buffer) + 1, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
					Sleep(100);
					stage = 1;
					break;
				case 1: //等待接收 200 阶段，没有收到则计数器+1，超时则放弃此次“链接”，等待从第一步开始
					recvSize = recvfrom(sockServer, buffer, BUFFER_LENGTH, 0, ((SOCKADDR*)&addrClient), &length);
					if (recvSize < 0){
						++waitCount;
						if (waitCount > 20){
							runFlag = false;
							printf("连接超时\n");
							break;
						}
						Sleep(500);
						continue;
					}
					else{
						if ((unsigned char)buffer[0] == 200){ //客户端返回200状态码，连接建立成功，初始化全局数据并进入数据传输阶段
							printf("握手成功，开始文件传输\n");
							printf("文件大小为：%dB，每个包为：1024B，包总数为：%d\n", sizeof(data), totalPacket);
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
				case 2://数据传输阶段
					if (seqIsAvailable()){
						//发送给客户端的序列号从 1 开始
						buffer[0] = curSeq + 1; //seq
						ack[curSeq] = FALSE;
						//数据发送的过程中应该判断是否传输完成
						//为简化过程此处并未实现 - - - - - - - - - - - - - - - -套你猴子一会加
						buffer[1] = recvSeq;
						memcpy(&buffer[2], data + 1024 * totalSeq, 1024);
						printf("服务器:::发送一个seq：%d, ack：%d 的包\n", curSeq + 1, buffer[1]);
						sendto(sockServer, buffer, BUFFER_LENGTH, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
						++curSeq;
						curSeq %= SEQ_SIZE;
						++totalSeq;
						Sleep(500);
					}   //等待 Ack， 若没有收到，则返回值为-1，计数器+1
					recvSize = recvfrom(sockServer, buffer, BUFFER_LENGTH, 0, (SOCKADDR*)&addrClient, &length);
					if (recvSize < 0){
						waitCount++; //20 次等待 ack 则超时重传'
						if (waitCount > 20){
							timeoutHandler();
							waitCount = 0;
						}
					}
					else{
						//收到 客户端 ack 和客户端发送的消息
						seq = (unsigned short)buffer[1];
						if (waitSeq - seq == 0)//收到的序列号正好是所等待的序列号：接受 ACK，数据
						{
							recvSeq = seq;
							++waitSeq;
							if (waitSeq == 20){
								waitSeq = 1;
							}
							printf("服务器:::接收到客户端包seq：%d\n", seq);//打印来自客户端数据的序号
							ackHandler(buffer[0]);
						}
						else
						{//没有接收到数据
							printf("服务器:::接收到客户端包seq：%d\n", seq);//打印来自客户端数据的序号
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
	//关闭套接字，卸载库
	closesocket(sockServer);
	WSACleanup();
	return 0;
}