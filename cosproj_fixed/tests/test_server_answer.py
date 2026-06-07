import socket
import threading
import argparse
import logging

OPCODE_REPLY = 2   # 결과 응답 opcode

# protocol_execution(): 한 클라이언트(Alice)와의 프로토콜을 처리한다. (서버=Bob)
# 이름을 주고받은 뒤, 두 정수의 합 요청을 받아 계산해서 돌려준다.
def protocol_execution(sock):
    # 1. Alice -> Bob: length of the name (4 bytes) || name (length bytes)
    # Get the length information (4 bytes)
    buf = sock.recv(4)                              # 이름 길이 4바이트 수신
    length = int.from_bytes(buf, "big")            # 빅엔디안 -> 정수
    logging.info("[*] Length received: {}".format(length))

    # Get the name (Alice)
    buf = sock.recv(length)                         # 길이만큼 이름 수신
    logging.info("[*] Name received: {}".format(buf.decode()))

    # 2. Bob -> Alice: length of the name (4 bytes) || name (length bytes)
    # Send the length information (4 bytes)
    name = "Bob"                                    # 내 이름
    length = len(name)                              # 이름 길이
    logging.info("[*] Length to be sent: {}".format(length))
    sock.send(int.to_bytes(length, 4, "big"))      # 길이를 4바이트 빅엔디안으로 전송

    # Send the name (Bob)
    logging.info("[*] Name to be sent: {}".format(name))
    sock.send(name.encode())                        # 이름 바이트 전송

    # Implement following the instructions below
    # 3. Alice -> Bob: opcode (4 bytes) || arg1 (4 bytes) || arg2 (4 bytes)
    # The opcode should be 1
    buf = sock.recv(12)                             # opcode(4)+arg1(4)+arg2(4) = 12바이트 수신

    # The values are encoded in the big-endian style and should be translated into the little-endian style (because my machine follows the little-endian style)
    # 주의: 클라이언트는 빅엔디안으로 보냈는데 여기서는 "little"로 해석한다.
    # (이 정답 파일은 리틀엔디안 기준으로 작성됨 — 인코딩/디코딩 엔디안 합의가 중요함을 보여주는 예)
    opcode = int.from_bytes(buf[0:4], "little")    # 앞 4바이트 -> opcode
    arg1 = int.from_bytes(buf[4:8], "little")      # 다음 4바이트 -> arg1
    arg2 = int.from_bytes(buf[8:12], "little")     # 다음 4바이트 -> arg2

    logging.info("[*] Opcode: {}".format(opcode))
    logging.info("[*] Arg1: {}".format(arg1))
    logging.info("[*] Arg2: {}".format(arg2))

    # 4. Bob -> Alice: opcode (4 bytes) || result (4 bytes)
    # The opcode should be 2
    result = arg1 + arg2                            # 두 인자의 합 계산
    logging.info("[*] Result: {}".format(result))
    opcode = 2
    sock.send(int.to_bytes(OPCODE_REPLY, 4, "big"))  # 응답 opcode(2) 4바이트 전송
    sock.send(int.to_bytes(result, 4, "big"))        # 결과 4바이트 전송

    sock.close()                                   # 연결 종료

# run(): 지정한 주소/포트에서 TCP 서버를 열고, 접속마다 protocol_execution을 스레드로 처리.
def run(addr, port):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((addr, port))                      # 주소/포트 바인딩

    server.listen(2)                               # 연결 대기 시작
    logging.info("[*] Server is Listening on {}:{}".format(addr, port))

    while True:
        client, info = server.accept()             # 연결 수락(블로킹)

        logging.info("[*] Server accept the connection from {}:{}".format(info[0], info[1]))

        # 연결마다 별도 스레드로 처리
        client_handle = threading.Thread(target=protocol_execution, args=(client,))
        client_handle.start()

# 명령행 인자(-a 주소, -p 포트, -l 로그레벨) 파싱
def command_line_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--addr", metavar="<server's IP address>", help="Server's IP address", type=str, default="0.0.0.0")
    parser.add_argument("-p", "--port", metavar="<server's open port>", help="Server's port", type=int, required=True)
    parser.add_argument("-l", "--log", metavar="<log level (DEBUG/INFO/WARNING/ERROR/CRITICAL)>", help="Log level (DEBUG/INFO/WARNING/ERROR/CRITICAL)", type=str, default="INFO")
    args = parser.parse_args()
    return args

def main():
    args = command_line_args()
    log_level = args.log
    logging.basicConfig(level=log_level)           # 로깅 레벨 설정

    run(args.addr, args.port)                       # 서버 실행

if __name__ == "__main__":
    main()
