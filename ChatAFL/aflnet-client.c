#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "alloc-inl.h"
#include "aflnet.h"

#define server_wait_usecs 10000

unsigned int *(*extract_response_codes)(unsigned char *buf, unsigned int buf_size, unsigned int *state_count_ref) = NULL;
region_t *(*extract_requests)(unsigned char *buf, unsigned int buf_size, unsigned int *region_count_ref) = NULL;

/* Expected arguments:
1. Path to the test case (e.g., crash-triggering input)
2. Application protocol (e.g., RTSP, FTP)
3. Server's network port
Optional:
4. First response timeout (ms), default 1
5. Follow-up responses timeout (us), default 1000
*/

int main(int argc, char *argv[])
{
  FILE *fp;
  int portno, n, local_port = 0;
  struct sockaddr_in serv_addr;
  struct sockaddr_in local_serv_addr;

  unsigned int region_count = 0;
  char *buf = NULL, *response_buf = NULL;
  int response_buf_size = 0;
  unsigned int i, state_count, packet_count = 0;
  long length;
  unsigned int *state_sequence;
  unsigned int socket_timeout = 1000;
  unsigned int poll_timeout = 1;

  if (argc < 5)
  {
    PFATAL("Usage: ./aflnet-client packet_file protocol port local_port [first_resp_timeout(us) [follow-up_resp_timeout(ms)]]");
  }

  // Read messages from packet_file
  fp = fopen(argv[1], "rb");
  if (!fp)
  {
    printf("cannot open the packet file\n");
    return -1;
  }
  fseek(fp, 0, SEEK_END);
  length = ftell(fp);
  buf = (char *)ck_alloc(length);
  fseek(fp, 0, SEEK_SET);
  fread(buf, length, 1, fp);
  fclose(fp);

  if (!strcmp(argv[2], "MODBUS"))
  {
    extract_response_codes = &extract_response_codes_modbus;
    extract_requests = &extract_requests_modbus;
  }
  else if (!strcmp(argv[2], "IEC104"))
  {
    extract_response_codes = &extract_response_codes_iec104;
    extract_requests = &extract_requests_iec104;
  }
  else if (!strcmp(argv[2], "ETHERNETIP"))
  {
    extract_response_codes = &extract_response_codes_ethernetip;
    extract_requests = &extract_requests_ethernetip;
  }
  else if (!strcmp(argv[2], "SLMPA"))
  {
    extract_response_codes = &extract_response_codes_slmpa;
    extract_requests = &extract_requests_slmpa;
  }
  else if (!strcmp(argv[2], "SLMPB"))
  {
    extract_response_codes = &extract_response_codes_slmpb;
    extract_requests = &extract_requests_slmpb;
  }
  else if (!strcmp(argv[2], "DNP3"))
  {
    extract_response_codes = &extract_response_codes_dnp3;
    extract_requests = &extract_requests_dnp3;
  }
  else if (!strcmp(argv[2], "BACNETIP"))
  {
    extract_response_codes = &extract_response_codes_bacnetip;
    extract_requests = &extract_requests_bacnetip;
  }
  else if (!strcmp(argv[2], "MQTT"))
  {
    extract_response_codes = &extract_response_codes_mqtt;
    extract_requests = &extract_requests_mqtt;
  }
  else if (!strcmp(argv[2], "OPCUACP"))
  {
    extract_response_codes = &extract_response_codes_opcuacp;
    extract_requests = &extract_requests_opcuacp;
  }
  else
  {
    fprintf(stderr, "[AFLNet-client] Protocol %s has not been supported yet!\n", argv[2]);
    exit(1);
  }

  portno = atoi(argv[3]);
  local_port = atoi(argv[4]);

  if (argc > 5)
  {
    poll_timeout = atoi(argv[5]);
    if (argc > 6)
    {
      socket_timeout = atoi(argv[6]);
    }
  }

  // Wait for the server to initialize
  usleep(server_wait_usecs);

  if (response_buf)
  {
    ck_free(response_buf);
    response_buf = NULL;
    response_buf_size = 0;
  }

  int sockfd;
  if ((!strcmp(argv[2], "DTLS12")) || (!strcmp(argv[2], "DNS")) || (!strcmp(argv[2], "SIP")))
  {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  }
  else
  {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
  }

  if (sockfd < 0)
  {
    PFATAL("Cannot create a socket");
  }

  // Set timeout for socket data sending/receiving -- otherwise it causes a big delay
  // if the server is still alive after processing all the requests
  struct timeval timeout;

  timeout.tv_sec = 0;
  timeout.tv_usec = socket_timeout;

  setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));

  memset(&serv_addr, '0', sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(portno);
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (local_port > 0)
  {
    local_serv_addr.sin_family = AF_INET;
    local_serv_addr.sin_addr.s_addr = INADDR_ANY;
    local_serv_addr.sin_port = htons(local_port);

    local_serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(sockfd, (struct sockaddr *)&local_serv_addr, sizeof(struct sockaddr_in)))
    {
      FATAL("Unable to bind socket on local source port");
    }
  }

  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    // If it cannot connect to the server under test
    // try it again as the server initial startup time is varied
    for (n = 0; n < 1000; n++)
    {
      if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 0)
        break;
      usleep(1000);
    }
    if (n == 1000)
    {
      close(sockfd);
      return 1;
    }
  }

  region_t *regions = (*extract_requests)(buf, length, &region_count);

  // Send requests one by one
  // And save all the server responses
  klist_t(lms) * kl_messages;
  kliter_t(lms) * it;
  kl_messages = construct_kl_messages(argv[1], regions, region_count);

  for (it = kl_begin(kl_messages); it != kl_end(kl_messages); it = kl_next(it))
  {
    if (net_recv(sockfd, timeout, poll_timeout, &response_buf, &response_buf_size))
      break;
    packet_count++;
    fprintf(stderr, "\nSize of the current packet %d is  %d; content is: %s\n", packet_count, kl_val(it)->msize, kl_val(it)->mdata);
    n = net_send(sockfd, timeout, kl_val(it)->mdata, kl_val(it)->msize);
    if (n != kl_val(it)->msize)
      break;
    if (net_recv(sockfd, timeout, poll_timeout, &response_buf, &response_buf_size))
      break;
  }

  close(sockfd);

  // Extract response codes
  state_sequence = (*extract_response_codes)(response_buf, response_buf_size, &state_count);

  fprintf(stderr, "\n--------------------------------");
  fprintf(stderr, "\nResponses from server:");

  for (i = 0; i < state_count; i++)
  {
    fprintf(stderr, "%d-", state_sequence[i]);
  }

  fprintf(stderr, "\n++++++++++++++++++++++++++++++++\nResponses in details:\n");
  for (i = 0; i < response_buf_size; i++)
  {
    fprintf(stderr, "%c", response_buf[i]);
  }
  fprintf(stderr, "\n--------------------------------\n");

  // Free memory
  ck_free(state_sequence);
  if (buf)
    ck_free(buf);
  ck_free(response_buf);

  return 0;
}

