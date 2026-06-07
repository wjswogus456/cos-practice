#ifndef __BYTE_OP_H__
#define __BYTE_OP_H__

#include <cstdio>
#include <cstring>

// ─────────────────────────────────────────────────────────────────────────────
// byte_op.h
// 변수(정수) <-> 메모리(바이트 배열) 사이를 빅엔디안으로 변환하는 매크로 모음.
// 빅엔디안 = 최상위 바이트(MSB)를 먼저 저장. 네트워크 전송 표준 순서와 같다.
// 매크로 특성상 내부에 // 주석을 넣을 수 없어(줄 끝 \ 가 깨짐) 각 매크로 "위"에 설명을 단다.
// 공통: v = 값을 담은 변수, p = 메모리 커서(포인터). p는 호출될 때마다 쓴/읽은 만큼 전진(p++)한다.
// ─────────────────────────────────────────────────────────────────────────────

// 변수 v의 하위 1바이트를 주소 p에 저장하고 p를 1 전진. (0~255 또는 -128~127 한 바이트 값)
#define VAR_TO_MEM_1BYTE_BIG_ENDIAN(v, p) \
  *(p++) = v & 0xff;

// 변수 v를 2바이트로 빅엔디안 저장: 상위 바이트 먼저, 하위 바이트 나중. p는 2 전진.
#define VAR_TO_MEM_2BYTES_BIG_ENDIAN(v, p) \
  *(p++) = (v >> 8) & 0xff; *(p++) = v & 0xff;

// 변수 v를 4바이트로 빅엔디안 저장: 24,16,8,0비트 순으로 한 바이트씩. p는 4 전진.
#define VAR_TO_MEM_4BYTES_BIG_ENDIAN(v, p) \
  *(p++) = (v >> 24) & 0xff; *(p++) = (v >> 16) & 0xff; *(p++) = (v >> 8) & 0xff; *(p++) = v & 0xff;

// 메모리 p의 1바이트를 읽어 변수 v에 넣고 p를 1 전진. (디코딩)
#define MEM_TO_VAR_1BYTE_BIG_ENDIAN(p, v) \
  v = (p[0] & 0xff); p += 1;

// 메모리 p의 2바이트(빅엔디안)를 읽어 정수 v로 복원하고 p를 2 전진.
#define MEM_TO_VAR_2BYTES_BIG_ENDIAN(p, v) \
  v = ((p[0] & 0xff) << 8) | (p[1] & 0xff); p += 2;

// 메모리 p의 4바이트(빅엔디안)를 읽어 정수 v로 복원하고 p를 4 전진.
#define MEM_TO_VAR_4BYTES_BIG_ENDIAN(p, v) \
  v = ((p[0] & 0xff) << 24) | ((p[1] & 0xff) << 16) | ((p[2] & 0xff) << 8) | (p[3] & 0xff); p += 4;

// 디버그용: 주소 p부터 len바이트를 16진수로 출력(16바이트마다 줄바꿈). 패킷 내용 확인에 사용.
#define PRINT_MEM(p, len) \
  printf("Print buffer:\n  >> "); \
  for (int i=0; i<len; i++) { \
    printf("%02x ", p[i]); \
    if (i % 16 == 15) printf("\n  >> "); \
  } \
  printf("\n");

#endif /* __BYTE_OP_H__ */
