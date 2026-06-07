#include "humidity_data.h"

HumidityData::HumidityData(time_t timestamp, double min, double max, double avg)
  : WeatherData(timestamp, min, max, avg, "%")
{
  this->next = NULL;
}

void HumidityData::setNext(HumidityData *next)
{
  this->next = next;
}

HumidityData *HumidityData::getNext()
{
  return this->next;
}
