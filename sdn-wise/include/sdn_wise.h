/*
* File:   sdn_wise.h
* Author: Sebastiano Milardo
*
* Created on 19 settembre 2013, 10.27
*/

#ifndef SDN_WISE_H
#define SDN_WISE_H

#include "queue.h"
#include "hal_types.h"

/*------------------------------------------------------------------------------
/         NODE ADDRESS
/ ----------------------------------------------------------------------------*/
#define SDN_WISE_DFLT_ADDR                      5
#define SDN_WISE_DFLT_NET_ID                    1

/*------------------------------------------------------------------------------
/         Packet
/ ----------------------------------------------------------------------------*/
enum PacketFields{
 SDN_WISE_LEN,                            
 SDN_WISE_NET_ID,                         
 SDN_WISE_SRC_H,                          
 SDN_WISE_SRC_L,                          
 SDN_WISE_DST_H,                          
 SDN_WISE_DST_L,                          
 SDN_WISE_TYPE,                           
 SDN_WISE_TTL,                            
 SDN_WISE_NXHOP_H,                        
 SDN_WISE_NXHOP_L,                        
 SDN_WISE_DIST,                           
 SDN_WISE_BATT,                           
 SDN_WISE_NEIGH                          
};

// packet types
enum PacketTypes{
 SDN_WISE_DATA,                           
 SDN_WISE_BEACON,                         
 SDN_WISE_REPORT,                         
 SDN_WISE_REQUEST,                        
 SDN_WISE_RESPONSE,                       
 SDN_WISE_OPEN_PATH,                      
 SDN_WISE_CONFIG,                         
};

// packet header length
#define SDN_WISE_PACKET_LEN                     128
#define SDN_WISE_DFLT_HDR_LEN                   10
#define SDN_WISE_BEACON_HDR_LEN                 SDN_WISE_DFLT_HDR_LEN+2
#define SDN_WISE_REPORT_HDR_LEN                 SDN_WISE_DFLT_HDR_LEN+3
#define SDN_WISE_RESPONSE_HDR_LEN               SDN_WISE_DFLT_HDR_LEN
#define SDN_WISE_OPEN_PATH_HDR_LEN              SDN_WISE_DFLT_HDR_LEN
#define SDN_WISE_CONFIG_HDR_LEN                 SDN_WISE_DFLT_HDR_LEN

// routing
#define SDN_WISE_DFLT_TTL_MAX                   20                          
#define SDN_WISE_DFLT_RSSI_MIN                  0 

// tables
#define SDN_WISE_RLS_MAX                        16
#define SDN_WISE_WINDOWS_MAX                    3
#define SDN_WISE_NEIGHBORS_MAX                  10
#define SDN_WISE_ACCEPTED_ID_MAX                10

// rule lengths
#define SDN_WISE_RL_ACTION_LEN  4
#define SDN_WISE_RL_WINDOW_LEN  4
#define SDN_WISE_RL_WINDOWS_LEN SDN_WISE_RL_WINDOW_LEN * SDN_WISE_WINDOWS_MAX
#define SDN_WISE_RL_BODY_LEN    SDN_WISE_RL_ACTION_LEN + SDN_WISE_RL_WINDOWS_LEN + 1
#define SDN_WISE_RL_TTL_DECR    10

// memory
#define SDN_WISE_PACKET                         1
#define SDN_WISE_STATUS                         0

// size
#define SDN_WISE_SIZE_0                         0
#define SDN_WISE_SIZE_1                         2
#define SDN_WISE_SIZE_2                         4

// operators
#define SDN_WISE_EQUAL                          8
#define SDN_WISE_NOT_EQUAL                      16
#define SDN_WISE_BIGGER                         24
#define SDN_WISE_LESS                           32 
#define SDN_WISE_EQUAL_OR_BIGGER                40
#define SDN_WISE_EQUAL_OR_LESS                  48

// multimatch
#define SDN_WISE_MULTI                          2
#define SDN_WISE_NOT_MULTI                      0

// actions
#define SDN_WISE_FORWARD_UNICAST                4
#define SDN_WISE_FORWARD_BROADCAST              8
#define SDN_WISE_DROP                           12
#define SDN_WISE_MODIFY                         16
#define SDN_WISE_AGGREGATE                      20
#define SDN_WISE_FORWARD_UP                     24

// stats
#define SDN_WISE_PERMANENT_RULE                 255

// timers
#if NODE                          
# define SDN_WISE_DFLT_CNT_DATA_MAX             10
# define SDN_WISE_DFLT_CNT_BEACON_MAX           10
# define SDN_WISE_DFLT_CNT_REPORT_MAX           2*SDN_WISE_DFLT_CNT_BEACON_MAX
# define SDN_WISE_DFLT_CNT_UPDTABLE_MAX         6   // TTL = 150s
# define SDN_WISE_DFLT_CNT_SLEEP_MAX            100
#else                               
# define SDN_WISE_DFLT_CNT_BEACON_MAX           10
# define SDN_WISE_DFLT_CNT_REPORT_MAX           2*SDN_WISE_DFLT_CNT_BEACON_MAX
# define SDN_WISE_DFLT_CNT_UPDTABLE_MAX         6   // TTL = 150s
#endif

