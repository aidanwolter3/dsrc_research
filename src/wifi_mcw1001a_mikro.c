/*
 * wifi_mcw1001a_mikro.c
 *
 *  Created on: Jul 22, 2015
 *      Author: Aidan
 */

#include "console.h"
#include "wifi_mcw1001a_mikro.h"
#include "mikro.h"
#include "task_manager.h"

//buffer that holds all received characters until 0x45
//should be private

extern uint8_t socket_handle;

#define           WIFI_BUFFER_LENGTH  1024
#define           BUFFER_NEXT(c)    (((c+1) > WIFI_BUFFER_LENGTH) ? 0 : (c+1))
volatile static char*     buffer;
volatile static uint16_t  buffer_size;
volatile static uint16_t  buffer_start;
volatile static uint16_t  buffer_end;

static volatile bool recv_from_clear = true;

//private procedures
void            wifi_mcw1001a_mikro_interrupt_handler();
void            wifi_send_basic_packet(uint16_t packet_type, uint16_t response_type, uint16_t len, uint8_t *data);
PACKET_STATUS   wifi_process_packet(WIFI_PACKET *p);
void            wifi_put_packet(WIFI_PACKET *p);
WIFI_PACKET*    wifi_get_packet();
void            wifi_print_packet(WIFI_PACKET *p);
bool            wifi_wait_for_response(uint16_t packet_type);
bool            wifi_wait_for_event_response(uint16_t event_type);

//private variables
WIFI_PACKET_NETWORK_STATUS                wifi_network_status;
WIFI_PACKET_SOCKET_ACCEPT_RESPONSE        wifi_socket_accept_response;
WIFI_PACKET_SOCKET_ALLOCATE_RESPONSE      wifi_socket_allocate_response;
WIFI_PACKET_SOCKET_BIND_RESPONSE          wifi_socket_bind_response;
WIFI_PACKET_SOCKET_RECV_RESPONSE          wifi_socket_recv_response;
WIFI_PACKET_SOCKET_RECV_FROM_RESPONSE     wifi_socket_recv_from_response;
WIFI_PACKET_SOCKET_SEND_TO_RESPONSE       wifi_socket_send_to_response;
uint8_t                                   wifi_socket_handle;
uint8_t                                   wifi_socket_connect_result;

void wifi_init() {

  //initialize the buffer
  buffer_size = 0;
  buffer_start = 0;
  buffer_end = 0;
  buffer = malloc(WIFI_BUFFER_LENGTH*sizeof(uint8_t));

  //initialize the data
  memset(&wifi_network_status, 0, sizeof(WIFI_PACKET_NETWORK_STATUS));

  //initialize the mikro bus
  mikro_enable_reset();
  GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_PIN_5);
  mikro_enable_uart(115200);
  mikro_enable_interrupt(&wifi_mcw1001a_mikro_interrupt_handler);
}

void wifi_parse_buffer() {
  WIFI_PACKET *p = wifi_get_packet();
  while(p != NULL) {
    wifi_process_packet(p);
    free(p->data);
    free(p);
    p = wifi_get_packet();
  }
}

void wifi_reset() {
  WIFI_PACKET *p = wifi_get_packet();
  p->type = WIFI_PACKET_TYPE_RESET_MSG;
  p->len = 0;
  wifi_put_packet(p);
  free(p);
}

//network configuration
void wifi_set_ip_address(WIFI_IP_CONFIG ip_config, uint8_t *ip) {
  uint8_t *data = malloc(18*sizeof(uint8_t));
  data[1] = ip_config;
  memcpy(&data[2], ip, 16);
  wifi_send_basic_packet(WIFI_PACKET_TYPE_SET_IP_ADDRESS_MSG, WIFI_PACKET_TYPE_ACK, 18, data);
  free(data);
}

void wifi_set_netmask(uint8_t *netmask) {
  wifi_send_basic_packet(WIFI_PACKET_TYPE_SET_NETWORK_MASK_MSG, WIFI_PACKET_TYPE_ACK, 16, netmask);
}

