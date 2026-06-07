#include "weather_data.h"

WeatherData::WeatherData(time_t timestamp, double min, double max, double avg, string unit)
  : SensorData(timestamp, avg, unit)
{
  this->min = min;
  this->max = max;
}

void WeatherData::setMin(double min)
{
  this->min = min;
}

double WeatherData::getMin()
{
  return this->min;
}

void WeatherData::setMax(double max)
{
  this->max = max;
}

double WeatherData::getMax()
{
  return this->max;
}
