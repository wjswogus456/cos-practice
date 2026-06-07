#ifndef __TEMPERATURE_H__
#define __TEMPERATURE_H__

#include "weather_data.h"

class TemperatureData : public WeatherData
{
  private:
    TemperatureData *next;

  public:
    TemperatureData(time_t timestamp, double min, double max, double avg);

    void setNext(TemperatureData *data);
    TemperatureData *getNext();
};

#endif /* __TEMPERATURE_H__ */