void wifi_set_gateway(uint8_t *gateway) {
  wifi_send_basic_packet(WIFI_PACKET_TYPE_SET_GATEWAY_IP_ADDRESS_MSG, WIFI_PACKET_TYPE_ACK, 16, gateway);
}

void wifi_set_mac(uint8_t *mac) {
  wifi_send_basic_packet(WIFI_PACKET_TYPE_SET_MACADDRESS_MSG, WIFI_PACKET_TYPE_ACK, 6, mac);
}

void wifi_set_arp_time(uint16_t arp_time) {
  wifi_send_basic_packet(WIFI_PACKET_TYPE_SET_ARP_TIME_MSG, WIFI_PACKET_TYPE_ACK, 2, (uint8_t*)&arp_time);
}

WIFI_PACKET_NETWORK_STATUS wifi_get_network_status() {
  wifi_send_basic_packet(WIFI_PACKET_TYPE_GET_NETWORK_STATUS, WIFI_PACKET_TYPE_NETWORK_STATUS_RESPONSE_MSG, 0, NULL);
  return wifi_network_status;
}

void wifi_print_network_status() {
  char *status_str = malloc(256*sizeof(char));

  con_println("\nNetwork status:");
  sprintf(status_str, "    mac    : %d:%d:%d:%d:%d:%d",
                              wifi_network_status.mac[0],
                              wifi_network_status.mac[1],
                              wifi_network_status.mac[2],
                              wifi_network_status.mac[3],
                              wifi_network_status.mac[4],
                              wifi_network_status.mac[5]);
  con_println(status_str);
  sprintf(status_str, "    ip     : %d:%d:%d:%d",
                            wifi_network_status.ip[0],
                            wifi_network_status.ip[1],
                            wifi_network_status.ip[2],
                            wifi_network_status.ip[3]);
  con_println(status_str);
  sprintf(status_str, "    netmask: %d:%d:%d:%d",
                            wifi_network_status.netmask[0],
                            wifi_network_status.netmask[1],
                            wifi_network_status.netmask[2],
                            wifi_network_status.netmask[3]);
  con_println(status_str);
  sprintf(status_str, "    gateway: %d:%d:%d:%d",
                            wifi_network_status.gateway[0],
                            wifi_network_status.gateway[1],
                            wifi_network_status.gateway[2],
                            wifi_network_status.gateway[3]);
  con_println(status_str);
  sprintf(status_str, "    status : %d\n", wifi_network_status.status);
  con_println(status_str);

  free(status_str);
}

void wifi_set_cp_ssid(WIFI_CP cp, char *ssid) {
uint8_t *data = malloc((2+strlen(ssid))*sizeof(uint8_t));
data[0] = cp;
data[1] = strlen(ssid);
memcpy(&data[2], ssid, strlen(ssid));
  wifi_send_basic_packet(WIFI_PACKET_TYPE_SET_CP_SSID_MSG, WIFI_PACKET_TYPE_ACK, (strlen(ssid)+2), data);
  free(data);
}

//general configuration
void wifi_set_cp_network_mode(WIFI_CP cp, WIFI_NETWORK_MODE network_mode) {
  uint16_t data;
  ((uint8_t*)&data)[0] = cp;
  ((uint8_t*)&data)[1] = network_mode;
  wifi_send_basic_packet(WIFI_PACKET_TYPE_SET_CP_NETWORK_MODE_MSG, WIFI_PACKET_TYPE_ACK, 2, (uint8_t*)&data);
}

void wifi_set_channel_list(uint8_t *channels, uint8_t num_channels) {
  uint8_t *data = malloc((2+num_channels)*sizeof(uint8_t));
  data[0] = num_channels;
  memcpy(&data[2], channels, num_channels);
  wifi_send_basic_packet(WIFI_PACKET_TYPE_SET_CHANNEL_LIST_MSG, WIFI_PACKET_TYPE_ACK, (num_channels+2), data);
  free(data);
}

