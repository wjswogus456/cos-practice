#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <getopt.h>
#include <stdint.h>

#include "../edge/byte_op.h"

#define BUFLEN        1024
#define OPCODE_SUM    1     // 합을 요청하는 opcode
#define OPCODE_REPLY  2     // 결과를 응답하는 opcode

void protocol_execution(int sock);
void error_handling(const char *message);

// 사용법 출력 후 종료
void usage(const char *pname)
{
  printf(">> Usage: %s [options]\n", pname);
  printf("Options\n");
  printf("  -a, --addr       Server's address\n");
  printf("  -p, --port       Server's port\n");
  exit(0);
}

// main(): 명령행 인자(-a 주소, -p 포트)를 파싱하고, 서버에 TCP 연결한 뒤
//         protocol_execution()으로 실제 프로토콜을 수행한다. (클라이언트=Alice)
int main(int argc, char *argv[])
{
	int sock;                          // 소켓 디스크립터
	struct sockaddr_in serv_addr;      // 서버 주소 구조체
  char msg[] = "Hello, World!\n";
	char message[30] = {0, };
	int c, port, tmp, str_len;
  char *pname;                       // 프로그램 이름
  uint8_t *addr;                     // 서버 주소 문자열
  uint8_t eflag;                     // 인자 오류 플래그

  pname = argv[0];
  addr = NULL;
  port = -1;
  eflag = 0;

  // getopt_long으로 -a/-p 옵션 파싱
  while (1)
  {
    int option_index = 0;
    static struct option long_options[] = {
      {"addr", required_argument, 0, 'a'},
      {"port", required_argument, 0, 'p'},
      {0, 0, 0, 0}
    };

    const char *opt = "a:p:0";

    c = getopt_long(argc, argv, opt, long_options, &option_index);

    if (c == -1)                     // 더 이상 옵션이 없으면 종료
      break;

    switch (c)
    {
      case 'a':                      // 서버 주소 복사
        tmp = strlen(optarg);
        addr = (uint8_t *)malloc(tmp);
        memcpy(addr, optarg, tmp);
        break;

      case 'p':                      // 포트 문자열 -> 정수
        port = atoi(optarg);
        break;

      default:
        usage(pname);
    }
  }

  if (!addr)                         // 주소 미지정 시 오류
  {
    printf("[*] Please specify the server's address to connect\n");
    eflag = 1;
  }

  if (port < 0)                      // 포트 미지정 시 오류
  {
    printf("[*] Please specify the server's port to connect\n");
    eflag = 1;
  }

  if (eflag)                         // 오류가 있으면 사용법 출력 후 종료
  {
    usage(pname);
    exit(0);
  }

	sock = socket(PF_INET, SOCK_STREAM, 0);   // TCP 소켓 생성
	if (sock == -1)
		error_handling("socket() error");
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr((const char *)addr);  // 문자열 IP -> 네트워크 주소
	serv_addr.sin_port = htons(port);                           // 포트를 네트워크 바이트 순서로

	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)  // 서버 연결
		error_handling("connect() error");
  printf("[*] Connected to %s:%d\n", addr, port);
  
  protocol_execution(sock);          // 프로토콜 수행

	close(sock);                       // 소켓 닫기
	return 0;
}

