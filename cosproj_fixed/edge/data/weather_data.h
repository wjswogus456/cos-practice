#ifndef __WEATHER_DATA_H__
#define __WEATHER_DATA_H__

#include "sensor_data.h"

class WeatherData : public SensorData
{
  protected:
    double min;
    double max;

  public:
    WeatherData(time_t timestamp, double min, double max, double avg, string unit);

    void setMin(double min);
    double getMin();

    void setMax(double max);
    double getMax();
};

#endif /* __WEATHER_DATA_H__ */