// status register
#define SDN_WISE_STATUS_LEN		        SDN_WISE_PACKET_LEN

// COM ports
#define SDN_WISE_COM_START_BYTE                 0x7A
#define SDN_WISE_COM_STOP_BYTE                  0x7E

// send
#define SDN_WISE_MAC_SEND_UNICAST               0
#define SDN_WISE_MAC_SEND_BROADCAST             1

// configurations
#define SDN_WISE_CNF_READ                       0
#define SDN_WISE_CNF_WRITE                      1
#define SDN_WISE_CNF_ID_ADDR                    0
#define SDN_WISE_CNF_ID_NET_ID                  1
#define SDN_WISE_CNF_ID_CNT_BEACON_MAX          2
#define SDN_WISE_CNF_ID_CNT_REPORT_MAX          3
#define SDN_WISE_CNF_ID_CNT_UPDTABLE_MAX        4
#define SDN_WISE_CNF_ID_CNT_SLEEP_MAX           5
#define SDN_WISE_CNF_ID_TTL_MAX                 6
#define SDN_WISE_CNF_ID_RSSI_MIN                7

#define SDN_WISE_CNF_ADD_ACCEPTED               8
#define SDN_WISE_CNF_REMOVE_ACCEPTED            9
#define SDN_WISE_CNF_LIST_ACCEPTED              10

#define SDN_WISE_CNF_ADD_RULE                   11
#define SDN_WISE_CNF_REMOVE_RULE                12
#define SDN_WISE_CNF_REMOVE_RULE_INDEX          13
#define SDN_WISE_CNF_LIST_RULE                  14

// mcu events
#define SDN_WISE_IDLE                           0
#define SDN_WISE_RX_UART                        1
#define SDN_WISE_RX_RADIO                       2
#define SDN_WISE_TX_BEACON                      3
#define SDN_WISE_TX_REPORT                      4
#define SDN_WISE_TX_DATA                        5
#define SDN_WISE_UPDTABLE                       6

// functions
#define LOW_BYTE(a)                     ((uint8_t)((a) & 0xFF))       
#define HIGH_BYTE(a)                    ((uint8_t)((a) >> 8  ))       
#define MERGE_BYTES(h,l)                ((uint16_t)((l) + ((h) << 8)))  
// extract n bits starting from s
#define GET_BIT_RANGE(b,s,n)            (((b) >> (s)) & ((1 << (n)) - 1))
#define SET_BIT_(n,pos)                 (((n) |= 1 << (pos)))
#define CLEAR_BIT_(n,pos)               (((n) &= ~(1 << (pos))))
#define GET_BIT_(n,pos)                 (((n) & (1 << (pos))))
#define GET_RL_WINDOW_SIZE(a)           ((uint8_t)(((a) & 0x06)))   
#define GET_RL_WINDOW_OPERATOR(a)       ((uint8_t)(((a) & 0xF8)))
#define GET_RL_WINDOW_LOCATION(a)       ((uint8_t)(((a) & 0x01))) //1 pckt 0 sta
#define GET_RL_ACTION_TYPE(a)           ((uint8_t)(((a) & 0xFC)))
#define GET_RL_ACTION_IS_MULTI(a)       ((uint8_t)(((a) & 0x02) >> 1))   
#define GET_RL_ACTION_LOCATION(a)       ((uint8_t)(((a) & 0x01))) //1 pckt 0 sta

/*------------------------------------------------------------------------------
/         Flow Table
/ ----------------------------------------------------------------------------*/

typedef struct StructWindow{
  uint8_t op;
  uint8_t pos;
  uint8_t value_h;
  uint8_t value_l;
} SdnWiseRuleWindow;

typedef struct StructAction{
  uint8_t act;
  uint8_t pos;
  uint8_t value_h;
  uint8_t value_l;
} SdnWiseRuleAction;

typedef struct StructStats{
  uint8_t ttl;
  uint8_t count;
} SdnWiseRuleStats;

typedef struct StructRule{ 
  SdnWiseRuleWindow window[SDN_WISE_WINDOWS_MAX];
  SdnWiseRuleAction action;
  SdnWiseRuleStats stats;
} SdnWiseRule;

/*------------------------------------------------------------------------------
/         Neighbors Table
/ ----------------------------------------------------------------------------*/

typedef struct StructNeig{
  uint8_t addr_h;
  uint8_t addr_l;
  uint8_t rssi;
  uint8_t batt;
} SdnWiseNeighbor;

/*------------------------------------------------------------------------------
/         Configuration Parameters
/ ----------------------------------------------------------------------------*/