void wifi_set_list_retry_count(uint8_t infra_count, uint8_t adhoc_count) {
  uint16_t data;
  ((uint8_t*)&data)[0] = infra_count;
  ((uint8_t*)&data)[1] = adhoc_count;
  wifi_send_basic_packet(WIFI_PACKET_TYPE_SET_LIST_RETRY_COUNT_MSG, WIFI_PACKET_TYPE_ACK, 2, (uint8_t*)&data);
}

//security configuration
void wifi_set_cp_security_open(WIFI_CP cp) {
  uint16_t data;
  ((uint8_t*)&data)[0] = cp;
  wifi_send_basic_packet(WIFI_PACKET_TYPE_SET_CP_SECURITY_OPEN_MSG, WIFI_PACKET_TYPE_ACK, 2, (uint8_t*)&data);
}

//connection
void wifi_connect(WIFI_CP cp) {
  uint16_t data;
  ((uint8_t*)&data)[0] = cp;
  wifi_send_basic_packet(WIFI_PACKET_TYPE_WIFI_CONNECT_MSG, WIFI_PACKET_TYPE_ACK, 2, (uint8_t*)&data);
  while(!wifi_wait_for_event_response(WIFI_PACKET_TYPE_EVENT_WIFI_STATUS_CHANGED));
}

void wifi_disconnect() {
  wifi_send_basic_packet(WIFI_PACKET_TYPE_WIFI_DISCONNECT_MSG, WIFI_PACKET_TYPE_ACK, 0, NULL);
  wifi_wait_for_response(WIFI_PACKET_TYPE_EVENT);
}

//socket
uint8_t wifi_socket_create(WIFI_SOCKET_TYPE socket_type) {
  uint16_t data;
  ((uint8_t*)&data)[0] = socket_type;
  wifi_send_basic_packet(WIFI_PACKET_TYPE_SOCKET_CREATE_MSG, WIFI_PACKET_TYPE_SOCKET_CREATE_RESPONSE_MSG, 2, (uint8_t*)&data);
  return wifi_socket_handle;
}

WIFI_PACKET_SOCKET_BIND_RESPONSE wifi_socket_bind(uint16_t port, uint8_t socket_handle) {
  uint8_t *data = malloc(4*sizeof(uint8_t));
  data[0] = port & 0xFF;
  data[1] = (port >> 8) & 0xFF;
  data[2] = socket_handle;
  data[3] = 0;
  wifi_send_basic_packet(WIFI_PACKET_TYPE_SOCKET_BIND_MSG, WIFI_PACKET_TYPE_SOCKET_BIND_RESPONSE_MSG, 4, data);
  free(data);
  return wifi_socket_bind_response;
}

void wifi_socket_recv_from(uint8_t socket_handle, uint16_t len) {
  if(recv_from_clear == true) {
    uint8_t *data = malloc(4*sizeof(uint8_t));
    data[0] = socket_handle;
    data[1] = 0;
    data[2] = len & 0xFF;
    data[3] = (len >> 8) & 0xFF;
    recv_from_clear = false;
    wifi_send_basic_packet(WIFI_PACKET_TYPE_SOCKET_RECV_FROM_MSG, WIFI_PACKET_TYPE_NONE, 4, data);
    free(data);
  }
}

void wifi_socket_recv(uint8_t socket_handle, uint16_t len) {
  uint8_t *data = malloc(4*sizeof(uint8_t));
  data[0] = socket_handle;
  data[1] = 0;
  data[2] = len & 0xFF;
  data[3] = (len >> 8) & 0xFF;
  wifi_send_basic_packet(WIFI_PACKET_TYPE_SOCKET_RECV_MSG, WIFI_PACKET_TYPE_NONE, 4, data);
  free(data);
}

