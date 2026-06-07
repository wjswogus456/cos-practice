#include "../edge/setting.h"
#include "../edge/edge.h"

#include <iostream>
#include <ctime>

#include "../edge/byte_op.h"

#define BUFLEN 1024

using namespace std;

// main(): DataSet 객체에서 각종 센서 데이터를 꺼내 읽는 방법을 연습하고,
//         마지막에 일부 값을 빅엔디안 바이트 버퍼에 인코딩해 출력하는 예제.
int main(int argc, char *argv[])
{
  // 사용할 객체/변수 선언
  DataReceiver *dr;                                   // 센서 데이터를 만들어 주는 생성기
  DataSet *ds;                                        // 특정 하루치 데이터 묶음
  HouseData *house;                                   // 가구 1채의 데이터
  TemperatureData *tdata;                             // 그날의 기온 데이터(일별 avg/min/max)
  HumidityData *hdata;                                // 그날의 습도 데이터
  PowerData *pdata;                                   // 가구 1채의 전력 데이터
  int num, tmp;                                       // num=가구 수, tmp=임시 정수
  time_t curr, ts;                                    // curr=조회할 시각, ts=읽어온 타임스탬프
  double max_temp, avg_temp, min_temp;               // 기온 통계
  double max_humid, avg_humid, min_humid;            // 습도 통계
  double power, sum_power, avg_power, max_power, min_power;  // 전력 통계
  unsigned char buf[BUFLEN];                          // 인코딩 결과를 담을 버퍼
  unsigned char *p;                                   // 버퍼 쓰기 커서

  curr = 1609459200;                                  // 2021-01-01 00:00:00 (Unix time)
  dr = new DataReceiver();                            // 생성기 인스턴스 생성
  ds = dr->getDataSet(curr);                          // 해당 날짜의 DataSet 획득
  
  // 1. Write a statement to get the timestamp value to 'ts' and print out the value (please refer to dataset.h)
  ts = ds->getTimestamp();                            // DataSet의 타임스탬프를 읽음
  cout << "timestamp: " << ts << endl;

  // 2. Write a statement to get the number of house data that contains the private information and the power value to 'num' (dataset.h)
  num = ds->getNumHouseData();                        // 이 DataSet에 들어있는 가구 수
  cout << "# of house data: " << num << endl;

  // 3. Write a statement to get the first house data to 'house' (please refer to dataset.h) 
  house = ds->getHouseData(0);                        // 0번째(첫) 가구
  
  // Write a statement to get the 10th house data to 'house' (dataset.h)
  house = ds->getHouseData(9);                        // 9번 인덱스 = 10번째 가구
  
  // Get the power data to 'pdata' (house_data.h)
  pdata = house->getPowerData();                      // 그 가구의 전력 데이터 객체
  
  // Get the daily power value to 'power' and print out the value (power_data.h)
  power = pdata->getValue();                          // 그 가구의 전력 값(double)
  cout << "Power: " << power << endl;
  
  // Explicitly cast the type from double to int and assign it to 'tmp', and print out the value
  tmp = (int)pdata->getValue();                       // double -> int 명시적 형변환(소수점 버림)
  cout << "Power (casted): " << tmp << endl;
  
  // Compute the value averaged over all the power data by using 'sum_power' and 'num', 
  // assign the average value to 'avg_power', and print out the value
  sum_power = 0;                                       // 합계 초기화
  for (int i=0; i<num; i++)                            // 모든 가구를 순회
  {
    house = ds->getHouseData(i);                       // i번째 가구
    pdata = house->getPowerData();                     // 그 가구 전력 데이터
    sum_power += pdata->getValue();                    // 전력 값을 누적
  }
  avg_power = sum_power / num;                          // 평균 = 합 / 가구 수
  cout << "Power (avg): " << avg_power << endl;
  
  // Find the maximum value among all the power data 
  max_power = -1;                                       // 충분히 작은 값으로 초기화
  for (int i=0; i<num; i++)
  {
    house = ds->getHouseData(i);
    pdata = house->getPowerData();
    power = pdata->getValue();

    if (power > max_power)                              // 더 크면 갱신
      max_power = power;
  }
  cout << "Power (max): " << max_power << endl;
  
  // Find the minimum value among all the power data
  min_power = 10000;                                   // 충분히 큰 값으로 초기화
  for (int i=0; i<num; i++)
  {
    house = ds->getHouseData(i);
    pdata = house->getPowerData();
    power = pdata->getValue();

    if (power < min_power)                             // 더 작으면 갱신
      min_power = power;
  }
  cout << "Power (min): " << min_power << endl;

  // 4. Write a statement to get the temperature data to 'tdata' (dataset.h)
  tdata = ds->getTemperatureData();                   // 그날의 기온 데이터 객체(일별 통계 제공)
  
  // Get the maximum value of the daily temperature (temperature_data.h)
  max_temp = tdata->getMax();                          // 일 최고 기온
  cout << "Temperature (max): " << max_temp << endl;
  
  // Get the average value of the daily temperature (temperature_data.h)
  avg_temp = tdata->getValue();                        // 일 평균 기온 (평균은 getValue)
  cout << "Temperature (avg): " << avg_temp << endl;
  
  // Get the minimum value of the daily temperature (temperature_data.h)
  min_temp = tdata->getMin();                          // 일 최저 기온
  cout << "Temperature (min): " << min_temp << endl;
  
  // Explicitly cast the type of the maximum value from double to int, assign the resultant value to 'tmp', and print it out
  tmp = (int)tdata->getMax();                          // 최고 기온을 int로 변환
  cout << "Temperature (max, casted): " << tmp << endl;

  // 5. Write a statement to get the humidity data to 'hdata' (dataset.h)
  hdata = ds->getHumidityData();                       // 그날의 습도 데이터 객체
  
  // Get the maximum value of the daily humidity (humidity_data.h)
  max_humid = hdata->getMax();                         // 일 최고 습도
  cout << "Humidity (max): " << max_humid << endl;
  
  // Get the average value of the daily humidity (humidity_data.h)
  avg_humid = hdata->getValue();                       // 일 평균 습도
  cout << "Humidity (avg): " << avg_humid << endl;
  
  // Get the minimum value of the daily humidity (humidity_data.h)
  min_humid = hdata->getMin();                         // 일 최저 습도
  cout << "Humidity (min): " << min_humid << endl;
  
  // Explicitly cast the type of the minimum value from double to int, assign the resultant value to 'tmp', and print it out
  tmp = (int)hdata->getMin();                          // 최저 습도를 int로 변환
  cout << "Humidity (min, casted): " << tmp << endl;

  // 6. Initialize the buffer 'buf' with zeros (its length is defined as BUFLEN) (use the memset() function, please google it!)
  memset(buf, 0, BUFLEN);                              // 버퍼 전체를 0으로 초기화

  // 7. Write statements to save the values into 'buf' using 'p' as follows:
  // # of house data (2 bytes) || maximum power (integer) (4 bytes) || maximum temperature (integer) (2 bytes)
  // Print out the buffer
  // Please use the macros defined in edge/byte_op.h
  p = buf;                                             // 커서를 버퍼 시작으로
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(num, p);               // 가구 수를 2바이트로 기록
  tmp = (int)min_power;                                // (예제) 전력 최소값을 int로
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(tmp, p);               // 4바이트로 기록
  tmp = (int)max_temp;                                 // 최고 기온을 int로
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(tmp, p);               // 4바이트로 기록

  tmp = p - buf;                                       // 실제로 기록한 바이트 수 = 커서 이동 거리
  PRINT_MEM(buf, tmp);                                 // 버퍼 내용을 16진수로 출력

	return 0;
}
