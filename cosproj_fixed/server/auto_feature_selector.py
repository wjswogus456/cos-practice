"""
auto_feature_selector.py  —  Accuracy-based Automatic Feature Selection

[무엇을 하는가]
수집 모드(server.py --collect)로 저장한 superset(v4, 16개 feature) train/test 데이터를 읽어,
전력(avg_power)을 예측 target으로 고정하고, 나머지 후보 feature 중 (total-1)개를 더해
만들 수 있는 모든 조합에 대해 AI 모듈에 LSTM 모델을 각각 생성/학습/평가한다.
그 다음 AI 모듈이 돌려주는 accuracy(상대오차 20% 이내 비율)가 가장 높은 조합을 자동 선택한다.

[왜 서버 쪽인가]
AI 모듈(ai.py)은 "주어진 vector를 학습/예측"하는 역할만 한다(model_id 별로 독립 모델).
feature를 고르는 일은 control server의 책임이므로, AI 모듈은 건드리지 않고 이 스크립트가
ai.py의 공개 REST API(POST /<id>, PUT /<id>/training, POST /<id>/training,
PUT /<id>/testing, GET /<id>/result)만 사용한다. server.py의 통신 규약(이중 JSON 인코딩)을
그대로 따른다.

[사용법]
  # 1) 먼저 edge를 FEATURE_VERSION 4로 빌드해 데이터 수집:
  #    (server)  python server.py --algorithm lstm --dimension 16 --index 0 \
  #                 --caddr 127.0.0.1 --cport 5556 --lport 5555 --name auto \
  #                 --ntrain 365 --ntest 365 --collect auto
  #    (edge)    ./edge --addr <server IP> --port 5555
  #    -> auto_train.json / auto_test.json 생성
  #
  # 2) AI 모듈(ai.py)을 켠 상태에서 이 스크립트 실행:
  #    python auto_feature_selector.py --caddr 127.0.0.1 --cport 5556 \
  #                 --train auto_train.json --test auto_test.json --name autosel --total 3
"""

import argparse
import json
import logging
import itertools
import requests

# edge/process_manager.cpp 의 v4 superset 순서와 반드시 일치해야 한다 (인덱스 0-15).
FEATURE_NAMES = [
    "avg_power",       # 0  (예측 target)
    "min_power",       # 1
    "max_power",       # 2
    "power_range",     # 3
    "avg_temp_x10",    # 4
    "min_temp_x10",    # 5
    "max_temp_x10",    # 6
    "temp_range_x10",  # 7
    "avg_humid_x10",   # 8
    "min_humid_x10",   # 9
    "max_humid_x10",   # 10
    "humid_range_x10", # 11
    "month",           # 12
    "season",          # 13
    "heating_degree_x10",  # 14
    "cooling_degree_x10",  # 15
]

TARGET_ID = 0   # avg_power 를 예측 target 으로 고정. 투영 시 항상 맨 앞(index 0)에 둔다.

# 후보 기본값: target(0)을 제외하고, 전력 예측에 의미 있을 만한 feature 위주로 추린 집합.
# (전수 C(15,2)=105개는 너무 많아 기본은 10개 -> C(10,2)=45개. --candidates 로 바꿀 수 있음)
DEFAULT_CANDIDATES = [2, 3, 4, 6, 7, 8, 11, 12, 14, 15]


def _post_json(url, payload):
    # ai.py 가 본문을 json.loads(request.get_json(force=True)) 로 이중 디코딩하므로,
    # dict 를 json.dumps 로 한 번 직렬화한 문자열을 보낸다 (server.py 와 동일 규약).
    r = requests.post(url, json=json.dumps(payload))
    return json.loads(r.content)


def _post(url):
    r = requests.post(url)
    return json.loads(r.content)


def _put_json(url, payload):
    r = requests.put(url, json=json.dumps(payload))
    return r.json()


def _get(url):
    r = requests.get(url)
    return json.loads(r.content)


def project(row, ids):
    """superset row 에서 ids 위치의 값만 뽑아 새 vector 를 만든다."""
    return [row[i] for i in ids]


