#ifndef APP_SENSOR_H_
#define APP_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

int app_sensor_sample(void);

int app_sensor_meteo_sample(void);
int app_sensor_meteo_aggreg(void);
int app_sensor_meteo_clear(void);

int app_sensor_barometer_sample(void);
int app_sensor_barometer_aggreg(void);
int app_sensor_barometer_clear(void);

int app_sensor_hygro_sample(void);
int app_sensor_hygro_aggreg(void);
int app_sensor_hygro_clear(void);

int app_sensor_w1_therm_sample(void);
int app_sensor_w1_therm_aggreg(void);
int app_sensor_w1_therm_clear(void);

int app_sensor_soil_sensor_sample(void);
int app_sensor_soil_sensor_aggreg(void);
int app_sensor_soil_sensor_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_SENSOR_H_ */