typedef struct StructConfig {
  uint16_t addr;
  uint8_t addr_h;
  uint8_t addr_l;
  uint8_t net_id; 
  uint16_t cnt_beacon_max; 
  uint16_t cnt_report_max; 
  uint16_t cnt_updtable_max; 
  uint16_t cnt_sleep_max; 
  uint8_t ttl_max; 
  uint8_t rssi_min; 
} Config;

/*------------------------------------------------------------------------------
/          Global Vars
/ ----------------------------------------------------------------------------*/

// Stato del microcontroller
extern uint8_t MC_Status[7];

// buffer ricezione da Seriale
extern uint8_t bufferInUSB [SDN_WISE_PACKET_LEN];

// flag ricezione da seriale (inizio di un pacchetto)
extern uint8_t packet_start_flg;

// Conteggio di caratteri ricevuti da seriale
extern uint8_t receivedBytes;
extern uint8_t expectedBytes;

// queue messaggi da inviare
extern Queue tx_queue;

// queue messaggi da processare
extern Queue rx_queue;

// configurazione del protocollo
extern Config config;

// stato pacchetti dati inviati su RF
extern uint8_t lastTransmission;

extern uint16_t copie;
extern uint8_t lastDsn;

/*------------------------------------------------------------------------------
/         Functions
/ ----------------------------------------------------------------------------*/

// invia un pacchetto al livello MAC
void radioTX(uint8_t* packet, uint8_t is_multicast);

// invia un pacchetto sulla seriale
void controllerTX(uint8_t* packet);

// Gestione pacchetti ricevuti in base al Type
void rxHandler(uint8_t* packet, uint8_t rssi);

void rxDATA(uint8_t* packet);
void rxBEACON(uint8_t* packet, uint8_t rssi);
void rxREPORT(uint8_t* packet);
void rxREQUEST(uint8_t* packet);
void rxRESPONSE(uint8_t* packet);
void rxOPEN_PATH(uint8_t* packet);
void rxCONFIG(uint8_t* packet);

// Funzioni richiamate dai timer logici
void timerInterrupt(void);
void updateTable(void);

void txBEACON(void);
void txREPORT(void);
void txDATA(void);

/*------------------------------------------------------------------------------
/         Inits
/ ----------------------------------------------------------------------------*/

// Inizializza le varibili necessarie al corretto funzionamento del protocollo
void SDN_WISE_Init(void);

// Inizializza la FlowTable
void initFlowTable(void);
void initRule(SdnWiseRule* rule);

// Cancella/Inizializza la lista dei vicini
void initNeighborTable(void);

/*------------------------------------------------------------------------------
/         Flow Table
/ ----------------------------------------------------------------------------*/

// Inserisce una regola nella tabella
void insertRule(SdnWiseRule* rule, uint8_t pos);

// Verifica che una condizione di una finestra di una regola Ã¨ soddisfatta
uint8_t matchWindow(SdnWiseRuleWindow* window, uint8_t* packet);

// Verifica che un pacchetto corrisponda a una regola
uint8_t matchRule(SdnWiseRule* rule, uint8_t* packet);

// Esegue l'azione 
void runAction(SdnWiseRuleAction* action, uint8_t* packet);

// Esegue e verifica il risultato di un operazione espressa nella finestra
uint8_t doOperation(uint8_t operation, uint16_t item1, uint16_t item2);

// Verifica l'esistenza di una regola nella tabella
uint8_t searchRule(SdnWiseRule* rule);

// Cerca nella tabella e in caso esegue la action o invia un Type3
void runFlowMatch(uint8_t* packet);

// Converte l'indice richiesto nell'indice effettivo della flow table 
uint8_t getActualFlowIndex(uint8_t pos);

// Scrive la regola per inoltrare i pacchetti di controllo al sink
void writeRuleToSink(uint8_t sink_address_h, uint8_t sink_address_l, 
                     uint8_t action_h, uint8_t action_l);

void writeRuleToController(void);

/*------------------------------------------------------------------------------
/         Neighbors Table
/ ----------------------------------------------------------------------------*/

// Sceglie un vicino nel caso in cui non so a chi inviare
uint8_t chooseNeighbor(uint8_t action_value_2_byte);

// Restuitisce l'indice del vicino se gia presente in lista altrimenti MAX+1
int getNeighborIndex(uint16_t addr);

/*------------------------------------------------------------------------------
/         Accepted ID
/ ----------------------------------------------------------------------------*/

// cerca la destinazione del pacchetto tra gli id accettati, ne ritorna l'index
uint8_t searchAcceptedId(uint16_t addr);

// verifica se un ID è accettato dal nodo a partire dal pacchetto
uint8_t isAcceptedIdPacket(uint8_t* packet);

// verifica se un ID è accettato dal nodo a partire dall'indirizzo
uint8_t isAcceptedIdAddress(uint8_t addr_h,uint8_t addr_l);

/*------------------------------------------------------------------------------
/         Application
/ ----------------------------------------------------------------------------*/

// viene invoca ogni volta che un pacchetto è destinato al livello applicativo
void SDN_WISE_Callback(uint8_t* packet);

#endif