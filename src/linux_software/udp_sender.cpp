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
   
#define PORT 25344
#define MAXLINE 1024

#define NUM_SHORTS 513
   
// Driver code
int main(int argc, char **argv) {

  if(argc != 3)
  {
    std::cout << "Not enough arguments, syntax is: rf-sender <ip-address> <num_packets>"
      << std::endl;
    return EXIT_FAILURE;
  }

  int sockfd;
  char buffer[MAXLINE];
  const char *hello = "Hello from client";
  struct sockaddr_in     servaddr;

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

  int iq = 0, i = 0;

  for(; i < num_packets; i++)
  {
    rf_packet[0] = i;
    for(int j = 1; j < NUM_SHORTS; j++)
      rf_packet[j] = iq++;

    sendto(sockfd, (void *)rf_packet, NUM_SHORTS * 2,
        MSG_CONFIRM, (const struct sockaddr *) &servaddr, 
        sizeof(servaddr));
  }

  std::cout << "Sent " << i << " packets" << std::endl;
  close(sockfd);
  return 0;
}
