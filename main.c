//#define _POSIX_C_SOURCE 199309L
//libraries necessary for wifi ioctl communication
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/wireless.h>
#include <sys/ioctl.h>
#include <stdio.h>


//struct to hold collected information
struct signalInfo {
    char mac[18];
    char ssid[33];
    int bitrate;
    int level;
};

int getSignalInfo(struct signalInfo *sigInfo, char *iwname);
int set_essid(int sock, const char *ifname, const char *essid);
int getRSSI(char *iwname);

int main(int argc, char *argv[])
{

  struct signalInfo sig;
  struct timespec ts, time;
  int result;
  
  ts.tv_sec = 0;
  ts.tv_nsec = 100 * 1000000;

 
  while(1){
	result = getRSSI("wlp1s0");
	printf("result : %d\n", result);   

    nanosleep(&ts, NULL);
  }
/* 
  while (1) {
    result = getSignalInfo(&sig, "wlx909f330dc15b");

    if(result >= 0) {
      printf("result: %d, bit : %d\n", sig.level, sig.bitrate);
	  printf("ssid : %s\n", sig.ssid);
    }
    ts.tv_sec = 0;
    ts.tv_nsec = 100 * 1000000;

    clock_gettime(CLOCK_MONOTONIC, &time);

    nanosleep(&ts, NULL);
  }
*/
/*
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	set_essid(sockfd, "wlp1s0", "Android");
*/
	return 0;
}

int getRSSI(char *iwname){
	struct iwreq req;							// wireless request struct
    struct iw_statistics *stats;
	
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
	strcpy(req.ifr_name, iwname);				// copy wireless interface name

	req.u.data.pointer = (struct iw_statistics *)malloc(sizeof(struct iw_statistics));
    req.u.data.length = sizeof(struct iw_statistics);

	if(ioctl(sockfd, SIOCGIWSTATS, &req) == -1){ 
		return -1;		//off or disconnected, or invaild name
	}
	else if(((struct iw_statistics *)req.u.data.pointer)->qual.updated & IW_QUAL_DBM){
        return ((struct iw_statistics *)req.u.data.pointer)->qual.level - 256;		//signal level (dBm)
    }

	close(sockfd);
}


//SIOCGIWRATE for bits/sec (convert to mbit)
int getMbitRate(char *iwname){
	struct iwreq req;							// wireless request struct
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);	
    int bitrate=-1;

    //this will get the bitrate of the link
    if(ioctl(sockfd, SIOCGIWRATE, &req) == -1){
        fprintf(stderr, "bitratefail");
        return(-1);
    }else{
        memcpy(&bitrate, &req.u.bitrate, sizeof(int));
        return bitrate/1000000;
    }

}

int getSignalInfo(struct signalInfo *sigInfo, char *iwname){
    struct iwreq req;
    strcpy(req.ifr_name, iwname);

    struct iw_statistics *stats;

    //have to use a socket for ioctl
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    //make room for the iw_statistics object
    req.u.data.pointer = (struct iw_statistics *)malloc(sizeof(struct iw_statistics));
    req.u.data.length = sizeof(struct iw_statistics);

    //this will gather the signal strength
    if(ioctl(sockfd, SIOCGIWSTATS, &req) == -1){
        //die with error, invalid interface
        fprintf(stderr, "Invalid interface.\n");
        return(-1);
    }
    else if(((struct iw_statistics *)req.u.data.pointer)->qual.updated & IW_QUAL_DBM){
        //signal is measured in dBm and is valid for us to use
        sigInfo->level=((struct iw_statistics *)req.u.data.pointer)->qual.level - 256;
    }

    //SIOCGIWESSID for ssid
    char buffer[32];
    memset(buffer, 0, 32);
    req.u.essid.pointer = buffer;
    req.u.essid.length = 32;
    //this will gather the SSID of the connected network
    if(ioctl(sockfd, SIOCGIWESSID, &req) == -1){
        //die with error, invalid interface
        return(-1);
    }
    else{
        memcpy(&sigInfo->ssid, req.u.essid.pointer, req.u.essid.length);
        memset(&sigInfo->ssid[req.u.essid.length],0,1);
    }

    //SIOCGIWRATE for bits/sec (convert to mbit)
    int bitrate=-1;
    //this will get the bitrate of the link
    if(ioctl(sockfd, SIOCGIWRATE, &req) == -1){
        fprintf(stderr, "bitratefail");
        return(-1);
    }else{
        memcpy(&bitrate, &req.u.bitrate, sizeof(int));
        sigInfo->bitrate=bitrate/1000000;
    }


    //SIOCGIFHWADDR for mac addr
    struct ifreq req2;
    strcpy(req2.ifr_name, iwname);
    //this will get the mac address of the interface
    if(ioctl(sockfd, SIOCGIFHWADDR, &req2) == -1){
        fprintf(stderr, "mac error");
        return(-1);
    }
    else{
        sprintf(sigInfo->mac, "%.2X", (unsigned char)req2.ifr_hwaddr.sa_data[0]);
        for(int s=1; s<6; s++){
            sprintf(sigInfo->mac+strlen(sigInfo->mac), ":%.2X", (unsigned char)req2.ifr_hwaddr.sa_data[s]);
        }
    }
    close(sockfd);
}

int set_essid(int sock, const char *ifname, const char *essid){
	char buf[IW_ESSID_MAX_SIZE + 1];
	struct iwreq req;
	int ret;

	req.u.essid.pointer = buf;
	req.u.essid.length = snprintf(buf, ((IW_ESSID_MAX_SIZE + 1) * sizeof(char)), "%s", essid);
	req.u.essid.flags = (1 == 1);

	strncpy(req.ifr_name, ifname, IFNAMSIZ);
	ret = ioctl(sock, SIOCSIWESSID, &req);

	if(ret < 0) printf("ERROR\n");

	return ret;
}
