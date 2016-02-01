/*
 * gps_l80_mikro.c
 *
 *  Created on: Jul 1, 2015
 *      Author: Aidan
 */

#include "console.h"
#include "gps_l80_mikro.h"
#include "mikro.h"
#include "task_manager.h"

//buffer that holds all received characters until newline
//should be private
#define NMEA_RECORD_LENGTH 256
static char buffer[NMEA_RECORD_LENGTH];
static uint8_t buffer_size;

//record strings
volatile static char nmea_record_strings[NMEA_TYPE_COUNT][NMEA_RECORD_LENGTH];
volatile static uint32_t nmea_record_dirty_strings;

//last read records
volatile NMEA_RECORD_GPRMC gprmc_record;

//private procedures
void gps_l80_mikro_interrupt_handler();
void gps_l80_mikro_records_update();

void gps_l80_mikro_init() {

  //initialize the records
  memset((void*)&gprmc_record, 0, sizeof(gprmc_record));

  //initialize the record strings
  nmea_record_dirty_strings = 0;

  //initialize the buffer
  buffer_size = 0;

  //initialize the mikro bus
  mikro_enable_reset(DEV2);
  mikro_enable_uart(DEV2, 9600);
  mikro_enable_interrupt(DEV2, &gps_l80_mikro_interrupt_handler);
  mikro_reset(DEV2);
}

void gps_parse_buffer() {
  gps_l80_mikro_records_update();
//  uint32_t lat;
//  uint32_t lon;
//  NMEA_STATUS ret = NMEA_VAL_OK;
//  ret |= gps_l80_get_latitude(&lat);
//  ret |= gps_l80_get_longitude(&lon);
//  if(ret == NMEA_VAL_INVALID) {
//    con_println("Error retrieving GPS location");
//  }
//  else {
//    char str[256];
//    sprintf(str, "lat: %d, lon: %d", lat, lon);
//    con_println(str);
//  }
}

bool gps_l80_get_latitude(uint32_t *lat) {
  gps_l80_mikro_records_update();

  if((gprmc_record.valid_values & bitset(GPRMC_VAL_LATITUDE)) == 0) {
    return NMEA_VAL_INVALID;
  }

  *lat = gprmc_record.latitude;
  return NMEA_VAL_OK;
}

bool gps_l80_get_longitude(uint32_t *lon) {
  gps_l80_mikro_records_update();

  if((gprmc_record.valid_values & bitset(GPRMC_VAL_LONGITUDE)) == 0) {
    return NMEA_VAL_INVALID;
  }

  *lon = gprmc_record.longitude;
  return NMEA_VAL_OK;
}


bool gps_l80_get_time_stamp(uint32_t *time_stamp) {
  gps_l80_mikro_records_update();

  if((gprmc_record.valid_values & bitset(GPRMC_VAL_TIME_STAMP)) == 0) {
    return NMEA_VAL_INVALID;
  }

  *time_stamp = gprmc_record.time_stamp;
  return NMEA_VAL_OK;
}

