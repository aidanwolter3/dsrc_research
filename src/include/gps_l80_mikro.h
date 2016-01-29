/*
 * gps_l80_mikro.h
 *
 *  Created on: Jul 1, 2015
 *      Author: Aidan
 */

#include "common.h"

#ifndef GPS_L80_MIKRO_H_
#define GPS_L80_MIKRO_H_

//procedures
void gps_l80_mikro_init();
void gps_parse_buffer();
bool gps_l80_get_latitude(uint32_t *lat);
bool gps_l80_get_longitude(uint32_t *lon);
bool gps_l80_get_time_stamp(uint32_t *time_stamp);

//nmea record types
typedef enum {
  NMEA_TYPE_GPBOD = 0,
  NMEA_TYPE_GPBWC,
  NMEA_TYPE_GPGGA,
  NMEA_TYPE_GPGLL,
  NMEA_TYPE_GPGSA,
  NMEA_TYPE_GPGSV,
  NMEA_TYPE_GPHDT,
  NMEA_TYPE_GPR00,
  NMEA_TYPE_GPRMA,
  NMEA_TYPE_GPRMB,
  NMEA_TYPE_GPRMC,
  NMEA_TYPE_GPRTE,
  NMEA_TYPE_GPTRF,
  NMEA_TYPE_GPSTN,
  NMEA_TYPE_GPVBW,
  NMEA_TYPE_GPVTG,
  NMEA_TYPE_GPWPL,
  NMEA_TYPE_GPXTE,
  NMEA_TYPE_GPZDA,
  NMEA_TYPE_COUNT,
  NMEA_TYPE_UNKNOWN,
} NMEA_TYPE;

//a nmea date
typedef struct {
  uint8_t year;
  uint8_t month;
  uint8_t day;
} NMEA_DATE;

typedef enum {
  NMEA_VAL_OK = 0,
  NMEA_VAL_INVALID
} NMEA_STATUS;

//gprmc record structure
typedef struct {
  uint32_t  time_stamp;       //time in seconds
  char    validity;       //A = OK, V = warning
  uint32_t  latitude;       //latitude in degrees * 1000000
  char    latitude_direction;   //N or S
  uint32_t  longitude;        //longitude in degrees * 1000000
  char    longitude_direction;  //E or W
  uint32_t  speed;          //Knots * 1000
  uint32_t  true_course;      //true course * 1000
  NMEA_DATE   date_stamp;
  uint32_t  variation;        //variation * 1000
  char    variation_direction;  //E or W
  uint16_t  valid_values;     //each bit indicates whether a value has been read or not
} NMEA_RECORD_GPRMC;

//gprmc values for parsing and checking validity
typedef enum {
  GPRMC_VAL_TIME_STAMP = 0,
  GPRMC_VAL_VALIDITY,
  GPRMC_VAL_LATITUDE,
  GPRMC_VAL_LATITUDE_DIRECTION,
  GPRMC_VAL_LONGITUDE,
  GPRMC_VAL_LONGITUDE_DIRECTION,
  GPRMC_VAL_SPEED,
  GPRMC_VAL_TRUE_COURSE,
  GPRMC_VAL_DATE_STAMP,
  GPRMC_VAL_VARIATION,
  GPRMC_VAL_VARIATION_DIRECTION,
  GPRMC_VAL_END
} NMEA_RECORD_GPRMC_VAL;

//states for parsing the NMEA records
typedef enum {
  NMEA_FIELD_TYPE = 0,
  NMEA_FIELD_VAL,
  NMEA_FIELD_CHKSUM,
  NMEA_FIELD_END
} NMEA_PARSE_STATE;

#endif //GPS_L80_MIKRO_H_