class AutoFeatureSelector:
    def __init__(self, caddr, cport, algorithm, name_prefix, total_features, candidates):
        self.base = "http://{}:{}".format(caddr, cport)
        self.algorithm = algorithm
        self.name_prefix = name_prefix
        self.total = total_features
        self.candidates = candidates

    def combinations(self):
        """target(0) 고정 + 후보 중 (total-1)개를 고르는 모든 조합."""
        combos = []
        for aux in itertools.combinations(self.candidates, self.total - 1):
            combos.append((TARGET_ID,) + aux)
        return combos

    def evaluate_one(self, feature_ids, train_rows, test_rows):
        ids = list(feature_ids)
        dim = len(ids)
        model_id = "{}_{}".format(self.name_prefix, "_".join(str(i) for i in ids))

        # 1) 모델 생성 (index=0: 투영 vector 의 맨 앞이 target)
        resp = _post_json("{}/{}".format(self.base, model_id),
                          {"algorithm": self.algorithm, "dimension": dim, "index": 0})
        if resp.get("opcode") != "success":
            raise RuntimeError("create_model failed: {}".format(resp))

        # 2) training 데이터 투입
        url_tr = "{}/{}/training".format(self.base, model_id)
        for row in train_rows:
            r = _put_json(url_tr, {"value": project(row, ids)})
            if r.get("opcode") != "success":
                raise RuntimeError("put_training failed: {}".format(r))

        # 3) 학습
        r = _post(url_tr)
        if r.get("opcode") != "success":
            raise RuntimeError("train failed: {}".format(r))

        # 4) testing 데이터 투입 (각 PUT 이 prediction 수행)
        url_te = "{}/{}/testing".format(self.base, model_id)
        for row in test_rows:
            r = _put_json(url_te, {"value": project(row, ids)})
            if r.get("opcode") != "success":
                raise RuntimeError("put_testing failed: {}".format(r))

        # 5) 결과 (accuracy 는 문자열로 옴)
        res = _get("{}/{}/result".format(self.base, model_id))
        if res.get("opcode") != "success":
            raise RuntimeError("get_result failed: {}".format(res))

        return {
            "model_id": model_id,
            "feature_ids": ids,
            "feature_names": [FEATURE_NAMES[i] for i in ids],
            "dimension": dim,
            "accuracy": float(res.get("accuracy", 0.0)),
            "correct": int(res.get("correct", 0)),
            "incorrect": int(res.get("incorrect", 0)),
            "num": int(res.get("num", 0)),
        }

    def select(self, train_rows, test_rows):
        combos = self.combinations()
        logging.info("[*] 평가할 조합 수: %d (total=%d, 후보=%s)",
                     len(combos), self.total, self.candidates)
        results = []
        for k, combo in enumerate(combos, 1):
            names = [FEATURE_NAMES[i] for i in combo]
            logging.info("[*] (%d/%d) 평가 중: %s", k, len(combos), names)
            try:
                results.append(self.evaluate_one(combo, train_rows, test_rows))
            except Exception as e:
                logging.error("    -> 실패, 건너뜀: %s", e)
        results.sort(key=lambda r: r["accuracy"], reverse=True)
        return results


def main():
    ap = argparse.ArgumentParser(description="Accuracy-based automatic feature selection")
    ap.add_argument("--caddr", required=True, help="AI module IP")
    ap.add_argument("--cport", required=True, type=int, help="AI module port")
    ap.add_argument("--train", required=True, help="<prefix>_train.json (superset rows)")
    ap.add_argument("--test", required=True, help="<prefix>_test.json (superset rows)")
    ap.add_argument("--algorithm", default="lstm")
    ap.add_argument("--name", default="autosel", help="모델 id 접두사")
    ap.add_argument("--total", type=int, default=3, help="최종 feature 개수(target 포함). 기본 3.")
    ap.add_argument("--candidates", default=None,
                    help="후보 feature id 쉼표 목록 (예: 2,4,12). 미지정 시 기본 집합.")
    ap.add_argument("--out", default="auto_feature_result.json")
    ap.add_argument("--log", default="INFO")
    args = ap.parse_args()
    logging.basicConfig(level=args.log, format="%(message)s")

    with open(args.train) as f:
        train_rows = json.load(f)
    with open(args.test) as f:
        test_rows = json.load(f)
    logging.info("[*] train rows=%d, test rows=%d, feature dim=%d",
                 len(train_rows), len(test_rows), len(train_rows[0]) if train_rows else 0)

    if args.candidates:
        candidates = [int(x) for x in args.candidates.split(",") if x.strip() != ""]
    else:
        candidates = DEFAULT_CANDIDATES

    sel = AutoFeatureSelector(args.caddr, args.cport, args.algorithm,
                              args.name, args.total, candidates)
    results = sel.select(train_rows, test_rows)

    if not results:
        logging.error("[*] 평가된 조합이 없습니다. (AI 모듈이 켜져 있는지 확인)")
        return

    best = results[0]
    report = {"best": best, "all_results": results}
    with open(args.out, "w") as f:
        json.dump(report, f, indent=2)

    # 보고서용 랭킹 표 출력
    print("\n================ Auto Feature Selection 결과 ================")
    print("Rank | Accuracy |  correct/num | Feature set")
    print("-----+----------+--------------+----------------------------------")
    for rank, r in enumerate(results, 1):
        print("{:>4} | {:>7.2f}% | {:>5}/{:<5} | {}".format(
            rank, r["accuracy"], r["correct"], r["num"], ", ".join(r["feature_names"])))
    print("-----------------------------------------------------------------")
    print("BEST: {}  (accuracy={:.2f}%, model_id={})".format(
        best["feature_names"], best["accuracy"], best["model_id"]))
    print("상세 결과 저장: {}".format(args.out))


if __name__ == "__main__":
    main()
