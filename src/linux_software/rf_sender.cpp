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

#define DDS_CLK_FREQ 125000000
#define DDS_PHASE_ACCUM_WIDTH 134217728

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

  if(argc != 2)
  {
    std::cout << "Not enough arguments, syntax is: rf-sender <ip-address>"
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

  *(radio_p+RADIO_TUNER_CONTROL_REG_OFFSET) = 1;

  bool exit = false;
  double freq = 0;
  double phase_inc_calc_ratio = (double)DDS_PHASE_ACCUM_WIDTH/DDS_CLK_FREQ;
  int phase_inc = 0;
  unsigned char user_input;
  unsigned int bytes_received = 0;
  unsigned int num_bytes_to_receive = 1;

  std::cout << "Welcome to the Cameron Frey's final radio lab\n";
  std::cout << "Enter u or U to increase freq, d or D to decrease freq\n";
  std::cout << "Enter f to input a frequency, z to reset DDS phase\n";
  std::cout << "Enter t to tune\n";
  std::cout << "Enter udp to send ethernet UDP packets\n";
  std::cout << "Enter e to exit\n";
  while(!exit)
  {
    std::string user_input;
    std::cin >> user_input;
    if(user_input == "u")
    {
      freq += 100;
      phase_inc = (int)(freq * phase_inc_calc_ratio);
      *(radio_p+RADIO_TUNER_FAKE_ADC_PINC_OFFSET) = phase_inc;
      printf("Phase inc set to %d, Frequency set to %fHz\n", phase_inc, freq);
    }
    else if(user_input == "U")
    {
      freq += 1000;
      phase_inc = (int)(freq * phase_inc_calc_ratio);
      *(radio_p+RADIO_TUNER_FAKE_ADC_PINC_OFFSET) = phase_inc;
      printf("Phase inc set to %d, Frequency set to %fHz\n", phase_inc, freq);
    }
    else if(user_input == "d")
    {
      if(freq < 100)
        freq = 0;
      else
        freq -= 100;
      phase_inc = (int)(freq * phase_inc_calc_ratio);
      *(radio_p+RADIO_TUNER_FAKE_ADC_PINC_OFFSET) = phase_inc;
      printf("Phase inc set to %d, Frequency set to %fHz\n", phase_inc, freq);
    }
    else if(user_input == "D")
    {
      if(freq < 1000)
        freq = 0;
      else
        freq -= 1000;
      phase_inc = (int)(freq * phase_inc_calc_ratio);
      *(radio_p+RADIO_TUNER_FAKE_ADC_PINC_OFFSET) = phase_inc;
      printf("Phase inc set to %d, Frequency set to %fHz\n", phase_inc, freq);
    }
    else if(user_input == "z")
    {
      *(radio_p+RADIO_TUNER_CONTROL_REG_OFFSET) = 0;
      *(radio_p+RADIO_TUNER_CONTROL_REG_OFFSET) = 1;
    }
    else if(user_input == "f")
    {
      printf("Enter a frequency then press enter:");
      std::cin >> freq;
      printf("%d\n", freq);

      phase_inc = (int)(freq * phase_inc_calc_ratio);
      *(radio_p+RADIO_TUNER_FAKE_ADC_PINC_OFFSET) = phase_inc;
      printf("Phase inc set to %d, Frequency set to %fHz\r\n", phase_inc, freq);
    }
    else if(user_input == "t")
    {
      printf("Enter a frequency then press enter:");
      std::cin >> freq;
      printf("%d\n", freq);

      phase_inc = (int)(freq * phase_inc_calc_ratio);
      *(radio_p+RADIO_TUNER_TUNER_PINC_OFFSET) = phase_inc;
      printf("Phase inc set to %d, Frequency set to %fHz\r\n", phase_inc, freq);
    }
    else if(user_input == "udp")
    {
      std::uint16_t rf_packet[NUM_SHORTS];
      int num_packets;
      std::cout << "Enter number of packets to send:";
      std::cin >> num_packets;
      std::cout << std::endl;

      unsigned int fifo_read_occupancy;

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

      std::cout << "Sent " << i << " packets" << std::endl;
    }
    else if(user_input == "e")
    {
      exit = true;
    }
  }

  *(radio_p+RADIO_TUNER_CONTROL_REG_OFFSET) = 0;
  close(sockfd);
  return 0;
}