void wifi_socket_send_to(uint8_t socket_handle, uint16_t remote_port, uint8_t *remote_ip, uint16_t len, uint8_t *data) {
  uint8_t *packet_data = malloc((22+len)*sizeof(uint8_t));
  packet_data[0] = socket_handle;
  packet_data[2] = remote_port & 0xFF;
  packet_data[3] = (remote_port >> 8) & 0xFF;
  memcpy(packet_data+4, remote_ip, 16*sizeof(uint8_t));
  packet_data[20] = len & 0xFF;
  packet_data[21] = (len >> 8) & 0xFF;
  memcpy(packet_data+22, data, len*sizeof(uint8_t));
  wifi_send_basic_packet(WIFI_PACKET_TYPE_SOCKET_SEND_TO_MSG, WIFI_PACKET_TYPE_NONE, 22+len, packet_data);

  //print what was transmitted
  #if SHOW_WIFI_TX
  char *str = malloc(256*sizeof(char));
  sprintf(str, "tx {%02x.%02x.%02x.%02x}: ", remote_ip[0],
                                             remote_ip[1],
                                             remote_ip[2],
                                             remote_ip[3]);
  int i;
  for(i = 0; i < len; i++) {
    sprintf(str+strlen(str), "%x ", packet_data[22+i]);
  }
  sprintf(str+strlen(str), "; ");
  for(i = 0; i < len; i++) {
    sprintf(str+strlen(str), "%c", packet_data[22+i]);
  }
  con_println(str);
  free(str);
  #endif

  free(packet_data);
}

//-------------------------------------------------------------------------------
//
//
// Hidden
//
//
//-------------------------------------------------------------------------------

void wifi_send_basic_packet(uint16_t packet_type, uint16_t response_type, uint16_t len, uint8_t *data) {
  WIFI_PACKET *p = malloc(sizeof(WIFI_PACKET));
  p->type = packet_type;
  p->len = len;
  p->data = malloc(len*sizeof(uint8_t));
  memcpy(p->data, data, len);

  //send requests until a correct response is found
  bool ret = false;
  while(ret == false) {
    wifi_put_packet(p);
    ret = wifi_wait_for_response(response_type);
  }

  free(p->data);
  free(p);
}

bool wifi_wait_for_response(uint16_t packet_type) {
  if(packet_type == WIFI_PACKET_TYPE_NONE) {
    return true;
  }

  WIFI_PACKET *p;
  while(task_wait_for_event_wto(TASK_EVENT_WIFI, 100) == true) {
    bool correct_response_found = false;

    //process all available wifi packets
    p = wifi_get_packet();
    while(p != NULL) {
      if((p->type & ~WIFI_PACKET_TYPE_PIGGYBACKBIT) == packet_type) {
        correct_response_found = true;
      }
      wifi_process_packet(p);
      free(p->data);
      free(p);
      p = wifi_get_packet();
    }
    task_clear_event(TASK_EVENT_WIFI);

    //found the correct response so stop waiting
    if(correct_response_found == true) {
      return true;
    }
  }
  return false;
}

bool wifi_wait_for_event_response(uint16_t event_type) {
  WIFI_PACKET *p;
  while(task_wait_for_event_wto(TASK_EVENT_WIFI, 100) == true) {
    bool correct_response_found = false;

    //process all available wifi packets
    p = wifi_get_packet();
    while(p != NULL) {
      if((p->type & ~WIFI_PACKET_TYPE_PIGGYBACKBIT) == WIFI_PACKET_TYPE_EVENT &&
          p->data[0] == event_type) {
        correct_response_found = true;
      }
      wifi_process_packet(p);
      free(p->data);
      free(p);
      p = wifi_get_packet();
    }
    task_clear_event(TASK_EVENT_WIFI);

    //found the correct response so stop waiting
    if(correct_response_found == true) {
      return true;
    }
  }
  return false;
}

void wifi_put_packet(WIFI_PACKET *p) {

#if SHOW_WIFI_PACKETS
  con_printf("-> ");
  wifi_print_packet(p);
#endif

  uint8_t *data = malloc((7+(p->len))*sizeof(char));
  data[0] = 0x55; //header
  data[1] = 0xAA;
  data[2] = p->type & 0xFF; //type
  data[3] = (p->type >> 8) & 0xFF;
  data[4] = p->len & 0xFF; //len
  data[5] = (p->len >> 8) & 0xFF;
  memcpy(data+6, p->data, p->len);
  data[6+(p->len)] = 0x45;

  int i;
  for(i = 0; i < (7+(p->len)); i++) {
    UARTCharPut(UART1_BASE, data[i]);
  }

  free(data);
}

