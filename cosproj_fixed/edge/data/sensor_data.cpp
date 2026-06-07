#include "sensor_data.h"

SensorData::SensorData(time_t timestamp, double avg, string unit)
{
  this->timestamp = timestamp;
  this->avg = avg;
  this->unit = unit;
}

void SensorData::setValue(double value)
{
  this->avg = value;
}

double SensorData::getValue()
{
  return this->avg;
}

void SensorData::setTimestamp(time_t timestamp)
{
  this->timestamp = timestamp;
}

time_t SensorData::getTimestamp()
{
  return this->timestamp;
}

string SensorData::getUnit()
{
  return this->unit;
}
