import socket
import requests
import threading
import argparse
import logging
import json
import sys

OPCODE_DATA = 1
OPCODE_WAIT = 2
OPCODE_DONE = 3
OPCODE_QUIT = 4

# Version -> dimension mapping. The packet starts with VERSION, and each value is a signed 4-byte integer.
# v3 is the final rubric-safe setting; v4 is a feature-superset collection mode; v5 is a compact test version.
VERSION_DIM = {1: 3, 2: 5, 3: 10, 4: 16, 5: 3}

class Server:
    def __init__(self, name, algorithm, dimension, index, port, caddr, cport, ntrain, ntest, collect=None):
        logging.info("[*] Initializing the server module to receive data from the edge device")
        self.name = name
        self.algorithm = algorithm
        self.dimension = dimension
        self.index = index
        self.caddr = caddr
        self.cport = cport
        self.ntrain = ntrain
        self.ntest = ntest
        self.collect = collect          # None=정상 모드, 문자열=수집 모드(파일 prefix)

        # 수집 모드에서는 시작 시 AI 모델을 만들 필요가 없으므로 connecter를 건너뛴다.
        if self.collect:
            logging.info("[*] COLLECT MODE: superset 데이터만 모아 '{}_train.json' / '{}_test.json'으로 저장 (AI 모듈 호출 없음)".format(self.collect, self.collect))
            success = True
        else:
            success = self.connecter()

        if success:
            self.port = port
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.bind(("0.0.0.0", port))
            self.socket.listen(10)
            self.listener()

    def connecter(self):
        # 시작 시 AI 모듈에 모델 생성을 요청한다. (POST /<name> : algorithm/dimension/index)
        # 성공 여부(success)를 반환하고, 성공해야만 edge용 소켓을 연다.
        success = True
        # AI 모듈로의 raw TCP 소켓 연결(도달성 확인용). 실제 요청은 아래 requests(HTTP)로 보낸다.
        self.ai = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.ai.connect((self.caddr, self.cport))
        # 모델 생성 엔드포인트 URL: http://<AI주소>:<AI포트>/<모델이름>
        url = "http://{}:{}/{}".format(self.caddr, self.cport, self.name)
        # 모델 생성에 필요한 파라미터를 dict로 구성
        request = {}
        request['algorithm'] = self.algorithm      # 사용할 알고리즘 (예: lstm)
        request['dimension'] = self.dimension      # 입력 벡터 차원 수
        request['index'] = self.index              # 예측 대상 값의 인덱스
        # ai.py가 본문을 이중 디코딩하므로 dict를 JSON 문자열로 한 번 직렬화해서 보낸다.
        js = json.dumps(request)
        logging.debug("[*] To be sent to the AI module: {}".format(js))
        # POST 요청 전송 후 응답(JSON)을 파싱
        result = requests.post(url, json=js)
        response = json.loads(result.content)
        logging.debug("[*] Received: {}".format(response))

        # 응답에 opcode가 없으면 비정상 응답 -> 실패 처리
        if "opcode" not in response:
            logging.debug("[*] Invalid response")
            success = False
        else:
            # opcode가 failure면 모델 생성 실패. reason이 있으면 함께 로그.
            if response["opcode"] == "failure":
                logging.error("Error happened")
                if "reason" in response:
                    logging.error("Reason: {}".format(response["reason"]))
                    logging.error("Please try again.")
                else:
                    logging.error("Reason: unknown. not specified")
                success = False
            else:
                # 그 외에는 반드시 success 여야 한다(방어적 assert).
                assert response["opcode"] == "success"
                logging.info("[*] Successfully connected to the AI module")
        return success

    def listener(self):
        # edge 디바이스의 접속을 받는 TCP 서버 루프.
        logging.info("[*] Server is listening on 0.0.0.0:{}".format(self.port))

        while True:
            # 새 연결을 수락(블로킹). client=연결 소켓, info=(상대 IP, 포트)
            client, info = self.socket.accept()
            logging.info("[*] Server accept the connection from {}:{}".format(info[0], info[1]))

            # 연결마다 별도 스레드. 수집 모드면 handler_collect, 아니면 정상 handler.
            target = self.handler_collect if self.collect else self.handler
            client_handle = threading.Thread(target=target, args=(client,))
            client_handle.start()

    def recv_exact(self, conn, n):
        """TCP partial recv 대비: 정확히 n바이트가 모일 때까지 반복 수신."""
        buf = b""
        while len(buf) < n:
            chunk = conn.recv(n - len(buf))
            if not chunk:
                # 연결이 끊어진 경우
                raise ConnectionError("Connection closed after {} of {} bytes".format(len(buf), n))
            buf += chunk
        return buf

    def parse_data(self, buf):
        """
        buf: 4*dimension 바이트 (VERSION 바이트 제외, VALUES만).
        각 값은 signed 32-bit big-endian (4바이트).
        반환: 정수 리스트 (dimension 개).
        """
        dim = len(buf) // 4
        value_list = []
        for i in range(dim):
            raw = buf[i*4 : i*4 + 4]
            val = int.from_bytes(raw, byteorder="big", signed=True)
            value_list.append(val)
        logging.debug("[*] parse_data: raw bytes = {}".format(buf.hex()))
        logging.info("[*] parse_data: value list (dim={}) = {}".format(dim, value_list))
        return value_list

    def send_instance(self, vlst, is_training):
        """value_list를 AI 모듈의 /name/training 또는 /name/testing에 PUT."""
        if is_training:
            url = "http://{}:{}/{}/training".format(self.caddr, self.cport, self.name)
        else:
            url = "http://{}:{}/{}/testing".format(self.caddr, self.cport, self.name)

        data = {"value": vlst}
        logging.debug("[*] send_instance: PUT {} with value = {}".format(url, vlst))

        # ai.py가 본문을 json.loads(request.get_json(force=True))로 이중 디코딩하므로
        # 본문은 'JSON 문자열'이어야 한다. dict를 json.dumps로 한 번 감싼 뒤 보낸다.
        req = json.dumps(data)
        response = requests.put(url, json=req)
        resp = response.json()
        logging.debug("[*] send_instance: response = {}".format(resp))

        if "opcode" not in resp:
            logging.error("[*] send_instance: unknown response (no opcode)")
            sys.exit(1)

        if resp["opcode"] == "failure":
            logging.error("[*] send_instance: AI module returned failure")
            if "reason" in resp:
                logging.error("[*] reason: {}".format(resp["reason"]))
            else:
                logging.error("[*] unknown error")
            sys.exit(1)

        # success 시 prediction 로그 (testing phase에서 반환될 수 있음)
        if "prediction" in resp:
            logging.info("[*] send_instance: prediction = {}".format(resp["prediction"]))

    def handler(self, client):
        logging.info("[*] Server starts to process the client's request")
        logging.info("[*] Model config: dimension={}, index={}".format(self.dimension, self.index))

        ntrain = self.ntrain
        url_train = "http://{}:{}/{}/training".format(self.caddr, self.cport, self.name)

        # ── Training phase ──────────────────────────────────────────────────
        logging.info("[*] Training phase: expecting {} packets".format(ntrain))

        while True:
            # 1) opcode 1바이트 수신
            raw_opcode = self.recv_exact(client, 1)
            opcode = int.from_bytes(raw_opcode, byteorder="big")
            logging.debug("[*] handler: received opcode = {}".format(opcode))

            if opcode != OPCODE_DATA:
                logging.error("[*] handler: unexpected opcode {} (expected OPCODE_DATA={})".format(opcode, OPCODE_DATA))
                sys.exit(1)

            logging.info("[*] handler: OPCODE_DATA received")

            # 2) VERSION 1바이트 수신
            raw_version = self.recv_exact(client, 1)
            version = int.from_bytes(raw_version, byteorder="big")
            logging.info("[*] handler: packet version = {}".format(version))

            # 3) version -> dimension 확인
            if version not in VERSION_DIM:
                logging.error("[*] handler: unknown version {}".format(version))
                sys.exit(1)
            pkt_dim = VERSION_DIM[version]
            logging.info("[*] handler: version {} -> dimension = {}".format(version, pkt_dim))

            # 4) 서버 설정과 일치 여부 검증
            assert pkt_dim == self.dimension, (
                "Dimension mismatch: packet version {} implies dim={}, but server was started with --dimension {}. "
                "Start server with matching --dimension.".format(version, pkt_dim, self.dimension)
            )

            # 5) VALUES: 4 * dimension 바이트 수신
            raw_values = self.recv_exact(client, 4 * pkt_dim)
            logging.debug("[*] handler: raw VALUES bytes = {}".format(raw_values.hex()))

            # 6) 파싱 후 AI 모듈로 전송
            value_list = self.parse_data(raw_values)
            logging.info("[*] handler: training value_list = {}  (remaining: {})".format(value_list, ntrain - 1))
            self.send_instance(value_list, is_training=True)

            ntrain -= 1
            logging.debug("[*] handler: ntrain remaining = {}".format(ntrain))

            if ntrain > 0:
                # 아직 더 받아야 함 -> OPCODE_DONE 전송
                logging.debug("[*] handler: sending OPCODE_DONE")
                client.send(int.to_bytes(OPCODE_DONE, 1, "big"))
            else:
                # 훈련 데이터 수신 완료 -> OPCODE_WAIT 전송 후 학습 트리거
                logging.info("[*] handler: training data complete, sending OPCODE_WAIT")
                client.send(int.to_bytes(OPCODE_WAIT, 1, "big"))
                break

        # ── 모델 학습 트리거: POST /name/training ───────────────────────────
        logging.info("[*] handler: triggering model training (POST {})".format(url_train))
        result = requests.post(url_train)
        response = json.loads(result.content)
        logging.debug("[*] handler: training POST response = {}".format(response))

        # 학습 완료 -> OPCODE_DONE 전송 (edge가 testing 시작 대기 중)
        logging.info("[*] handler: training done, sending OPCODE_DONE")
        client.send(int.to_bytes(OPCODE_DONE, 1, "big"))

        # ── Testing phase ───────────────────────────────────────────────────
        ntest = self.ntest
        logging.info("[*] Testing phase: expecting {} packets".format(ntest))

        while ntest > 0:
            # 1) opcode 1바이트 수신
            raw_opcode = self.recv_exact(client, 1)
            opcode = int.from_bytes(raw_opcode, byteorder="big")
            logging.debug("[*] handler: received opcode = {}".format(opcode))

            if opcode != OPCODE_DATA:
                logging.error("[*] handler: unexpected opcode {} (expected OPCODE_DATA={})".format(opcode, OPCODE_DATA))
                sys.exit(1)

            logging.info("[*] handler: OPCODE_DATA received")

            # 2) VERSION 1바이트 수신
            raw_version = self.recv_exact(client, 1)
            version = int.from_bytes(raw_version, byteorder="big")
            logging.info("[*] handler: packet version = {}".format(version))

            # 3) version -> dimension 확인
            if version not in VERSION_DIM:
                logging.error("[*] handler: unknown version {}".format(version))
                sys.exit(1)
            pkt_dim = VERSION_DIM[version]

            # 4) 서버 설정과 일치 여부 검증
            assert pkt_dim == self.dimension, (
                "Dimension mismatch: packet version {} implies dim={}, but server --dimension {}".format(
                    version, pkt_dim, self.dimension)
            )

            # 5) VALUES 수신
            raw_values = self.recv_exact(client, 4 * pkt_dim)
            logging.debug("[*] handler: raw VALUES bytes = {}".format(raw_values.hex()))

            # 6) 파싱 후 AI 모듈로 전송 (testing)
            value_list = self.parse_data(raw_values)
            logging.info("[*] handler: testing value_list = {}  (remaining: {})".format(value_list, ntest - 1))
            self.send_instance(value_list, is_training=False)

            ntest -= 1
            logging.debug("[*] handler: ntest remaining = {}".format(ntest))

            if ntest > 0:
                logging.debug("[*] handler: sending OPCODE_DONE")
                client.send(int.to_bytes(OPCODE_DONE, 1, "big"))
            else:
                # 테스트 데이터 수신 완료 -> OPCODE_QUIT
                logging.info("[*] handler: testing complete, sending OPCODE_QUIT")
                client.send(int.to_bytes(OPCODE_QUIT, 1, "big"))
                break

        # ── 결과 조회 ───────────────────────────────────────────────────────
        url_result = "http://{}:{}/{}/result".format(self.caddr, self.cport, self.name)
        logging.info("[*] handler: fetching result from {}".format(url_result))
        result = requests.get(url_result)
        response = json.loads(result.content)
        logging.debug("[*] handler: result response = {}".format(response))

        if "opcode" not in response:
            logging.error("[*] handler: invalid response from AI module (no opcode)")
            sys.exit(1)

        if response["opcode"] == "failure":
            logging.error("[*] handler: AI module returned failure for result")
            if "reason" in response:
                logging.error("[*] reason: {}".format(response["reason"]))
            sys.exit(1)
        elif response["opcode"] == "success":
            self.print_result(response)
        else:
            logging.error("[*] handler: unknown opcode in result response")
            sys.exit(1)

        client.close()
        logging.info("[*] handler: connection closed")

    def _recv_one_packet(self, client):
        """opcode(1)+version(1)+values(4*dim)를 받아 (opcode, version, value_list) 반환.
        수집 모드 전용: dim은 패킷의 version에서 결정하며 self.dimension과 비교하지 않는다."""
        opcode = int.from_bytes(self.recv_exact(client, 1), byteorder="big")
        if opcode != OPCODE_DATA:
            logging.error("[*] collect: unexpected opcode {} (expected {})".format(opcode, OPCODE_DATA))
            sys.exit(1)
        version = int.from_bytes(self.recv_exact(client, 1), byteorder="big")
        if version not in VERSION_DIM:
            logging.error("[*] collect: unknown version {}".format(version))
            sys.exit(1)
        pkt_dim = VERSION_DIM[version]
        raw_values = self.recv_exact(client, 4 * pkt_dim)
        value_list = self.parse_data(raw_values)
        return opcode, version, value_list

    def handler_collect(self, client):
        """수집 모드 handler. 정상 handler와 '동일한 opcode 송신 순서'를 유지하되,
        AI 모듈로 보내는 대신 superset 행을 train_rows/test_rows에 모아 JSON으로 저장한다."""
        import json as _json
        logging.info("[*] COLLECT: start. expecting train={} test={}".format(self.ntrain, self.ntest))
        train_rows = []
        test_rows = []

        # ── Training phase: ntrain개 수신 ──
        ntrain = self.ntrain
        while True:
            _, version, value_list = self._recv_one_packet(client)
            train_rows.append(value_list)
            logging.info("[*] COLLECT: train row {}/{} (v{}, dim={})".format(len(train_rows), self.ntrain, version, len(value_list)))
            ntrain -= 1
            if ntrain > 0:
                client.send(int.to_bytes(OPCODE_DONE, 1, "big"))
            else:
                client.send(int.to_bytes(OPCODE_WAIT, 1, "big"))   # 정상 handler와 동일 위치
                break

        # 정상 handler는 여기서 POST(training)로 학습을 트리거하지만, 수집 모드는 생략.
        # 단, 학습 후 보내던 OPCODE_DONE 은 프로토콜 타이밍 유지를 위해 그대로 보낸다.
        client.send(int.to_bytes(OPCODE_DONE, 1, "big"))

        # ── Testing phase: ntest개 수신 ──
        ntest = self.ntest
        while ntest > 0:
            _, version, value_list = self._recv_one_packet(client)
            test_rows.append(value_list)
            logging.info("[*] COLLECT: test row {}/{}".format(len(test_rows), self.ntest))
            ntest -= 1
            if ntest > 0:
                client.send(int.to_bytes(OPCODE_DONE, 1, "big"))
            else:
                client.send(int.to_bytes(OPCODE_QUIT, 1, "big"))
                break

        # ── 저장 ──
        ftrain = "{}_train.json".format(self.collect)
        ftest = "{}_test.json".format(self.collect)
        with open(ftrain, "w") as f:
            _json.dump(train_rows, f)
        with open(ftest, "w") as f:
            _json.dump(test_rows, f)
        logging.info("[*] COLLECT DONE: saved {} train rows -> {}, {} test rows -> {}".format(
            len(train_rows), ftrain, len(test_rows), ftest))
        client.close()

    def print_result(self, result):
        logging.info("=== Result of Prediction ({}) ===".format(self.name))
        logging.info("   # of instances: {}".format(result["num"]))
        logging.debug("   sequence: {}".format(result["sequence"]))
        logging.debug("   prediction: {}".format(result["prediction"]))
        logging.info("   correct predictions: {}".format(result["correct"]))
        logging.info("   incorrect predictions: {}".format(result["incorrect"]))
        logging.info("   accuracy: {}%".format(result["accuracy"]))

