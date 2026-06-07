#ifndef __HUMIDITY_DATA_H__
#define __HUMIDITY_DATA_H__

#include "weather_data.h"

class HumidityData : public WeatherData
{
  private:
    HumidityData *next;

  public:
    HumidityData(time_t timestamp, double min, double max, double avg);

    void setNext(HumidityData *data);
    HumidityData *getNext();
};

#endif /* __HUMIDITY_DATA_H__ */