void gps_l80_mikro_records_update() {

  //interate of every dirty record string
  uint32_t type;
  for(type = 0; type < NMEA_TYPE_COUNT; type++) {
    if((nmea_record_dirty_strings & bitset(type)) != 0) {

      //set up the parsing
      NMEA_PARSE_STATE  state = NMEA_FIELD_VAL;
      uint8_t   val_state = 0;

      //parse every field in the record string
      uint16_t ptr = 0;
      char record_str[NMEA_RECORD_LENGTH];
      strcpy(record_str, (char*)nmea_record_strings[type]);
      ptr += 7;
      char *field = xstrtok(record_str+ptr, ",");
      ptr += strlen(field) + 1;
      while(strlen(record_str+ptr) > 0) {

        switch(state) {

          //process the values within the record now that we know the type
          case NMEA_FIELD_VAL: {
            switch(type) {

              //GPRMC record
              case NMEA_TYPE_GPRMC: {
                switch(val_state) {
                  case GPRMC_VAL_TIME_STAMP: {
                    if(strlen(field) > 0) {
                      char hour[3] = {0};
                      char min[3] = {0};
                      char sec[3] = {0};

                      strncpy(hour, field, 2);
                      strncpy(min, field+2, 2);
                      strncpy(sec, field+4, 2);

                      uint32_t seconds = strtod(hour, NULL) * 3600;
                      seconds += strtod(min, NULL) * 60;
                      seconds += strtod(sec, NULL);

                      gprmc_record.time_stamp = seconds;
                      gprmc_record.valid_values |= bitset(GPRMC_VAL_TIME_STAMP);
                    }
                    else {
                      gprmc_record.valid_values &= bitclear(GPRMC_VAL_TIME_STAMP);
                    }

                    val_state++;
                    break;
                  }
                  case GPRMC_VAL_VALIDITY: {
                    if(strlen(field) > 0) {
                      gprmc_record.validity = field[0];
                      gprmc_record.valid_values |= bitset(GPRMC_VAL_VALIDITY);
                    }
                    else {
                      gprmc_record.valid_values &= bitclear(GPRMC_VAL_VALIDITY);
                    }

                    val_state++;
                    break;
                  }
                  case GPRMC_VAL_LATITUDE: {
                    if(strlen(field) > 0) {
                      char deg_str[3] = {0};
                      char min_str[8] = {0};

                      strncpy(deg_str, field, 2);
                      field += strlen(deg_str);
                      strcpy(min_str, field);
                      field += strlen(min_str);

                      uint32_t deg = (strtod(deg_str, NULL) * 1000000);
                      uint32_t min = (strtod(min_str, NULL) * 1000000) / 60;

                      deg += min;
                      gprmc_record.latitude = deg;
                      gprmc_record.valid_values |= bitset(GPRMC_VAL_LATITUDE);
                    }
                    else {
                      gprmc_record.valid_values &= bitclear(GPRMC_VAL_LATITUDE);
                    }

                    val_state++;
                    break;
                  }
                  case GPRMC_VAL_LATITUDE_DIRECTION: {
                    if(strlen(field) > 0) {
                      gprmc_record.latitude_direction = field[0];
                      gprmc_record.valid_values |= bitset(GPRMC_VAL_LATITUDE_DIRECTION);
                    }
                    else {
                      gprmc_record.valid_values &= bitclear(GPRMC_VAL_LATITUDE_DIRECTION);
                    }

                    val_state++;
                    break;
                  }
                  case GPRMC_VAL_LONGITUDE: {
                    if(strlen(field) > 0) {
                      char deg_str[3] = {0};
                      char min_str[8] = {0};

                      strncpy(deg_str, field, 3);
                      field += strlen(deg_str);
                      strcpy(min_str, field);
                      field += strlen(min_str);

                      uint32_t deg = (strtod(deg_str, NULL) * 1000000);
                      uint32_t min = (strtod(min_str, NULL) * 1000000) / 60;

                      deg += min;
                      gprmc_record.longitude = deg;
                      gprmc_record.valid_values |= bitset(GPRMC_VAL_LONGITUDE);
                    }
                    else {
                      gprmc_record.valid_values &= bitclear(GPRMC_VAL_LONGITUDE);
                    }

                    val_state++;
                    break;
                  }
                  case GPRMC_VAL_LONGITUDE_DIRECTION: {
                    if(strlen(field) > 0) {
                      gprmc_record.longitude_direction = field[0];
                      gprmc_record.valid_values |= bitset(GPRMC_VAL_LONGITUDE_DIRECTION);
                    }
                    else {
                      gprmc_record.valid_values &= bitclear(GPRMC_VAL_LONGITUDE_DIRECTION);
                    }

                    val_state++;
                    break;
                  }
                  case GPRMC_VAL_SPEED: {
                    if(strlen(field) > 0) {
                      gprmc_record.speed = strtod(field, NULL) * 1000;
                      gprmc_record.valid_values |= bitset(GPRMC_VAL_LONGITUDE);
                    }
                    else {
                      gprmc_record.valid_values &= bitclear(GPRMC_VAL_LONGITUDE);
                    }

                    val_state++;
                    break;
                  }
                  case GPRMC_VAL_TRUE_COURSE: {
                    if(strlen(field) > 0) {
                      gprmc_record.true_course = strtod(field, NULL) * 1000;
                      gprmc_record.valid_values |= bitset(GPRMC_VAL_TRUE_COURSE);
                    }
                    else {
                      gprmc_record.valid_values &= bitclear(GPRMC_VAL_TRUE_COURSE);
                    }
                    val_state++;
                    break;
                  }
                  case GPRMC_VAL_DATE_STAMP: {
                    if(strlen(field) > 0) {
                      char year[3] = {0};
                      char month[3] = {0};
                      char day[3] = {0};

                      strncpy(year, field, 2);
                      strncpy(month, field+2, 2);
                      strncpy(day, field+4, 2);

                      gprmc_record.date_stamp.year = strtod(year, NULL);
                      gprmc_record.date_stamp.month = strtod(month, NULL);
                      gprmc_record.date_stamp.day = strtod(day, NULL);
                      gprmc_record.valid_values |= bitset(GPRMC_VAL_DATE_STAMP);
                    }
                    else {
                      gprmc_record.valid_values &= bitclear(GPRMC_VAL_DATE_STAMP);
                    }

                    val_state++;
                    break;
                  }
                  case GPRMC_VAL_VARIATION: {
                    if(strlen(field) > 0) {
                      gprmc_record.variation = strtod(field, NULL) * 1000;
                      gprmc_record.valid_values |= bitset(GPRMC_VAL_VARIATION);
                    }
                    else {
                      gprmc_record.valid_values &= bitclear(GPRMC_VAL_VARIATION);
                    }
                    val_state++;
                    break;
                  }
                  case GPRMC_VAL_VARIATION_DIRECTION: {
                    if(strlen(field) > 0) {
                      gprmc_record.variation_direction = field[0];
                      gprmc_record.valid_values |= bitset(GPRMC_VAL_VARIATION_DIRECTION);
                    }
                    else {
                      gprmc_record.valid_values &= bitclear(GPRMC_VAL_VARIATION_DIRECTION);
                    }
                    val_state++;
                    //TODO add verification to the checksum
                    break;
                  }
                  default: {
                    break;
                  }

                } //switch(val_state)
                break;

              } //case NMEA_TYPE_GPRMC

              //state not implemented so end processing
              default: {
                state = NMEA_FIELD_END;
                break;
              }

            } //switch(type)

            break;

          } //case NMEA_FIELD_VAL

          //unknown state so end processing
          default: {
            ptr += strlen(record_str);
            break;
          }
        } //switch(type)

        if(state == NMEA_FIELD_END) {
          break;
        }

        field = xstrtok(record_str+ptr, ",");
        ptr += strlen(field) + 1;
      } //for every field

      //clear the dirty bit
      nmea_record_dirty_strings &= bitclear(type);
    }
  }
}

