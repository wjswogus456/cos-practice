#include "power_data.h"

PowerData::PowerData(time_t timestamp, double avg)
  : SensorData(timestamp, avg, "kWh")
{
  this->next = NULL;
}

void PowerData::setNext(PowerData *next)
{
  this->next = next;
}

PowerData *PowerData::getNext()
{
  return this->next;
}
