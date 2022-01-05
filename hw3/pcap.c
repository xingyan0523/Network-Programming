#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <pcap.h>

#define SNAP_LEN 1518
#define SIZE_ETHERNET 14
#define ETHER_ADDR_LEN 6
#define PAY_LINE 16

/* Ethernet header */
struct eth_hdr {
    u_char  ether_dhost[ETHER_ADDR_LEN];    /* destination host address */
    u_char  ether_shost[ETHER_ADDR_LEN];    /* source host address */
    u_short ether_type;
};

/* IP header */
struct ip_hdr {
    u_char  ip_vhl;                 /* version << 4 | header length >> 2 */
    u_char  ip_tos;                 /* type of service */
    u_short ip_len;                 /* total length */
    u_short ip_id;                  /* identification */
    u_short ip_off;                 /* fragment offset field */
    #define IP_RF 0x8000            /* reserved fragment flag */
    #define IP_DF 0x4000            /* dont fragment flag */
    #define IP_MF 0x2000            /* more fragments flag */
    #define IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
    u_char  ip_ttl;                 /* time to live */
    u_char  ip_p;                   /* protocol */
    u_short ip_sum;                 /* checksum */
    struct  in_addr ip_src,ip_dst;  /* source and dest address */
};
#define IP_HL(ip)               (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)                (((ip)->ip_vhl) >> 4)

/* TCP header */
typedef u_int tcp_seq;

struct tcp_hdr {
    u_short th_sport;               /* source port */
    u_short th_dport;               /* destination port */
    tcp_seq th_seq;                 /* sequence number */
    tcp_seq th_ack;                 /* acknowledgement number */
    u_char  th_offx2;               /* data offset, rsvd */
#define TH_OFF(th)      (((th)->th_offx2 & 0xf0) >> 4)
    u_char  th_flags;
    #define TH_FIN  0x01
    #define TH_SYN  0x02
    #define TH_RST  0x04
    #define TH_PUSH 0x08
    #define TH_ACK  0x10
    #define TH_URG  0x20
    #define TH_ECE  0x40
    #define TH_CWR  0x80
    #define TH_FLAGS        (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
    u_short th_win;                 /* window */
    u_short th_sum;                 /* checksum */
    u_short th_urp;                 /* urgent pointer */
};

/* UDP header */
struct udp_hdr {
	u_int16_t uh_sport;				/* source port */
	u_int16_t uh_dport;				/* destination port */
	u_int16_t uh_len;				/* total length */
    u_int16_t uh_sum;               /* checksum */
};

void print_payload_line(const u_char *payload, int len, int offset) {
	int i;
	int gap;
	const u_char *ch;

	printf("%04d   ", offset);
	
	ch = payload;
	for(i = 0; i < len; i++) {
		printf("%02x ", *ch++);
		if (i == 7) printf(" ");
	}
	if (len < 8) printf(" ");
	
	if (len < 16) {
		gap = 16 - len;
		for (i = 0; i < gap; i++)
			printf("   ");
	}
	printf("   ");
	ch = payload;
	for(i = 0; i < len; i++) {
		if (isprint(*ch)) printf("%c", *ch);
		else printf(".");
		ch++;
		if(i == 7) printf(" ");
	}
	printf("\n");
}

void print_mac(u_char *ptr){
	for(int i = 0; i < 5; i++){
		printf("%02x:", *ptr++);
	}
	printf("%02x", *ptr++);
}

void print_payload(const u_char *payload, int len) {
	int line_len;
	int offset = 0;
	const u_char *ch = payload;

	if (len <= 0) return;
	if (len <= PAY_LINE) {
		print_payload_line(ch, len, offset);
		return;
	}

	while(1) {
		line_len = PAY_LINE % len;
		print_payload_line(ch, line_len, offset);
		len -= line_len;
		ch += line_len;
		offset += PAY_LINE;
		if (len <= PAY_LINE) {
			print_payload_line(ch, len, offset);
			break;
		}
	}
}

