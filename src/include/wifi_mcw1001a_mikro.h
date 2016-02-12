/*
 * wifi_mcw1001a_mikro.h
 *
 *  Created on: Jul 22, 2015
 *      Author: Aidan
 */



#ifndef WIFI_MCW1001A_MIKRO_H_
#define WIFI_MCW1001A_MIKRO_H_

/*-------------------------------------------------------------------
Includes
-------------------------------------------------------------------*/
#include "common.h"

/*-------------------------------------------------------------------
Types
-------------------------------------------------------------------*/
//wifi packet structure
typedef struct {
  uint16_t type;
  uint16_t len;
  uint8_t *data;
} WIFI_PACKET;

//packet processing return codes
typedef enum {
  PACKET_OK = 0,
  PACKET_LENGTH_INVALID,
  PACKET_UNKNOWN
} PACKET_STATUS;

//wifi network connection status
typedef enum {
  WIFI_NETWORK_STATUS_NOT_CONNECTED_STATIC_IP = 0,
  WIFI_NETWORK_STATUS_CONNECTED_STATIC_IP,
  WIFI_NETWORK_STATUS_NOT_CONNECTED_DHCP,
  WIFI_NETWORK_STATUS_CONNECTED_DHCP
} WIFI_NETWORK_STATUS;

//wifi status packet
typedef struct {
  uint8_t RESERVED;
  uint8_t mac[6];
  uint8_t ip[16];
  uint8_t netmask[16];
  uint8_t gateway[16];
  WIFI_NETWORK_STATUS status;
} WIFI_PACKET_NETWORK_STATUS;

//wifi socket bind response packet
typedef struct {
  uint16_t local_port;
  uint8_t result;
  uint8_t RESERVED;
} WIFI_PACKET_SOCKET_BIND_RESPONSE;

//wifi socket accept response packet
typedef struct {
  uint8_t socket_handle;
  uint8_t RESERVED;
  uint16_t remote_port;
  uint8_t remote_ip[16];
} WIFI_PACKET_SOCKET_ACCEPT_RESPONSE;

//wifi socket allocate response packet
typedef struct {
  uint8_t response;
  uint8_t allocated_count;
} WIFI_PACKET_SOCKET_ALLOCATE_RESPONSE;

//wifi socket receive from response packet
typedef struct {
  uint8_t socket_handle;
  uint8_t RESERVED;
  uint16_t size;
  uint8_t *data;
} WIFI_PACKET_SOCKET_RECV_RESPONSE;

//wifi socket receive from response packet
typedef struct {
  uint8_t socket_handle;
  uint8_t RESERVED;
  uint16_t remote_port;
  uint8_t remote_ip[16];
  uint16_t size;
  uint8_t *data;
  uint16_t dir;
} WIFI_PACKET_SOCKET_RECV_FROM_RESPONSE;

//wifi socket send to response packet
typedef struct {
  uint16_t size;
} WIFI_PACKET_SOCKET_SEND_TO_RESPONSE;

//connection profiles
typedef enum {
  WIFI_CP1 = 1,
  WIFI_CP2
} WIFI_CP;

//network modes
typedef enum {
  WIFI_NETWORK_MODE_INFRA = 1,
  WIFI_NETWORK_MODE_ADHOC
} WIFI_NETWORK_MODE;

typedef enum {
  WIFI_IP_CONFIG_DHCP = 0,
  WIFI_IP_CONFIG_STATIC,
} WIFI_IP_CONFIG;

typedef enum {
  WIFI_SOCKET_TYPE_UDP = 0,
  WIFI_SOCKET_TYPE_TCP
} WIFI_SOCKET_TYPE;

/*-------------------------------------------------------------------
Constants
-------------------------------------------------------------------*/

#define WIFI_SOCKET_HANDLE_INVALID                        254
#define WIFI_SOCKET_HANDLE_UNKOWN                         255

#define WIFI_PACKET_TYPE_NONE                             0xFFFF

//ack
#define WIFI_PACKET_TYPE_ACK                              0
#define WIFI_PACKET_TYPE_PIGGYBACKBIT                     bitset(15)

//control messages
#define WIFI_PACKET_TYPE_RESET_MSG                        170
#define WIFI_PACKET_TYPE_GET_VERSION_MSG                  23
#define WIFI_PACKET_TYPE_GPIO_MSG                         172
#define WIFI_PACKET_TYPE_GPIO_RESPONSE_MSG                50

