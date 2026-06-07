#include "process_manager.h"
#include "opcode.h"
#include "byte_op.h"
#include "setting.h"
#include <cstring>
#include <iostream>
#include <ctime>
using namespace std;

ProcessManager::ProcessManager()
{
  this->num = 0;
}

void ProcessManager::init()
{
}

#define FEATURE_VERSION 3

// Aggregates one daily DataSet into a versioned feature vector for the AI model.
uint8_t *ProcessManager::processData(DataSet *ds, int *dlen)
{
  // ret: 버퍼 시작 주소(고정). p: 쓰기 커서 (VAR_TO_MEM_* 매크로가 내부에서 p++ 수행).
  uint8_t *ret, *p;
  int num;
  HouseData *house;
  TemperatureData *tdata;
  HumidityData *hdata;
  PowerData *pdata;

  // 공통 데이터 소스
  tdata = ds->getTemperatureData();          // 일별 기온 (avg=getValue / min / max)
  hdata = ds->getHumidityData();             // 일별 습도 (avg=getValue / min / max)
  num   = ds->getNumHouseData();             // 가구 수 (전력 루프 상한)

  // 타임스탬프 -> month
  time_t ts    = ds->getTimestamp();
  struct tm *t = localtime(&ts);
  int month    = t->tm_mon + 1;              // tm_mon: 0-based -> 1~12

  // 전력: PowerData는 가구 단위라 루프 돌려 avg/min/max 직접 계산
  double sum_power = 0.0;
  double max_power_d = -1e9;
  double min_power_d =  1e9;
  for (int i = 0; i < num; i++) {
    house = ds->getHouseData(i);
    double pw = house->getPowerData()->getValue();
    sum_power += pw;
    if (pw > max_power_d) max_power_d = pw;
    if (pw < min_power_d) min_power_d = pw;
  }
  // SPEC: 예측 대상(index 0 = power)은 스케일링하지 않는다
  int avg_power   = (int)(sum_power / num);
  int max_power   = (int)max_power_d;
  int min_power   = (int)min_power_d;
  int power_range = max_power - min_power;    // v3 전용

  // 온도/습도: SPEC대로 x10 후 int 저장 (-3.5 -> -35)
  int avg_temp    = (int)(tdata->getValue() * 10);
  int temp_range  = (int)(tdata->getMax() * 10) - (int)(tdata->getMin() * 10);  // v3
  int avg_humid   = (int)(hdata->getValue() * 10);
  int humid_range = (int)(hdata->getMax() * 10) - (int)(hdata->getMin() * 10);  // v3

  // season_code (v3): 3~5봄(0) 6~8여름(1) 9~11가을(2) 12/1/2겨울(3)
  int season_code;
  if      (month >= 3  && month <= 5)  season_code = 0;
  else if (month >= 6  && month <= 8)  season_code = 1;
  else if (month >= 9  && month <= 11) season_code = 2;
  else                                 season_code = 3;

  // 버퍼 초기화
  ret = (uint8_t *)malloc(BUFLEN);
  memset(ret, 0, BUFLEN);
  p = ret;

  // VERSION (1 byte). 앞의 OPCODE_DATA(1byte)는 sendData가 붙이므로 여기는 VERSION부터.
  VAR_TO_MEM_1BYTE_BIG_ENDIAN(FEATURE_VERSION, p);

  // 버전별 피처 (각 4 bytes, signed big-endian)
#if FEATURE_VERSION == 1
  // v1: dim=3, [avg_power, max_power, month]
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(avg_power, p);
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(max_power, p);
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(month,     p);
#elif FEATURE_VERSION == 2
  // v2: dim=5, [avg_power, max_power, avg_temp, avg_humid, month]
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(avg_power,  p);
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(max_power,  p);
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(avg_temp,   p);   // x10
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(avg_humid,  p);   // x10
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(month,      p);
#elif FEATURE_VERSION == 3
  // v3: dim=10
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(avg_power,   p);
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(min_power,   p);
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(max_power,   p);
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(power_range, p);
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(avg_temp,    p);  // x10
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(temp_range,  p);  // x10
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(avg_humid,   p);  // x10
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(humid_range, p);  // x10
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(month,       p);
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(season_code, p);
#elif FEATURE_VERSION == 4
  // v4: dim=16, "feature superset" — 자동 feature 선택(AutoFeatureSelector)용.
  // 서버는 이 16개를 그대로 수집(저장)만 하고, 오프라인 선택기가 부분조합을 골라 평가한다.
  // 순서(0-15)는 server/auto_feature_selector.py 의 FEATURE_NAMES 와 반드시 일치해야 한다.
  int min_temp   = (int)(tdata->getMin() * 10);   // x10
  int max_temp   = (int)(tdata->getMax() * 10);   // x10
  int min_humid  = (int)(hdata->getMin() * 10);   // x10
  int max_humid  = (int)(hdata->getMax() * 10);   // x10
  double atr     = tdata->getValue();             // 평균기온 실수값
  // 난방도일/냉방도일: 전력 수요와 직접 연관되는 파생 feature (x10)
  int heating_degree = (int)((atr < 18.0 ? (18.0 - atr) : 0.0) * 10);
  int cooling_degree = (int)((atr > 24.0 ? (atr - 24.0) : 0.0) * 10);

  VAR_TO_MEM_4BYTES_BIG_ENDIAN(avg_power,      p);  // 0
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(min_power,      p);  // 1
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(max_power,      p);  // 2
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(power_range,    p);  // 3
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(avg_temp,       p);  // 4  x10
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(min_temp,       p);  // 5  x10
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(max_temp,       p);  // 6  x10
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(temp_range,     p);  // 7  x10
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(avg_humid,      p);  // 8  x10
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(min_humid,      p);  // 9  x10
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(max_humid,      p);  // 10 x10
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(humid_range,    p);  // 11 x10
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(month,          p);  // 12
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(season_code,    p);  // 13
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(heating_degree, p);  // 14 x10
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(cooling_degree, p);  // 15 x10
#elif FEATURE_VERSION == 5
  // v5: dim=3, [avg_power, avg_temp(정수, ×10 안 함), month]
  // 작은 스케일 유지 -> 전력이 loss를 지배 -> 전력 정확도↑
  int avg_temp_plain = (int)tdata->getValue();
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(avg_power,      p);
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(avg_temp_plain, p);
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(month,          p);
#else
  #error "FEATURE_VERSION must be 1, 2, 3, 4, or 5"
#endif

  // dlen = 이동 거리 = VERSION(1) + VALUES(4*dim)
  *dlen = p - ret;

  return ret;
}