// protocol_execution(): 이름 교환 후, 두 정수의 합을 서버에 요청하고 결과를 받는다.
// tbs = 보낼 바이트 수(to be sent), tbr = 받을 바이트 수(to be received), offset = 진행 위치
void protocol_execution(int sock)
{
  char msg[] = "Alice";              // 내 이름
  char buf[BUFLEN];
  int tbs, sent, tbr, rcvd, offset;
  int len;

  // tbs: the number of bytes to send
  // tbr: the number of bytes to receive
  // offset: the offset of the message

  // 1. Alice -> Bob: length of the name (4 bytes) || name (length bytes)
  // Send the length information (4 bytes)
  len = strlen(msg);                 // 이름 길이
  printf("[*] Length information to be sent: %d\n", len);

  len = htonl(len);                  // 길이를 네트워크(빅엔디안) 바이트 순서로 변환
  tbs = 4;                           // 길이 필드는 4바이트
  offset = 0;

  // write가 일부만 보낼 수 있으므로 tbs를 다 보낼 때까지 반복
  while (offset < tbs)
  {
    sent = write(sock, &len + offset, tbs - offset);
    if (sent > 0)
      offset += sent;
  }

  // Send the name (Alice)
  tbs = ntohl(len);                  // 길이를 다시 호스트 순서로 -> 이름 바이트 수
  offset = 0;

  printf("[*] Name to be sent: %s\n", msg);
  while (offset < tbs)               // 이름 전체를 전송
  {
    sent = write(sock, msg + offset, tbs - offset);
    if (sent > 0)
      offset += sent;
  }

  // 2. Bob -> Alice: length of the name (4 bytes) || name (length bytes)
  // Receive the length information (4 bytes)
  tbr = 4;
  offset = 0;

  while (offset < tbr)               // 상대 이름 길이(4바이트) 수신
  {
	  rcvd = read(sock, &len + offset, tbr - offset);
    if (rcvd > 0)
      offset += rcvd;
  }
  len = ntohl(len);                  // 빅엔디안 -> 호스트 순서
  printf("[*] Length received: %d\n", len);

  // Receive the name (Bob)
  tbr = len;
  offset = 0;

  while (offset < tbr)               // 상대 이름 바이트 수신
  {
    rcvd = read(sock, buf + offset, tbr - offset);
    if (rcvd > 0)
      offset += rcvd;
  }

	printf("[*] Name received: %s \n", buf);

  // Implement following the instructions below
  // Let's assume there are two opcodes:
  //     1: summation request for the two arguments
  //     2: reply with the result
  // 3. Alice -> Bob: opcode (4 bytes) || arg1 (4 bytes) || arg2 (4 bytes)
  // The opcode should be 1

  char *p;
  int i, arg1, arg2;

  memset(buf, 0, BUFLEN);            // 송신 버퍼 초기화
  p = buf;                           // 쓰기 커서
  arg1 = 2;                          // 더할 첫 번째 값
  arg2 = 5;                          // 더할 두 번째 값

  VAR_TO_MEM_1BYTE_BIG_ENDIAN(OPCODE_SUM, p);   // opcode(합 요청) 기록
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(arg1, p);        // arg1 4바이트 기록
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(arg2, p);        // arg2 4바이트 기록
  tbs = p - buf;                                // 기록한 총 바이트 수
  offset = 0;

  printf("[*] # of bytes to be sent: %d\n", tbs);
  printf("[*] The following bytes will be sent\n");
  for (i=0; i<tbs; i++)                          // 보낼 바이트를 16진수로 출력
    printf ("%02x ", buf[i]);
  printf("\n");

  while (offset < tbs)                           // 요청 전송
  {
    sent = write(sock, buf + offset, tbs - offset);
    if (sent > 0)
      offset += sent;
  }

  // 4. Bob -> Alice: opcode (4 bytes) || result (4 bytes)
  // The opcode should be 2

  int opcode, result;

  tbr = 8; offset = 0;               // 응답은 opcode(4) + result(4) = 8바이트
  memset(buf, 0, BUFLEN);            // 수신 버퍼 초기화

  printf("[*] # of bytes to be received: %d\n", tbr);
  while (offset < tbr)               // 8바이트를 모두 받을 때까지 반복
  {
    rcvd = read(sock, buf + offset, tbs - offset);
    if (rcvd > 0)
      offset += rcvd;
  }

  printf("[*] The following bytes is received\n");
  for (i=0; i<tbr; i++)              // 받은 바이트를 16진수로 출력
    printf("%02x ", buf[i]);
  printf("\n");

  p = buf;                           // 디코딩 커서
  MEM_TO_VAR_4BYTES_BIG_ENDIAN(p, opcode);   // 앞 4바이트 -> opcode
  printf("[*] Opcode: %d\n", opcode);
  MEM_TO_VAR_4BYTES_BIG_ENDIAN(p, result);   // 다음 4바이트 -> result(합)
  printf("[*] Result: %d\n", result);
}

// 에러 메시지를 출력하고 프로그램 종료
void error_handling(const char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