//network configuration messages
#define WIFI_PACKET_TYPE_SET_IP_ADDRESS_MSG               41
#define WIFI_PACKET_TYPE_SET_NETWORK_MASK_MSG             42
#define WIFI_PACKET_TYPE_SET_GATEWAY_IP_ADDRESS_MSG       44
#define WIFI_PACKET_TYPE_GET_NETWORK_STATUS               48
#define WIFI_PACKET_TYPE_NETWORK_STATUS_RESPONSE_MSG      48
#define WIFI_PACKET_TYPE_SET_MACADDRESS_MSG               49
#define WIFI_PACKET_TYPE_SET_ARP_TIME_MSG                 173

//wifi general configuration messages
#define WIFI_PACKET_TYPE_SET_CP_NETWORK_MODE_MSG          55
#define WIFI_PACKET_TYPE_SET_CP_SSID_MSG                  57
#define WIFI_PACKET_TYPE_SET_REGIONAL_DOMAIN_MSG          56
#define WIFI_PACKET_TYPE_SET_CHANNEL_LIST_MSG             58
#define WIFI_PACKET_TYPE_SET_LIST_RETRY_COUNT_MSG         59

//wifi security configuration messages
#define WIFI_PACKET_TYPE_SET_CP_SECURITY_OPEN_MSG         65
#define WIFI_PACKET_TYPE_SET_CP_SECURITY_WEP40_MSG        66
#define WIFI_PACKET_TYPE_SET_CP_SECURITY_WEP104_MSG       67
#define WIFI_PACKET_TYPE_SET_CP_SECURITY_WPA_MSG          68
#define WIFI_PACKET_TYPE_GET_CP_WPAKEY_MSG                71
#define WIFI_PACKET_TYPE_WPAKEY_RESPONSE_MSG              49

//wifi scanning messages
#define WIFI_PACKET_TYPE_SCAN_START_MSG                   80
#define WIFI_PACKET_TYPE_SCAN_GET_RESULTS_MSG             81
#define WIFI_PACKET_TYPE_SCAN_RESULT_MSG                  22

//wifi connection messages
#define WIFI_PACKET_TYPE_WIFI_CONNECT_MSG                 90
#define WIFI_PACKET_TYPE_WIFI_DISCONNECT_MSG              91

//icmp messages
#define WIFI_PACKET_TYPE_PING_SEND_MSG                    121

//socket messages
#define WIFI_PACKET_TYPE_SOCKET_CREATE_MSG                110
#define WIFI_PACKET_TYPE_SOCKET_CREATE_RESPONSE_MSG       23
#define WIFI_PACKET_TYPE_SOCKET_CLOSE_MSG                 111
#define WIFI_PACKET_TYPE_SOCKET_BIND_MSG                  112
#define WIFI_PACKET_TYPE_SOCKET_BIND_RESPONSE_MSG         24
#define WIFI_PACKET_TYPE_SOCKET_CONNECT_MSG               113
#define WIFI_PACKET_TYPE_SOCKET_CONNECT_RESPONSE_MSG      25
#define WIFI_PACKET_TYPE_SOCKET_LISTEN_MSG                114
#define WIFI_PACKET_TYPE_SOCKET_LISTEN_RESPONSE_MSG       26
#define WIFI_PACKET_TYPE_SOCKET_ACCEPT_MSG                115
#define WIFI_PACKET_TYPE_SOCKET_ACCEPT_RESPONSE_MSG       27
#define WIFI_PACKET_TYPE_SOCKET_SEND_MSG                  116
#define WIFI_PACKET_TYPE_SOCKET_SEND_RESPONSE_MSG         28
#define WIFI_PACKET_TYPE_SOCKET_RECV_MSG                  117
#define WIFI_PACKET_TYPE_SOCKET_RECV_RESPONSE_MSG         29
#define WIFI_PACKET_TYPE_SOCKET_SEND_TO_MSG               118
#define WIFI_PACKET_TYPE_SOCKET_SEND_TO_RESPONSE_MSG      30
#define WIFI_PACKET_TYPE_SOCKET_RECV_FROM_MSG             119
#define WIFI_PACKET_TYPE_SOCKET_RECV_FROM_RESPONSE_MSG    31
#define WIFI_PACKET_TYPE_SOCKET_ALLOCATE_RESPONSE_MSG     32