void parse_packet(const struct pcap_pkthdr *header, const u_char *packet) {
	static int count = 1;
	
	struct eth_hdr *eth;
	struct ip_hdr *ip;
	struct tcp_hdr *tcp;
	struct udp_hdr *udp;
	char *payload;
        
    struct tm *tm_info;
    char timestr[26];
    time_t local_tv_sec;

	local_tv_sec = header->ts.tv_sec;
    tm_info = localtime(&local_tv_sec);
	strftime(timestr, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    
	int size_ip;
	int size_tcp;
	int size_udp;
	int size_payload;

	printf("\nPacket number %d:\n", count++);
	printf("Time: %s.%6d  ",timestr,(int)header->ts.tv_usec);
	printf("Length: %d bytes\n",header->len);

	eth = (struct eth_hdr*)(packet);

	printf("Src mac: ");
	print_mac(eth->ether_shost);
	printf("  Dst mac: ");
	print_mac(eth->ether_dhost);
	printf("  Eth type: 0x%02x%02x", eth->ether_type%256, eth->ether_type/256);
	printf("\n");

	ip = (struct ip_hdr*)(packet + SIZE_ETHERNET);
	size_ip = IP_HL(ip)*4;
	if (size_ip < 20) {
		printf("   * Invalid IP header length: %u bytes\n", size_ip);
		return;
	}

	printf("From: %s  ", inet_ntoa(ip->ip_src));
	printf("To: %s  ", inet_ntoa(ip->ip_dst));
    
	if(ip->ip_p == IPPROTO_TCP){
		printf("Protocol: TCP\n");
		/* define/compute tcp header offset */
		tcp = (struct tcp_hdr*)(packet + SIZE_ETHERNET + size_ip);
		size_tcp = TH_OFF(tcp)*4;
		if (size_tcp < 20) {
			printf("   * Invalid TCP header length: %u bytes\n", size_tcp);
			return;
		}
		
		printf("Src port: %d  ", ntohs(tcp->th_sport));
		printf("Dst port: %d\n", ntohs(tcp->th_dport));
		/* define/compute tcp payload (segment) offset */
		payload = (u_char *)(packet + SIZE_ETHERNET + size_ip + size_tcp);
		
		/* compute tcp payload (segment) size */
		size_payload = ntohs(ip->ip_len) - (size_ip + size_tcp);
	}
	else if(ip->ip_p == IPPROTO_UDP){
		printf("Protocol: UDP\n");
		/* define/compute tcp header offset */
		udp = (struct udp_hdr*)(packet + SIZE_ETHERNET + size_ip);
		size_udp = udp->uh_len;
		if (size_udp < 8) {
			printf("   * Invalid UDP header length: %u bytes\n", size_udp);
			return;
		}
		
		printf("Src port: %d  ", ntohs(udp->uh_sport));
		printf("Dst port: %d\n", ntohs(udp->uh_dport));
		/* define/compute udp payload (segment) offset */
		payload = (u_char *)(packet + SIZE_ETHERNET + size_ip + size_udp);
		
		/* compute tcp payload (segment) size */
		size_payload = ntohs(ip->ip_len) - (size_ip + size_udp);
	}
	else if(ip->ip_p == IPPROTO_ICMP){
		printf("Protocol: ICMP\n");
		return;
	}
	else if(ip->ip_p == IPPROTO_IP){
		printf("Protocol: IP\n");
		return;
	}
	else{
		printf("Protocol: unknown\n");
		return;
	}

	if(size_payload > 0) printf("Payload: \n");
	print_payload(payload, size_payload);
}

int main(int argc, char **argv) {			
	pcap_if_t *dev = NULL;				/* capture device name */
	char errbuf[PCAP_ERRBUF_SIZE];		/* error buffer */
	pcap_t *handle;						/* packet capture handle */
	u_char *packet;
	struct pcap_pkthdr header;
	
	const char *filter= "";
	struct bpf_program fp;
	
	if (argc == 2) {
	    filter = argv[1]; 
	}
	else if (argc > 2) {
		fprintf(stderr, "error\n");
		exit(1);
	}
	else {
		if (pcap_findalldevs(&dev, errbuf) == -1) {
			fprintf(stderr, "Couldn't find device: %s\n", errbuf);
			exit(1);
		}
	}

	printf("Device: %s\n", dev[0].name);
	printf("Filter expression: %s\n", filter);

	handle = pcap_open_offline("./test.pcap", errbuf);
	if (handle == NULL) {
		fprintf(stderr, "Couldn't open file %s: %s\n", dev[0].name, errbuf);
		exit(1);
	}

	if (pcap_datalink(handle) != DLT_EN10MB) {
		fprintf(stderr, "%s is not an Ethernet\n", dev[0].name);
		exit(1);
	}

	if (pcap_compile(handle, &fp, filter, 0, 0) == -1) {
		fprintf(stderr, "Couldn't parse filter %s: %s\n",
		    filter, pcap_geterr(handle));
		exit(1);
	}

	if (pcap_setfilter(handle, &fp) == -1) {
		fprintf(stderr, "Couldn't install filter %s: %s\n",
		    filter, pcap_geterr(handle));
		exit(1);
	}

	while(packet = (u_char*)pcap_next(handle, &header)){
		parse_packet(&header, packet);
	}
	
	pcap_freecode(&fp);
	pcap_close(handle);
	pcap_freealldevs(dev);

	printf("\nComplete.\n");

	return 0;
}