WIFI_PACKET* wifi_get_packet() {
  WIFI_PACKET *p = malloc(sizeof(WIFI_PACKET));
  memset(p, 0, sizeof(WIFI_PACKET));

  //check that we have at least 7 bytes before the end of the buffer
  uint16_t distance = buffer_end + 1 + WIFI_BUFFER_LENGTH - buffer_start;
  if(distance >= WIFI_BUFFER_LENGTH) {
    distance -= WIFI_BUFFER_LENGTH;
  }
  if(distance < 7) {
    free(p);
    buffer_start = buffer_end;
    buffer_size  = 0;
    return NULL;
  }

  //get the `next` byte of the header to compare
  uint16_t next = BUFFER_NEXT(buffer_start);

  //look for header 0x55AA
  while(buffer[buffer_start] != 0x55 || buffer[next] != 0xAA) {
    distance--;
    if(distance < 7) {
      free(p);
      buffer_start = buffer_end;
      buffer_size  = 0;
      return NULL;
    }

    //move forward and keep looking
    buffer_start = next;
    next = BUFFER_NEXT(buffer_start);
  }

  //get the packet type
  uint16_t type;
  next = BUFFER_NEXT(next);
  type = buffer[next];
  next = BUFFER_NEXT(next);
  type |= (buffer[next] << 8);
  p->type = type;
  distance -= 2;

  //get the packet data length
  uint16_t len;
  next = BUFFER_NEXT(next);
  len = buffer[next];
  next = BUFFER_NEXT(next);
  len |= (buffer[next] << 8);
  p->len = len;
  distance -= 2;

  //check that we have enough distance for the entire packet
  if(distance < (1+len)) {
    free(p);
    buffer_start = buffer_end;
    buffer_size  = 0;
    return NULL;
  }

  //get the data
  p->data = malloc(len * sizeof(uint8_t));
  int i;
  for(i = 0; i < len; i++) {
    next = BUFFER_NEXT(next);
    p->data[i] = buffer[next];
  }

  //verify that the end of the packet has 0x45
  next = BUFFER_NEXT(next);
  if(buffer[next] != 0x45) {
    free(p->data);
    free(p);
    buffer_start = buffer_end;
    buffer_size  = 0;
    return NULL;
  }

  //update the buffer and return the packet
  if(next == buffer_end) {
    buffer_end = BUFFER_NEXT(next);
  }
  buffer_start = BUFFER_NEXT(next);
  buffer_size -= (7 + len);
  return p;
}

void wifi_print_packet(WIFI_PACKET *p) {
  if(p != NULL) {
    char *str = malloc(256*sizeof(char));
    sprintf(str, "packet: {type: %d, len: %d, data: ",
        p->type,
        p->len);
    
    int i;
    for(i = 0; i < p->len; i++) {
      sprintf(str+strlen(str), "0x%x ", p->data[i]);
    }

    sprintf(str+strlen(str), "}");

    con_println(str);
    free(str);
  }
}