//receive character from the uart and process
void gps_l80_mikro_interrupt_handler() {

  //clear interrupt
  uint32_t ui32Status;
  ui32Status = ROM_UARTIntStatus(UART0_BASE, true);
  ROM_UARTIntClear(UART1_BASE, ui32Status);

  //retrieve character
  int32_t c = UARTCharGetNonBlocking(UART1_BASE);

  //if a newline, copy the buffer to the corresponding record string
  if(c == '\n') {

    //make the buffer to make it a string
    buffer[buffer_size] = '\0';

    //move to the first $
    uint8_t ptr = 0;
    while(buffer[ptr] != '$' && ptr < buffer_size) {
      ptr++;
    }
    if(ptr >= buffer_size) {
      return;
    }

    //determine the record type
    NMEA_TYPE type = NMEA_TYPE_UNKNOWN;
    if(strncmp(buffer+ptr, "$GPBOD", 6) == 0) {
      type = NMEA_TYPE_GPBOD;
    }
    else if(strncmp(buffer+ptr, "$GPBWC", 6) == 0) {
      type = NMEA_TYPE_GPBWC;
    }
    else if(strncmp(buffer+ptr, "$GPGGA", 6) == 0) {
      type = NMEA_TYPE_GPGGA;
    }
    else if(strncmp(buffer+ptr, "$GPGLL", 6) == 0) {
      type = NMEA_TYPE_GPGLL;
    }
    else if(strncmp(buffer+ptr, "$GPGSA", 6) == 0) {
      type = NMEA_TYPE_GPGSA;
    }
    else if(strncmp(buffer+ptr, "$GPHDT", 6) == 0) {
      type = NMEA_TYPE_GPHDT;
    }
    else if(strncmp(buffer+ptr, "$GPR00", 6) == 0) {
      type = NMEA_TYPE_GPR00;
    }
    else if(strncmp(buffer+ptr, "$GPRMA", 6) == 0) {
      type = NMEA_TYPE_GPRMA;
    }
    else if(strncmp(buffer+ptr, "$GPRMB", 6) == 0) {
      type = NMEA_TYPE_GPRMB;
    }
    else if(strncmp(buffer+ptr, "$GPRMC", 6) == 0) {
      type = NMEA_TYPE_GPRMC;
    }
    else if(strncmp(buffer+ptr, "$GPRTE", 6) == 0) {
      type = NMEA_TYPE_GPRTE;
    }
    else if(strncmp(buffer+ptr, "$GPTRF", 6) == 0) {
      type = NMEA_TYPE_GPTRF;
    }
    else if(strncmp(buffer+ptr, "$GPSTN", 6) == 0) {
      type = NMEA_TYPE_GPSTN;
    }
    else if(strncmp(buffer+ptr, "$GPVBW", 6) == 0) {
      type = NMEA_TYPE_GPVBW;
    }
    else if(strncmp(buffer+ptr, "$GPVTG", 6) == 0) {
      type = NMEA_TYPE_GPVTG;
    }
    else if(strncmp(buffer+ptr, "$GPWPL", 6) == 0) {
      type = NMEA_TYPE_GPWPL;
    }
    else if(strncmp(buffer+ptr, "$GPXTE", 6) == 0) {
      type = NMEA_TYPE_GPXTE;
    }
    else if(strncmp(buffer+ptr, "$GPZDA", 6) == 0) {
      type = NMEA_TYPE_GPZDA;
    }
    else {
      type = NMEA_TYPE_UNKNOWN;
    }

    //copy the record into the record string buffer
    if(type != NMEA_TYPE_UNKNOWN) {
      strcpy((char*)nmea_record_strings[type], buffer+ptr);
      nmea_record_dirty_strings |= bitset(type);
      task_set_event(TASK_EVENT_GPS);
    }

    //reset the buffer
    buffer_size = 0;
  }

  //otherwise, append it to the buffer
  else {

    //ensure that we do not overflow the buffer
    if(buffer_size < NMEA_RECORD_LENGTH-1) {
      buffer[buffer_size++] = c;
    }
  }
}