def command_line_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-a", "--algorithm", metavar="<AI algorithm to be used>", help="AI algorithm to be used", type=str, required=True)
    parser.add_argument("-d", "--dimension", metavar="<Dimension of each instance>", help="Dimension of each instance", type=int, default=1)
    parser.add_argument("-b", "--caddr", metavar="<AI module's IP address>", help="AI module's IP address", type=str, required=True)
    parser.add_argument("-c", "--cport", metavar="<AI module's listening port>", help="AI module's listening port", type=int, required=True)
    parser.add_argument("-p", "--lport", metavar="<server's listening port>", help="Server's listening port", type=int, required=True)
    parser.add_argument("-n", "--name", metavar="<model name>", help="Name of the model", type=str, default="model")
    parser.add_argument("-x", "--ntrain", metavar="<number of instances for training>", help="Number of instances for training", type=int, default=10)
    parser.add_argument("-y", "--ntest", metavar="<number of instances for testing>", help="Number of instances for testing", type=int, default=10)
    parser.add_argument("-z", "--index", metavar="<the index number for the power value>", help="Index number for the power value", type=int, default=0)
    parser.add_argument("-l", "--log", metavar="<log level (DEBUG/INFO/WARNING/ERROR/CRITICAL)>", help="Log level (DEBUG/INFO/WARNING/ERROR/CRITICAL)", type=str, default="INFO")
    parser.add_argument("--collect", metavar="<file prefix>", help="COLLECT MODE: edge가 보낸 v4 superset을 <prefix>_train.json/_test.json으로 저장만 함 (AI 모듈 호출 안 함). 자동 feature 선택용 데이터 수집.", type=str, default=None)
    args = parser.parse_args()
    return args

def main():
    args = command_line_args()
    logging.basicConfig(level=args.log)

    if args.ntrain <= 0 or args.ntest <= 0:
        logging.error("Number of instances for training or testing should be larger than 0")
        sys.exit(1)

    Server(args.name, args.algorithm, args.dimension, args.index, args.lport, args.caddr, args.cport, args.ntrain, args.ntest, collect=args.collect)

if __name__ == "__main__":
    main()
