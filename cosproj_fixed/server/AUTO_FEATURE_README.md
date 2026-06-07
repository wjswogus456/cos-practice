# 정확도 향상 / Feature 선택 워크플로우 (B + C)

## B. 3가지 aggregation function (루브릭 "≥3 types" 만점용)

`edge/process_manager.cpp` 맨 위 `#define FEATURE_VERSION` 값을 바꾸고 edge를 다시 빌드,
서버는 `--dimension`(과 필요 시 `--index 0`)을 맞춰서 세 번 돌린 뒤 정확도를 비교한다.

| ver | dim | feature 구성 | 보고서에 쓸 아이디어(요지) |
|----|----|----|----|
| 1 | 3 | avg_power, max_power, month | 전력은 계절성이 강함 → month로 난방/냉방 수요 주기를, max_power로 피크 부하를 포착 |
| 2 | 5 | avg_power, max_power, avg_temp, avg_humid, month | 기온·습도가 냉난방 부하를 직접 좌우 → 월 내 일별 변동을 설명 |
| 3 | 10 | avg_power, min/max/range_power, avg_temp, temp_range, avg_humid, humid_range, month, season | 분산(range)은 변동성, season은 month보다 큰 단위의 계절 레짐을 포착 |

실행 예 (v2):
```
# edge: #define FEATURE_VERSION 2 로 두고 make
python ai.py --port 5556
python server.py --algorithm lstm --dimension 5 --index 0 --caddr 127.0.0.1 --cport 5556 \
    --lport 5555 --name my_model --ntrain 365 --ntest 365
./edge --addr <노트북 IP> --port 5555
```
세 버전의 accuracy를 표로 정리 + 각 feature를 고른 이유 서술 = 루브릭 충족.

## C. 자동 feature 선택 (가산점: "additional feature 제안")

목적: 16개 후보 중 전력 예측에 가장 좋은 조합을 정확도 기준으로 자동 탐색.
AI 모듈(ai.py)은 안 건드리고, control server 쪽 오프라인 스크립트로 처리한다.

### 1) superset 데이터 수집 (edge를 v4로)
```
# edge: #define FEATURE_VERSION 4 로 두고 make
# 서버를 'collect 모드'로 (AI 모듈 불필요, caddr/cport는 더미값)
python server.py --algorithm lstm --dimension 16 --index 0 --caddr 127.0.0.1 --cport 9999 \
    --lport 5555 --name auto --ntrain 365 --ntest 365 --collect auto
./edge --addr <노트북 IP> --port 5555
# -> auto_train.json / auto_test.json 생성
```

### 2) 자동 선택 실행 (ai.py 켠 상태)
```
python ai.py --port 5556          # 다른 터미널에서 실행 중
python auto_feature_selector.py --caddr 127.0.0.1 --cport 5556 \
    --train auto_train.json --test auto_test.json --name autosel --total 3
```
- 모든 3-feature 조합(target=avg_power 고정 + 후보 2개)을 만들어 LSTM 각각 학습/평가.
- accuracy 내림차순 랭킹표 출력 + `auto_feature_result.json` 저장.
- 후보 줄이기/늘리기: `--candidates 2,4,12,...`, 개수 바꾸기: `--total 4`.
- LSTM을 수십 번 학습하므로 시간이 걸린다 → **오프라인 1회 실행**용. 데모는 B의 best 조합으로.

### feature 인덱스 맵 (edge v4 ↔ selector 동일 순서)
```
0 avg_power(target)  1 min_power     2 max_power       3 power_range
4 avg_temp_x10       5 min_temp_x10  6 max_temp_x10    7 temp_range_x10
8 avg_humid_x10      9 min_humid_x10 10 max_humid_x10  11 humid_range_x10
12 month             13 season       14 heating_degree_x10  15 cooling_degree_x10
```
(온도/습도/도일은 x10 정수로 인코딩됨. 정확도 기준은 target의 상대오차 ≤ 20%.)