//process the packet
//1. check the type
//2. check that the size is correct based on the type
//3. parse the data and save
PACKET_STATUS wifi_process_packet(WIFI_PACKET *p) {

#if SHOW_WIFI_PACKETS
  con_printf("<- ");
  wifi_print_packet(p);
#endif

  switch(p->type & ~WIFI_PACKET_TYPE_PIGGYBACKBIT) {
    case WIFI_PACKET_TYPE_ACK: {
      if(p->len != 0) {
        return PACKET_LENGTH_INVALID;
      }
      break;
    }
    case WIFI_PACKET_TYPE_GPIO_RESPONSE_MSG: {
      if(p->len != 2) {
        return PACKET_LENGTH_INVALID;
      }
      break;
    }
    case WIFI_PACKET_TYPE_NETWORK_STATUS_RESPONSE_MSG: {
      if(p->len != 56) {
        return PACKET_LENGTH_INVALID;
      }
      memcpy(&wifi_network_status, p->data, sizeof(WIFI_PACKET_NETWORK_STATUS));


      break;
    }
    case WIFI_PACKET_TYPE_WPAKEY_RESPONSE_MSG: {
      if(p->len != 32) {
        return PACKET_LENGTH_INVALID;
      }
      break;
    }
    case WIFI_PACKET_TYPE_SCAN_RESULT_MSG: {
      if(p->len != 57) {
        return PACKET_LENGTH_INVALID;
      }
      break;
    }
    case WIFI_PACKET_TYPE_SOCKET_CREATE_RESPONSE_MSG: {
      if(p->len != 2) {
        return PACKET_LENGTH_INVALID;
      }
      wifi_socket_handle = p->data[0];
      break;
    }
    case WIFI_PACKET_TYPE_SOCKET_BIND_RESPONSE_MSG: {
      if(p->len != 4) {
        return PACKET_LENGTH_INVALID;
      }
      memcpy(&wifi_socket_bind_response, p->data, sizeof(WIFI_PACKET_SOCKET_BIND_RESPONSE));
      if(wifi_socket_bind_response.result == 0) {
        con_println("Successfully bound socket");
      }
      else {
        con_println("Failed to bind socket");
      }
      break;
    }
    case WIFI_PACKET_TYPE_SOCKET_CONNECT_RESPONSE_MSG: {
      if(p->len != 2) {
        return PACKET_LENGTH_INVALID;
      }
      wifi_socket_connect_result = p->data[0];
      break;
    }
    case WIFI_PACKET_TYPE_SOCKET_LISTEN_RESPONSE_MSG: {
      if(p->len != 2) {
        return PACKET_LENGTH_INVALID;
      }
      break;
    }
    case WIFI_PACKET_TYPE_SOCKET_ACCEPT_RESPONSE_MSG: {
      if(p->len != 4) {
        return PACKET_LENGTH_INVALID;
      }
      memcpy(&wifi_socket_accept_response, p->data, sizeof(WIFI_PACKET_SOCKET_ACCEPT_RESPONSE));
      break;
    }
    case WIFI_PACKET_TYPE_SOCKET_SEND_RESPONSE_MSG: {
      if(p->len != 2) {
        return PACKET_LENGTH_INVALID;
      }
      break;
    }
    case WIFI_PACKET_TYPE_SOCKET_RECV_RESPONSE_MSG: {
      if(p->len < 4) {
        return PACKET_LENGTH_INVALID;
      }

      //do not copy the data yet
      memcpy(&wifi_socket_recv_response, p->data, sizeof(WIFI_PACKET_SOCKET_RECV_RESPONSE)-sizeof(uint8_t*));

      //copy the data
      free(wifi_socket_recv_response.data);
      wifi_socket_recv_response.data = malloc(wifi_socket_recv_response.size);
      memcpy(&wifi_socket_recv_response.data, p->data+4, wifi_socket_recv_response.size);
      break;
    }
    case WIFI_PACKET_TYPE_SOCKET_SEND_TO_RESPONSE_MSG: {
      if(p->len != 2) {
        return PACKET_LENGTH_INVALID;
      }

      memcpy(&wifi_socket_send_to_response, p->data, sizeof(WIFI_PACKET_SOCKET_SEND_TO_RESPONSE));
      break;
    }
    case WIFI_PACKET_TYPE_SOCKET_RECV_FROM_RESPONSE_MSG: {
      if(p->len < 22) {
        return PACKET_LENGTH_INVALID;
      }

      //save the data
      memcpy(&wifi_socket_recv_from_response, p->data, 22*sizeof(uint8_t*));
      wifi_socket_recv_from_response.data = malloc(wifi_socket_recv_from_response.size*sizeof(uint8_t));
      memcpy(&(wifi_socket_recv_from_response.data), p->data+22, wifi_socket_recv_from_response.size);

      //print what was received
      #if SHOW_WIFI_RX
      char *str = malloc(256*sizeof(char));
      sprintf(str, "rx {%02x.%02x.%02x.%02x}: ", wifi_socket_recv_from_response.remote_ip[0],
                                             wifi_socket_recv_from_response.remote_ip[1],
                                             wifi_socket_recv_from_response.remote_ip[2],
                                             wifi_socket_recv_from_response.remote_ip[3]);
      int i;
      for(i = 0; i < wifi_socket_recv_from_response.size; i++) {
        sprintf(str+strlen(str), "%x ", p->data[22+i]);
      }
      sprintf(str+strlen(str), "; ");
      for(i = 0; i < wifi_socket_recv_from_response.size; i++) {
        sprintf(str+strlen(str), "%c", p->data[22+i]);
      }
      con_println(str);
      free(str);
      #endif

      recv_from_clear = true;
      break;
    }
    case WIFI_PACKET_TYPE_SOCKET_ALLOCATE_RESPONSE_MSG: {
      if(p->len != 2) {
        return PACKET_LENGTH_INVALID;
      }
      memcpy(&wifi_socket_allocate_response, p->data, sizeof(WIFI_PACKET_SOCKET_ALLOCATE_RESPONSE));
      break;
    }
    case WIFI_PACKET_TYPE_EVENT: {
      uint8_t event_type = p->data[0];
      switch(event_type) {
        case WIFI_PACKET_TYPE_EVENT_IP_ADDRESS_ASSIGNED: {
          if(p->len != 6) {
            return PACKET_LENGTH_INVALID;
          }
          con_println("Event: ip address assigned");
          break;
        }
        case WIFI_PACKET_TYPE_EVENT_WIFI_STATUS_CHANGED: {
          if(p->len != 4) {
            return PACKET_LENGTH_INVALID;
          }
          con_println("Event: wifi status changed");
          break;
        }
        case WIFI_PACKET_TYPE_EVENT_WIFI_SCAN_RESULTS_READY: {
          if(p->len != 2) {
            return PACKET_LENGTH_INVALID;
          }
          con_println("Event: wifi scan results ready");
          break;
        }
        case WIFI_PACKET_TYPE_EVENT_PING_RESPONSE_EVENT: {
          if(p->len != 4) {
            return PACKET_LENGTH_INVALID;
          }
          con_println("Event: ping response event");
          break;
        }
        case WIFI_PACKET_TYPE_EVENT_ERROR: {
          if(p->len != 8) {
            return PACKET_LENGTH_INVALID;
          }
          uint16_t error_type = (p->data[2] | (p->data[3] << 8));
          char str[30];
          sprintf(str, "Event: error#%d", error_type);
          con_println(str);
          break;
        }
      }
      break;
    }
    default: {
      return PACKET_UNKNOWN;
    }
  }

  //no errors so return true
  return PACKET_OK;
}

void wifi_mcw1001a_mikro_interrupt_handler() {

  //clear interrupt
  uint32_t ui32Status;
  ui32Status = ROM_UARTIntStatus(UART0_BASE, true);
  ROM_UARTIntClear(UART1_BASE, ui32Status);

  //retrieve character
  int32_t c = UARTCharGetNonBlocking(UART1_BASE);

  //buffer is empty so place the character in the first space
  if(buffer_size == 0) {
    buffer[buffer_start] = c;
    buffer_size++;
  }

  //buffer is not empty, so ensure that we do not overflow
  else {

    //calculate the new buffer end location
    uint16_t proposed_buffer_end = BUFFER_NEXT(buffer_end);

    //ensure that we do not overflow the buffer
    if(proposed_buffer_end != buffer_start) {
      buffer_end = proposed_buffer_end;
      buffer[buffer_end] = c;
      buffer_size++;
    }
  }

  //if an end of packet was found, set the event to be triggered
  if(c == 0x45) {
    task_set_event(TASK_EVENT_WIFI);
  }
}
