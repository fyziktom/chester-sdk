#ifndef APP_HANDLER_H_
#define APP_HANDLER_H_

/* CHESTER includes */
#include <chester/ctr_edge.h>
#include <chester/ctr_lte.h>
#include <chester/drivers/ctr_s1.h>
#include <chester/drivers/ctr_z.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_SHIELD_CTR_LTE)
void app_handler_lte(enum ctr_lte_event event, union ctr_lte_event_data *data, void *param);
#endif /* defined(CONFIG_SHIELD_CTR_LTE) */

#if defined(CONFIG_SHIELD_CTR_METEO_A) || defined(CONFIG_SHIELD_CTR_METEO_B)
void app_handler_edge_wind_speed_callback(struct ctr_edge *edge, enum ctr_edge_event edge_event,
					  void *user_data);
void app_handler_edge_rainfall_callback(struct ctr_edge *edge, enum ctr_edge_event edge_event,
					void *user_data);
#endif /* defined(CONFIG_SHIELD_CTR_METEO_A) || defined(CONFIG_SHIELD_CTR_METEO_B) */

#if defined(CONFIG_SHIELD_CTR_Z)
void app_handler_ctr_z(const struct device *dev, enum ctr_z_event event, void *param);
#endif /* defined(CONFIG_SHIELD_CTR_Z) */

#ifdef __cplusplus
}
#endif

#endif /* APP_HANDLER_H_ */