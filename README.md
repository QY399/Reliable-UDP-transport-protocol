# UDP可靠传输协议
## 项目简介：
基于面向连接的、可靠的、基于字节流的 UDP 传输层通信协议，摒弃在快速传输短数据包时TCP的冗余功能，取得比 TCP 通讯更好的效果，在数据的实时性、传输效率、带宽的利用率等方面优于TCP。
## 功能实现：
   采取 GBN 协议的滑动窗口，保证数据帧有序传输。  
   实现一个带超时的请求回应机制，让业务层负责超时重发，保证了数据传输一定限度下的可靠性。  
   加入自定义报头，利用状态码拆分数据包，模拟握手连接。  
   使用非阻塞I/O，防止网络拥塞情况下造成的性能下降，匹配 UDP 短数据包，高并发的传输优势。  
   完成测试 server 和 client，给定丢包率情况下传输流程可视化。  

### 一、滑动窗口采用GBN协议：  
发送窗口大小为 10，GBN 中应满足 W + 1 <= N（W 为发送窗口大小，N 为序列号个数）  
本例取序列号 0...19 共 20 个   
如果将窗口大小设为 1，则为停-等协议
协议详见[理解GBN协议](https://blog.csdn.net/do_best_/article/details/79771841)
```
   const int BUFFER_LENGTH = 1026;//缓冲区大小，（以太网中 UDP 的数据 帧中包长度应小于 1480 字节） 
   
   const int SEND_WIND_SIZE = 10;
   const int SEQ_SIZE = 20;
   //序列号的个数，从 0~19 共计 20 个
   //由于发送数据第一个字节如果值为 0，则数据会发送 失败
   //因此接收端序列号为 1~20，与发送端一一对应 
   BOOL ack[SEQ_SIZE];//收到 ack 情况，对应 0~19 的 ack
   int curSeq;        //当前数据包的 seq
   int curAck;        //当前等待确认的 ack
   int totalSeq;      //收到的包的总数
   int totalPacket;   //需要发送的包总数
```  

### 二、超时重传  
```
   //************************************ 
   // Method: timeoutHandler 
   // FullName: timeoutHandler 
   // Access: public 
   // Returns: void 
   // Qualifier: 超时重传处理函数，滑动窗口内的数据帧都要重传 
   //************************************ 
   void timeoutHandler()
   {
   	printf("连接超时.\n")；
      int index;
	   for (int i = 0; i < SEND_WIND_SIZE; ++i){
	   	index = (i + curAck) % SEQ_SIZE;
	   	ack[index] = TRUE;
	   }
	   totalSeq -= SEND_WIND_SIZE;
	   curSeq = curAck;
   }  
```

### 三、自定义报头  
将数据包的第一位空置，用以存放自定义的状态码，建立伪握手阶段
```
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
						Sleep(500);
						continue;
						if ((unsigned char)buffer[0] == 200){ //客户端返回200状态码，连接建立成功，初始化全局数据并进入数据传输阶段
							stage = 2;
						}
					}
		break;
		case 2://数据传输阶段
```

### 四、server端非阻塞
```
	SOCKET sockServer = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //设置套接字为非阻塞模式
	int iMode = 1; // 1：非阻塞，0：阻塞
	ioctlsocket(sockServer, FIONBIO, (u_long FAR*) &iMode);//非阻塞时设置
```
