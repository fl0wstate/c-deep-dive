#include <arpa/inet.h> // For inet_ntop()
#include <netdb.h>     // For getaddrinfo(), addrinfo, gai_strerror()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // For close()

void get_domain_ips(const char *domain);

int main(int ac, char *argv[])
{
  if (ac != 2)
  {
    fprintf(stderr, "Usage: %s <domain>\n", argv[0]);
    return (1);
  }
  else
    get_domain_ips(argv[1]);
  return (0);
}

/**
 * getAddrinfo - get address information from the domain given
 * domain: domain name to get address information
 * Return: address information, and port number
 */
void get_domain_ips(const char *domain)
{
  struct addrinfo hints;
  struct addrinfo *listips, *temp;
  char buf[INET6_ADDRSTRLEN];  // will be used to store the converted string ip
                               // address
  uint8_t status = 0;          // by default setting the status to 1(fail)
  uint8_t addrinfo_number = 0; // port number to connect to

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC; // which address family IPV4 or IPV6
  hints.ai_socktype =
      SOCK_STREAM; // which type of connection stream is established TCP or UDP

  // get the address information with the provided hints above
  status = getaddrinfo(domain, NULL, &hints, &listips);
  if (status != 0)
  {
    fprintf(stderr, "Error: %s", gai_strerror(status));
    exit(EXIT_FAILURE);
  }

  // loop over each of the available ips and print the out
  temp = listips;

  while (temp)
  {
    if (temp->ai_family == AF_INET6)
    {
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)temp->ai_addr;
      inet_ntop(temp->ai_family, &(ipv6->sin6_addr), buf, sizeof buf);
      printf("IPv6: %s\n", buf);
    }
    else
    {
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)temp->ai_addr;
      inet_ntop(temp->ai_family, &(ipv4->sin_addr), buf, sizeof(buf));
      printf("IPV4: %s\n", buf);
    }
    temp = temp->ai_next;
  }

  freeaddrinfo(listips);
}
