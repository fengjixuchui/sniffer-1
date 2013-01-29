// =====================================================================================
// 
//       Filename:  capturethread.cpp
//
//    Description:  后台捕获数据的多线程类实现文件
//
//        Version:  1.0
//        Created:  2013年01月24日 21时44分49秒
//       Revision:  none
//       Compiler:  g++
//
//         Author:  Hurley (LiuHuan), liuhuan1992@gmail.com
//        Company:  Class 1107 of Computer Science and Technology
// 
// =====================================================================================

#include "capturethread.h"
#include "listtreeview.h"
#include "sniffertype.h"
#include "Sniffer.h"

#ifdef WIN32
	#pragma warning(disable:4996)
#endif

CaptureThread::CaptureThread()
{
	bStopped = false;
	sniffer  = NULL;
	mainTree = NULL;
}

CaptureThread::CaptureThread(ListTreeView *pTree, Sniffer *pSniffer, QString tmpFileName)
{
	bStopped = false;

	mainTree = pTree;
	sniffer  = pSniffer;
	tmpFile  = tmpFileName;
}

void CaptureThread::run()
{
	int res;
	struct tm *ltime;
	char szNum[10];
	char szLength[6];
	char timestr[16];
	time_t local_tv_sec;

	int num = 1;
	SnifferData tmpSnifferData;

	if (!tmpFile.isEmpty()) {
		sniffer->openDumpFile((const char *)tmpFile.toLocal8Bit());
	}

	while (bStopped != true && (res = sniffer->captureOnce()) >= 0) {
		if (res == 0) {
			continue;
		}

		sniffer->saveCaptureData();

		sprintf(szNum, "%d", num);
		tmpSnifferData.strNum = szNum;
		num++;

		local_tv_sec = sniffer->header->ts.tv_sec;
		ltime = localtime(&local_tv_sec);
		strftime(timestr, sizeof(timestr), "%H:%M:%S", ltime);

		tmpSnifferData.strTime = timestr;

		sprintf(szLength, "%d", sniffer->header->len);
		tmpSnifferData.strLength = szLength;

		ip_header		*ih;
		udp_header		*uh;
		tcp_header		*th;
		unsigned short	 sport, dport;
		unsigned int	 ip_len;

		// 获得 IP 协议头
		ih = (ip_header *)(sniffer->pkt_data + 14);

		// 获得 IP 头的大小
		ip_len = (ih->ver_ihl & 0xF) * 4;

		unsigned char *pByte;
		switch (ih->proto) {
			case TCP_SIG:
				tmpSnifferData.strProto = "TCP";
				th = (tcp_header *)((unsigned char *)ih + ip_len);		// 获得 TCP 协议头
				sport = ntohs(th->sport);								// 获得源端口和目的端口
				dport = ntohs(th->dport);
				break;
			case UDP_SIG:
				tmpSnifferData.strProto = "UDP";
				uh = (udp_header *)((unsigned char *)ih + ip_len);		// 获得 UDP 协议头
				sport = ntohs(uh->sport);								// 获得源端口和目的端口
				dport = ntohs(uh->dport);
				// 判断是否为 QQ 协议
				pByte = (unsigned char *)ih + ip_len + sizeof(udp_header);
				if (*pByte == QQ_SIGN) {
					tmpSnifferData.strProto += " (QQ)";
				}
				break;
			default:
				continue;
		}

		char szSaddr[24], szDaddr[24];

		sprintf( szSaddr, "%d.%d.%d.%d : %d", ih->saddr[0], ih->saddr[1], ih->saddr[2], ih->saddr[3], sport); 
		sprintf( szDaddr, "%d.%d.%d.%d : %d", ih->daddr[0], ih->daddr[1], ih->daddr[2], ih->daddr[3], dport);

		tmpSnifferData.strSIP = szSaddr;
		tmpSnifferData.strDIP = szDaddr;

		mainTree->addOneCaptureItem(tmpSnifferData.strNum.c_str(), tmpSnifferData.strTime.c_str(),
		 							tmpSnifferData.strSIP.c_str(), tmpSnifferData.strDIP.c_str(),
		 							tmpSnifferData.strProto.c_str(), tmpSnifferData.strLength.c_str());

		sniffer->snifferDataVector.push_back(tmpSnifferData);
	}
	sniffer->consolePrint();
}

void CaptureThread::stop()
{
	bStopped = true;
}