//event
#define WIFI_PACKET_TYPE_EVENT                            1
#define WIFI_PACKET_TYPE_EVENT_IP_ADDRESS_ASSIGNED        16
#define WIFI_PACKET_TYPE_EVENT_WIFI_STATUS_CHANGED        8
#define WIFI_PACKET_TYPE_EVENT_WIFI_SCAN_RESULTS_READY    9
#define WIFI_PACKET_TYPE_EVENT_PING_RESPONSE_EVENT        26
#define WIFI_PACKET_TYPE_EVENT_ERROR                      255

//error message types
#define WIFI_ERROR_BAUD_RATE_GENERATOR                    60
#define WIFI_ERROR_INVALID_CONNECTION_PROFILE_ID          61
#define WIFI_ERROR_WIFI_ALREADY_CONNECTED                 62
#define WIFI_ERROR_WIFI_ALREADY_DISCONNECTED              63
#define WIFI_ERROR_CLOSE_SOCKET_FAILED                    64
#define WIFI_ERROR_SOCKET_SENDTO_TIMEOUT                  65
#define WIFI_ERROR_SCAN_INDEX_OUT_OF_RANGE                66
#define WIFI_ERROR_ICMP_PING_FLOOD                        67
#define WIFI_ERROR_ICMP_PING_IN_USE                       68
#define WIFI_ERROR_SOCKET_RECVFROM_FAILED                 69
#define WIFI_ERROR_SERIAL_TX_BUFFER_ALLOCATION            71
#define WIFI_ERROR_GENERAL_ASSERT                         72
#define WIFI_ERROR_INVALID_POWERSAVE_MODE                 73
#define WIFI_ERROR_BUSY_HIBERNATE                         74
#define WIFI_ERROR_BUSY_SCAN                              75

/*-------------------------------------------------------------------
Procedures
-------------------------------------------------------------------*/
void wifi_init();
void wifi_parse_buffer();

//control
void wifi_reset();
void wifi_get_version();
void wifi_gpio();

//network configuration
void wifi_set_ip_address(WIFI_IP_CONFIG ip_config, uint8_t *ip);
void wifi_set_netmask(uint8_t *netmask);
void wifi_set_gateway(uint8_t *gateway);
void wifi_set_mac(uint8_t *mac);
void wifi_set_arp_time(uint16_t arp_time);
WIFI_PACKET_NETWORK_STATUS wifi_get_network_status();
void wifi_print_network_status();

//general configuration
void wifi_set_cp_network_mode(WIFI_CP cp, WIFI_NETWORK_MODE network_mode);
void wifi_set_cp_ssid(WIFI_CP cp, char *ssid);
void wifi_set_regional_domain(uint8_t country_code);
void wifi_set_channel_list(uint8_t *channels, uint8_t num_channels);
void wifi_set_list_retry_count(uint8_t infra_count, uint8_t adhoc_count);

//security configuration
void wifi_set_cp_security_open(WIFI_CP cp);
void wifi_set_cp_security_wep40();
void wifi_set_cp_security_wep104();
void wifi_set_cp_security_wpa();
void wifi_get_cp_wpakey();

//scanning
void wifi_scan_start();
void wifi_scan_get_results();

//connection
void wifi_connect(WIFI_CP cp);
void wifi_disconnect();

//power save mode
void wifi_set_power_save_mode();

//icmp
void wifi_ping_send();

//socket
uint8_t wifi_socket_create(WIFI_SOCKET_TYPE socket_type);
void wifi_socket_close();
WIFI_PACKET_SOCKET_BIND_RESPONSE wifi_socket_bind(uint16_t port, uint8_t socket_handle);
void wifi_socket_connect();
void wifi_socket_listen();
void wifi_socket_accept();
void wifi_socket_send();
void wifi_socket_recv(uint8_t socket_handle, uint16_t len);
void wifi_socket_send_to(uint8_t socket_handle, uint16_t remote_port, uint8_t *remote_ip, uint16_t len, uint8_t *data);
void wifi_socket_recv_from(uint8_t socket_handle, uint16_t len);
bool wifi_get_recv_from_packet(WIFI_PACKET_SOCKET_RECV_FROM_RESPONSE *recv_from);
void wifi_socket_allocate();

#endif //WIFI_MCW1001A_MIKRO_H_
