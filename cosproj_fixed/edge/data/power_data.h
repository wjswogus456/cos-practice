#ifndef __POWER_DATA_H__
#define __POWER_DATA_H__

#include "sensor_data.h"

class PowerData : public SensorData
{
  private:
    PowerData *next;

  public:
    PowerData(time_t timestamp, double avg);

    void setNext(PowerData *data);
    PowerData *getNext();
};

#endif /* __POWER_DATA_H__ */
