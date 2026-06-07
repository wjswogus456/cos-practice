#ifndef __SENSOR_DATA_H__
#define __SENSOR_DATA_H__

#include <ctime>
#include <string>
using namespace std;

class SensorData
{
  protected:
    time_t timestamp;
    double avg;
    string unit;

  public:
    SensorData(time_t timestamp, double avg, string unit);

    void setValue(double value);
    double getValue();

    void setTimestamp(time_t timestamp);
    time_t getTimestamp();

    string getUnit();
};

#endif /* __SENSOR_DATA_H__ */
