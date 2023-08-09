// Client side implementation of UDP client-server model
#include <bits/stdc++.h>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


#define RADIO_TUNER_FAKE_ADC_PINC_OFFSET 0
#define RADIO_TUNER_TUNER_PINC_OFFSET 1
#define RADIO_TUNER_CONTROL_REG_OFFSET 2
#define RADIO_TUNER_TIMER_REG_OFFSET 3
#define RADIO_PERIPH_ADDRESS 0x43c00000

#define FIFO_ADDRESS 0x43c10000
#define FIFO_OCCUPANCY_OFFSET 1
#define FIFO_DATA_OFFSET 2
   
#define PORT 25344
#define MAXLINE 1024

#define NUM_SHORTS 513
#define FIFO_PACKET_WIDTH 256

// the below code uses a device called /dev/mem to get a pointer to a physical
// address.  We will use this pointer to read/write the custom peripheral
volatile unsigned int * get_a_pointer(unsigned int phys_addr)
{
	int mem_fd = open("/dev/mem", O_RDWR | O_SYNC); 
	void *map_base = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, phys_addr); 
	volatile unsigned int *radio_base = (volatile unsigned int *)map_base; 
	return (radio_base);
}


int main(int argc, char **argv) {

  if(argc != 3)
  {
    std::cout << "Not enough arguments, syntax is: rf-sender <ip-address> <num_packets>"
      << std::endl;
    return EXIT_FAILURE;
  }
  
  volatile unsigned int *fifo_p = get_a_pointer(FIFO_ADDRESS);
  volatile unsigned int *radio_p = get_a_pointer(RADIO_PERIPH_ADDRESS);

  int sockfd;
  struct sockaddr_in servaddr;

  // Creating socket file descriptor
  if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  memset(&servaddr, 0, sizeof(servaddr));

  // Filling server information
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(PORT);
  servaddr.sin_addr.s_addr = inet_addr(argv[1]);

  std::uint16_t rf_packet[NUM_SHORTS];
  int num_packets = std::stoi(argv[2]);
  
  unsigned int fifo_read_occupancy;

  *(radio_p+RADIO_TUNER_FAKE_ADC_PINC_OFFSET) = 30001000;
  *(radio_p+RADIO_TUNER_TUNER_PINC_OFFSET) = 30000000;
  *(radio_p+RADIO_TUNER_CONTROL_REG_OFFSET) = 1;

  int iq = 0, i = 0;

  for(; i < num_packets; i++)
  {
    rf_packet[0] = i;
    int *rf_data = (int *)&rf_packet[1];
    while(*(fifo_p+FIFO_OCCUPANCY_OFFSET) < FIFO_PACKET_WIDTH);
    for(int j = 0; j < FIFO_PACKET_WIDTH; j++)
    {
      *(rf_data+j) = *(fifo_p+FIFO_DATA_OFFSET);
    }

    sendto(sockfd, (void *)rf_packet, NUM_SHORTS * 2,
        MSG_CONFIRM, (const struct sockaddr *) &servaddr, 
        sizeof(servaddr));
  }

  *(radio_p+RADIO_TUNER_CONTROL_REG_OFFSET) = 0;

  std::cout << "Sent " << i << " packets" << std::endl;
  close(sockfd);
  return 0;
}
