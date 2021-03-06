/*
* File:   sdn_wise.c
* Author: Sebastiano Milardo
*
* Created on 19 settembre 2013, 10.27
*/

#include <stdlib.h>
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "hal_uart.h"
#include "msa.h" 
#include "sdn_wise.h"
#include "hal_mcu.h"

// Timer
uint16_t cntBeacon = 0;
uint16_t cntReport = 0;
uint16_t cntUpdTable = 0;
#if NODE  // Generic Node
uint16_t cntData = 0;
uint16_t cntSleep = 0;
#endif


uint8_t num_hop_vs_sink;
uint8_t rssi_vs_sink;

uint8_t semaforo;

uint8_t battery;
uint8_t battery_next_hop;

SdnWiseNeighbor neighbor_table[SDN_WISE_NEIGHBORS_MAX];
uint16_t accepted_id[SDN_WISE_ACCEPTED_ID_MAX]; 
SdnWiseRule flow_table[SDN_WISE_RLS_MAX];
SdnWiseRule* rule_to_sink = &flow_table[0];

static const SdnWiseRuleAction empty_rule_action = {0,0,0,0};
static const SdnWiseRuleWindow empty_rule_window = {0,0,0,0};
static const SdnWiseRuleStats empty_rule_stats = {254,0};
static const SdnWiseNeighbor empty_neighbor = {255,255,255,255};

uint8_t MC_Status[7] = {0};
uint8_t statusRegister [SDN_WISE_STATUS_LEN] = {0};
uint8_t bufferInUSB [SDN_WISE_PACKET_LEN] = {0};

uint8_t packet_start_flg;
uint8_t receivedBytes;
uint8_t expectedBytes;
int8_t rssi;

uint8_t flow_table_free_pos;
uint8_t accepted_id_free_pos;
uint8_t neighbors_number;

Queue tx_queue;
Queue rx_queue;
Config config;

// App
uint8_t dataGestiti;
uint8_t lastTransmission;

// Interrupt
void timerInterrupt(void) {
  
#if NODE 
  if (semaforo) {
#endif
    
    cntBeacon++;
    cntReport++;
    cntUpdTable++;
#if NODE
    //    cntSleep++;
    //    cntData++;
#endif
    
    // in order to simulate a dynamic network we add a random reset to the node
    
    //if (rand() <= 2500) {
    //    HAL_SYSTEM_RESET();
    //}
    
    
    if ((cntBeacon) >= config.cnt_beacon_max) {
      cntBeacon = 0;
      MC_Status[SDN_WISE_TX_BEACON] = 1;
    }
    
    if ((cntReport) >= config.cnt_report_max) {
      cntReport = 0;
      MC_Status[SDN_WISE_TX_REPORT] = 1;
    }
    
    if ((cntUpdTable) >= config.cnt_updtable_max) {
      cntUpdTable = 0;
      MC_Status[SDN_WISE_UPDTABLE] = 1;
    }
    
    
#if NODE    
    if ((cntSleep) >= config.cnt_sleep_max) {
      cntSleep = 0;
    }
  }
#endif
}

// The beacon is sent in broadcast and contains as next hop the next hop towards the sink 
void txBEACON(void) {
  uint8_t packet[SDN_WISE_BEACON_HDR_LEN];
  packet[SDN_WISE_LEN] = SDN_WISE_BEACON_HDR_LEN;
  packet[SDN_WISE_NET_ID] = config.net_id;
  packet[SDN_WISE_SRC_H] = config.addr_h; 
  packet[SDN_WISE_SRC_L] = config.addr_l;
  packet[SDN_WISE_DST_H] = 255; // Broadcast
  packet[SDN_WISE_DST_L] = 255;
  packet[SDN_WISE_TYPE] = SDN_WISE_BEACON;
  packet[SDN_WISE_TTL] = config.ttl_max;
  
#if NODE
  packet[SDN_WISE_NXHOP_H] = rule_to_sink->window[0].value_h;
  packet[SDN_WISE_NXHOP_L] = rule_to_sink->window[0].value_l;
#else 
  packet[SDN_WISE_NXHOP_H] = config.addr_h;
  packet[SDN_WISE_NXHOP_L] = config.addr_l;
#endif  
  
  packet[SDN_WISE_DIST] = num_hop_vs_sink;
  packet[SDN_WISE_BATT] = battery;
  radioTX(packet,SDN_WISE_MAC_SEND_BROADCAST);
}

// neigh. table
void txREPORT(void) {
  int i = 0;
  int j = 0;
  
  uint8_t packet[16+SDN_WISE_RLS_MAX]; 
  packet[SDN_WISE_NET_ID] = config.net_id;
  packet[SDN_WISE_SRC_H] = config.addr_h;
  packet[SDN_WISE_SRC_L] = config.addr_l;
  packet[SDN_WISE_DST_H] = rule_to_sink->window[0].value_h; // sink
  packet[SDN_WISE_DST_L] = rule_to_sink->window[0].value_l;
  packet[SDN_WISE_TYPE] = SDN_WISE_REPORT;
  packet[SDN_WISE_TTL] = config.ttl_max;
  packet[SDN_WISE_NXHOP_H] = rule_to_sink->action.value_h;
  packet[SDN_WISE_NXHOP_L] = rule_to_sink->action.value_l; 
  packet[SDN_WISE_DIST] = num_hop_vs_sink;
  packet[SDN_WISE_BATT] = battery;
  packet[SDN_WISE_NEIGH] = neighbors_number; // size
  packet[SDN_WISE_LEN] = (neighbors_number*3) + SDN_WISE_REPORT_HDR_LEN;
  
  for (j = 0; j < packet[SDN_WISE_NEIGH]; j++) {
    //neigh i
    packet[SDN_WISE_REPORT_HDR_LEN + i++] = neighbor_table[j].addr_h;
    packet[SDN_WISE_REPORT_HDR_LEN + i++] = neighbor_table[j].addr_l;
    //RSSI
    packet[SDN_WISE_REPORT_HDR_LEN + i++] = neighbor_table[j].rssi;
  } 
  initNeighborTable();
  
#if NODE
  radioTX(packet,SDN_WISE_MAC_SEND_UNICAST);
#else
  controllerTX(packet);
#endif
}

// App layer
void txDATA(void) {
  uint8_t packet[11];
  packet[SDN_WISE_LEN] = 11;
  packet[SDN_WISE_NET_ID] = config.net_id;
  packet[SDN_WISE_SRC_H] = config.addr_h;
  packet[SDN_WISE_SRC_L] = config.addr_l;
  packet[SDN_WISE_DST_H] = rule_to_sink->window[0].value_h;
  packet[SDN_WISE_DST_L] = rule_to_sink->window[0].value_l;
  packet[SDN_WISE_TYPE] = SDN_WISE_DATA;
  packet[SDN_WISE_TTL] = config.ttl_max;
  packet[SDN_WISE_NXHOP_H] = rule_to_sink->action.value_h;
  packet[SDN_WISE_NXHOP_L] = rule_to_sink->action.value_l; 
  /* 
  reading
  */
  packet[10] = 111;
  
  runFlowMatch(packet);
}

// When the TTL reaches 0 the entry is deleted
void updateTable(void) {
  uint8_t i = 0;
  for (i = 0; i < SDN_WISE_RLS_MAX; i++) {
    if (flow_table[i].stats.ttl != SDN_WISE_PERMANENT_RULE){
      if (flow_table[i].stats.ttl >= SDN_WISE_RL_TTL_DECR) {
        flow_table[i].stats.ttl -= SDN_WISE_RL_TTL_DECR;
      } else {
        initRule(&flow_table[i]);
#if NODE
        if (i == 0) {
          semaforo = 0;
          num_hop_vs_sink = 255;
        }
#endif
      }
    }
  }
}

// incoming packets
void rxHandler(uint8_t* packet, uint8_t rssi) {  
  if (packet[SDN_WISE_LEN] > SDN_WISE_DFLT_HDR_LEN && 
      packet[SDN_WISE_NET_ID] == config.net_id &&
        packet[SDN_WISE_TTL])
  {
    
    switch (packet[SDN_WISE_TYPE]) {
    case SDN_WISE_DATA:
#if TRACE_DELAY    
      if (packet[SDN_WISE_LEN]>14){
        if (packet[SDN_WISE_SRC_H] == 0 && packet[SDN_WISE_SRC_L] == 0){
          if (packet[10] == 0){
            packet[10] = 1;
            packet[11] = T1CNTL;
            packet[12] = T1CNTH;
          }
        } else {
          packet[13] = T1CNTL;
          packet[14] = T1CNTH;
        }
      }
#endif
      rxDATA(packet);
      break;
      
    case SDN_WISE_BEACON: 
      rxBEACON(packet, rssi);
      break;
      
    case SDN_WISE_RESPONSE:      
      rxRESPONSE(packet);
      break;
      
    case SDN_WISE_OPEN_PATH: 
      rxOPEN_PATH(packet);
      break;
      
    case SDN_WISE_CONFIG: 
      rxCONFIG(packet);
      break;
      
    default:
      rxREPORT(packet);
      break;
    } 
  }
}

/*-----------------------------------------------------------------------------
/               Packets coming from/going to the sink           
/ ----------------------------------------------------------------------------*/
void rxDATA(uint8_t* packet) {
  if (isAcceptedIdPacket(packet))
  {
    SDN_WISE_Callback(packet);
  } else if (isAcceptedIdAddress(packet[SDN_WISE_NXHOP_H],
                                 packet[SDN_WISE_NXHOP_L])){
                                   runFlowMatch(packet);
                                 }
}

/*-----------------------------------------------------------------------------
/                               Beacon
/ ----------------------------------------------------------------------------*/
void rxBEACON(uint8_t* packet, uint8_t rssi) {
  
  if (rssi > config.rssi_min){
    int8_t index;
#if NODE
    
    semaforo = 1;
    // a node can send a packet only if semaforo==1
    // check the distance from the sink
    if (packet[SDN_WISE_DIST] < num_hop_vs_sink && (rssi >= rssi_vs_sink)) 
    { 
      //is the sender has a smaller distance, select it as next_hop
      writeRuleToSink(packet[SDN_WISE_NXHOP_H],packet[SDN_WISE_NXHOP_L],
                      packet[SDN_WISE_SRC_H],packet[SDN_WISE_SRC_L]);
      num_hop_vs_sink = packet[SDN_WISE_DIST] + 1; 
      
    } else if (packet[SDN_WISE_DIST] == num_hop_vs_sink && 
               rule_to_sink->action.value_h == packet[SDN_WISE_SRC_H] &&
                 rule_to_sink->action.value_l == packet[SDN_WISE_SRC_L])
    {// Restore TTL
      rule_to_sink->stats.ttl = 254;             
    }
#endif
    // Learn the existence of one hop distance neigh.
    // It can return:
    // SDN_WISE_RLS_MAX + 1 if the list is full
    // -1 if empty
    index = getNeighborIndex(MERGE_BYTES(packet[SDN_WISE_SRC_H],
                                         packet[SDN_WISE_SRC_L]));
    if (index != SDN_WISE_NEIGHBORS_MAX + 1) {
      if (index != -1){
        neighbor_table[index].rssi = rssi;
        neighbor_table[index].batt = packet[SDN_WISE_BATT];
      }else{      
        neighbor_table[neighbors_number].addr_h = packet[SDN_WISE_SRC_H];
        neighbor_table[neighbors_number].addr_l = packet[SDN_WISE_SRC_L];
        neighbor_table[neighbors_number].rssi = rssi;
        neighbor_table[neighbors_number].batt = packet[SDN_WISE_BATT];
        neighbors_number++;
      }
    }
  } 
}

/*-----------------------------------------------------------------------------
/                       Report - Request
/ ----------------------------------------------------------------------------*/
void rxREPORT(uint8_t* packet) {
#if NODE
  runFlowMatch(packet);  
#else
  controllerTX(packet);
#endif
}

/*-----------------------------------------------------------------------------
/                     Rule/Action Response 
/ ----------------------------------------------------------------------------*/
// Sent in unicast, forwarded by the nodes read by the destination
void rxRESPONSE(uint8_t* packet) {
  uint8_t i = 0;
  if (isAcceptedIdPacket(packet))
  {
    // Check the rule in the table
    uint8_t nRules = 
      (packet[SDN_WISE_LEN]-SDN_WISE_RESPONSE_HDR_LEN)/(SDN_WISE_RL_BODY_LEN);
    
    if ((nRules * SDN_WISE_RL_BODY_LEN)+SDN_WISE_RESPONSE_HDR_LEN == 
        packet[SDN_WISE_LEN]){
          
          for (i = 0; i<nRules; i++){
            SdnWiseRule rule;
            uint8_t i;
            uint8_t packet_index = SDN_WISE_RESPONSE_HDR_LEN;
            
            for (i=0;i<SDN_WISE_WINDOWS_MAX;i++){
              rule.window[i].op = packet[packet_index++];
              rule.window[i].pos = packet[packet_index++];
              rule.window[i].value_h = packet[packet_index++];
              rule.window[i].value_l = packet[packet_index++];
            }
            
            rule.action.act = packet[packet_index++];
            rule.action.pos = packet[packet_index++];
            rule.action.value_h = packet[packet_index++];
            rule.action.value_l = packet[packet_index++];
            
            rule.stats.ttl = packet[packet_index++];
            rule.stats.count = 0;
            
            insertRule(&rule, searchRule(&rule));
          }
        }
  } else {
    runFlowMatch(packet);
  }
}

/*-----------------------------------------------------------------------------
/                      Open Path
/ ----------------------------------------------------------------------------*/
void rxOPEN_PATH(uint8_t* packet) {
  if (isAcceptedIdPacket(packet))
  { 
    uint8_t i;
    
    i = SDN_WISE_OPEN_PATH_HDR_LEN;  
    
    while (i < packet[SDN_WISE_LEN]) {
      if (isAcceptedIdAddress(packet[i], packet[i + 1])){
        
        
        uint16_t last_path_addr = MERGE_BYTES(packet[packet[SDN_WISE_LEN] - 2], 
                                              packet[packet[SDN_WISE_LEN] - 1]);
        
        uint16_t first_path_addr = MERGE_BYTES(packet[SDN_WISE_OPEN_PATH_HDR_LEN], 
                                               packet[SDN_WISE_OPEN_PATH_HDR_LEN+1]);
        
        uint16_t actual_address = MERGE_BYTES(packet[i], packet[i + 1]);
        
        // se non sono il primo
        if (actual_address != first_path_addr){
          SdnWiseRule rule;
          initRule(&rule);
          
          // back
          rule.window[0].op = SDN_WISE_EQUAL|SDN_WISE_SIZE_2|SDN_WISE_PACKET;
          rule.window[0].pos = SDN_WISE_DST_H;
          rule.window[0].value_h = HIGH_BYTE(first_path_addr);
          rule.window[0].value_l = LOW_BYTE(first_path_addr);
          
          rule.action.act = SDN_WISE_FORWARD_UNICAST | SDN_WISE_PACKET;
          rule.action.pos = SDN_WISE_NXHOP_H;
          rule.action.value_h = packet[i - 2];
          rule.action.value_l = packet[i - 1];
          //e la imparo
          uint8_t p = searchRule(&rule);
          insertRule(&rule, p);
        }
        
        // if I'm not the last one 
        if (actual_address != last_path_addr){
          SdnWiseRule rule;
          initRule(&rule);
          
          // forward
          rule.window[0].op = SDN_WISE_EQUAL|SDN_WISE_SIZE_2|SDN_WISE_PACKET;
          rule.window[0].pos = SDN_WISE_DST_H; 
          rule.window[0].value_h = HIGH_BYTE(last_path_addr); 
          rule.window[0].value_l = LOW_BYTE(last_path_addr); 
          
          rule.action.act = SDN_WISE_FORWARD_UNICAST | SDN_WISE_PACKET;
          rule.action.pos = SDN_WISE_NXHOP_H; 
          rule.action.value_h = packet[i + 2]; 
          rule.action.value_l = packet[i + 3]; 
          uint8_t p = searchRule(&rule);
          insertRule(&rule, p);
          
          // change next hop
          packet[SDN_WISE_DST_H] = packet[i + 2];                       
          packet[SDN_WISE_DST_L] = packet[i + 3];  
          packet[SDN_WISE_NXHOP_H] = packet[i + 2];                       
          packet[SDN_WISE_NXHOP_L] = packet[i + 3];  
          // and forwardo  
          radioTX(packet,SDN_WISE_MAC_SEND_UNICAST);
          break;
        } 
      }
      i += 2;
    }
  } else {
    runFlowMatch(packet);
  }
}

/*-----------------------------------------------------------------------------
/                      Config
/ ----------------------------------------------------------------------------*/
void rxCONFIG(uint8_t* packet) {
  uint16_t dest = MERGE_BYTES(packet[SDN_WISE_DST_H], packet[SDN_WISE_DST_L]);
#if !NODE
  uint16_t src = MERGE_BYTES(packet[SDN_WISE_SRC_H], packet[SDN_WISE_SRC_L]);
#endif  
  if (dest != config.addr) {
    runFlowMatch(packet);
  } else { 
#if !NODE
    if (src == dest){
#endif    
      uint8_t toBeSent = 0;
      uint8_t nValues = (packet[SDN_WISE_LEN] - SDN_WISE_CONFIG_HDR_LEN)/3;
      uint8_t i, pos;
      uint8_t ii = 1;
      uint8_t jj = 0;
      
      
      for (i = 0; i < nValues; i++){
        uint8_t isWrite = GET_BIT_(packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)],7);
        uint8_t id = GET_BIT_RANGE(packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)],0,7);
        uint16_t value = MERGE_BYTES(packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+1],
                                     packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+2]);  
        if (isWrite){
          switch (id){
          case SDN_WISE_CNF_ID_ADDR:
            config.addr_h = packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+1];
            config.addr_l = packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+2];
            config.addr = value;
            break;
          case SDN_WISE_CNF_ID_NET_ID:
            config.net_id = packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+2];
            break;
          case SDN_WISE_CNF_ID_CNT_BEACON_MAX:
            config.cnt_beacon_max = value;
            break;
          case SDN_WISE_CNF_ID_CNT_REPORT_MAX:
            config.cnt_report_max = value;
            break;
          case SDN_WISE_CNF_ID_CNT_UPDTABLE_MAX:
            config.cnt_updtable_max = value;
            break;
          case SDN_WISE_CNF_ID_CNT_SLEEP_MAX:
            config.cnt_sleep_max = value;
            break;
          case SDN_WISE_CNF_ID_TTL_MAX:
            config.ttl_max = packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+2];
            break;
          case SDN_WISE_CNF_ID_RSSI_MIN:
            config.rssi_min = packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+2];
            break;
          case SDN_WISE_CNF_ADD_ACCEPTED:
            pos = searchAcceptedId(value);
            if (pos == (SDN_WISE_ACCEPTED_ID_MAX + 1)){
              pos = searchAcceptedId(65535);
              accepted_id[pos] = value;
            } 
            break;
          case SDN_WISE_CNF_REMOVE_ACCEPTED:
            pos = searchAcceptedId(value);
            if (pos != (SDN_WISE_ACCEPTED_ID_MAX + 1)){
              accepted_id[pos] = 65535;
            } 
            break;         
          case SDN_WISE_CNF_REMOVE_RULE_INDEX:
            if (value){
              initRule(&flow_table[getActualFlowIndex(value)]);
            }
            break;  
          case SDN_WISE_CNF_REMOVE_RULE:
            // TODO 
            break;  
          }
        }else{
          toBeSent = 1;
          uint8_t packetList[SDN_WISE_CONFIG_HDR_LEN+SDN_WISE_ACCEPTED_ID_MAX*2]; // len
          uint8_t iii;
          switch (id){
          case SDN_WISE_CNF_ID_ADDR:
            packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+1] = config.addr_h;
            packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+2] = config.addr_l;
            break;
          case SDN_WISE_CNF_ID_NET_ID:
            packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+2] = config.net_id;
            break;
          case SDN_WISE_CNF_ID_CNT_BEACON_MAX:
            packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+1] = HIGH_BYTE(config.cnt_beacon_max);
            packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+2] = LOW_BYTE(config.cnt_beacon_max);
            break;
          case SDN_WISE_CNF_ID_CNT_REPORT_MAX:
            packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+1] = HIGH_BYTE(config.cnt_report_max);
            packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+2] = LOW_BYTE(config.cnt_report_max);
            break;
          case SDN_WISE_CNF_ID_CNT_UPDTABLE_MAX:
            packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+1] = HIGH_BYTE(config.cnt_updtable_max);
            packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+2] = LOW_BYTE(config.cnt_updtable_max);
            break;
          case SDN_WISE_CNF_ID_CNT_SLEEP_MAX:
            packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+1] = HIGH_BYTE(config.cnt_sleep_max);
            packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+2] = LOW_BYTE(config.cnt_sleep_max);
            break;
          case SDN_WISE_CNF_ID_TTL_MAX:
            packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+2] = config.ttl_max;
            break;
          case SDN_WISE_CNF_ID_RSSI_MIN:
            packet[SDN_WISE_CONFIG_HDR_LEN+(i*3)+2] = config.rssi_min;
            break;
          case SDN_WISE_CNF_LIST_ACCEPTED:
            
            toBeSent = 0;
            packetList[SDN_WISE_NET_ID] = config.net_id;
            packetList[SDN_WISE_SRC_H] = packet[SDN_WISE_DST_H];
            packetList[SDN_WISE_SRC_L] = packet[SDN_WISE_DST_L];
            packetList[SDN_WISE_DST_H] = packet[SDN_WISE_SRC_H];
            packetList[SDN_WISE_DST_L] = packet[SDN_WISE_SRC_L];
            packetList[SDN_WISE_TYPE] = packet[SDN_WISE_TYPE];
            packetList[SDN_WISE_TTL] = config.ttl_max;
            packetList[SDN_WISE_NXHOP_H] = rule_to_sink->action.value_h;
            packetList[SDN_WISE_NXHOP_L] = rule_to_sink->action.value_l; 
            
            packetList[SDN_WISE_CONFIG_HDR_LEN] = SDN_WISE_CNF_LIST_ACCEPTED;
            
            for (jj = 0; jj < SDN_WISE_ACCEPTED_ID_MAX; jj++) {
              if (accepted_id[jj] != 65535){
                packetList[SDN_WISE_CONFIG_HDR_LEN + ii++] = 
                  HIGH_BYTE(accepted_id[jj]);
                packetList[SDN_WISE_CONFIG_HDR_LEN + ii++] = 
                  LOW_BYTE(accepted_id[jj]);
              }
            }
            packetList[SDN_WISE_LEN] = ii + SDN_WISE_CONFIG_HDR_LEN;          
#if NODE
            radioTX(packetList,SDN_WISE_MAC_SEND_UNICAST);
#else
            controllerTX(packetList);
#endif        
            break;
          case SDN_WISE_CNF_LIST_RULE:
            toBeSent = 0;
            
            packetList[SDN_WISE_NET_ID] = config.net_id;
            packetList[SDN_WISE_SRC_H] = packet[SDN_WISE_DST_H];
            packetList[SDN_WISE_SRC_L] = packet[SDN_WISE_DST_L];
            packetList[SDN_WISE_DST_H] = packet[SDN_WISE_SRC_H];
            packetList[SDN_WISE_DST_L] = packet[SDN_WISE_SRC_L];
            packetList[SDN_WISE_TYPE] = packet[SDN_WISE_TYPE];
            packetList[SDN_WISE_TTL] = config.ttl_max;
            packetList[SDN_WISE_NXHOP_H] = rule_to_sink->action.value_h;
            packetList[SDN_WISE_NXHOP_L] = rule_to_sink->action.value_l; 
            
            packetList[SDN_WISE_CONFIG_HDR_LEN] = SDN_WISE_CNF_LIST_RULE;
            
            ii = SDN_WISE_CONFIG_HDR_LEN;
            packetList[ii] = packet[ii];
            ii++;
            packetList[ii] = packet[ii];
            ii++;
            
            jj = getActualFlowIndex(packet[ii]);
            packetList[ii] = packet[ii];
            ii++;
            
            for (iii=0;iii<SDN_WISE_WINDOWS_MAX;iii++){
              packetList[ii++] = flow_table[jj].window[iii].op;
              packetList[ii++] = flow_table[jj].window[iii].pos;
              packetList[ii++] = flow_table[jj].window[iii].value_h;
              packetList[ii++] = flow_table[jj].window[iii].value_l;
            }
            
            packetList[ii++] = flow_table[jj].action.act;
            packetList[ii++] = flow_table[jj].action.pos;
            packetList[ii++] = flow_table[jj].action.value_h;
            packetList[ii++] = flow_table[jj].action.value_l;
            
            packetList[ii++] = flow_table[jj].stats.ttl;
            packetList[ii++] = flow_table[jj].stats.count;
            
            packetList[SDN_WISE_LEN] = ii;          
#if NODE
            radioTX(packetList,SDN_WISE_MAC_SEND_UNICAST);
#else
            controllerTX(packetList);
#endif        
            
            break;
            
          }        
        }
      }
      
      if (toBeSent){     
        packet[SDN_WISE_SRC_H] = config.addr_h;
        packet[SDN_WISE_SRC_L] = config.addr_l;
        packet[SDN_WISE_DST_H] = rule_to_sink->window[0].value_h;
        packet[SDN_WISE_DST_L] = rule_to_sink->window[0].value_l;
        packet[SDN_WISE_TTL] = config.ttl_max;
        packet[SDN_WISE_NXHOP_H] = rule_to_sink->action.value_h;
        packet[SDN_WISE_NXHOP_L] = rule_to_sink->action.value_l;
#if NODE
        runFlowMatch(packet); 
#else
        controllerTX(packet);
#endif      
      }    
#if !NODE
    }else{
      controllerTX(packet);
    }
#endif
  }
}

// Init
void SDN_WISE_Init(void) {
  config.cnt_beacon_max = SDN_WISE_DFLT_CNT_BEACON_MAX;
  config.cnt_report_max = SDN_WISE_DFLT_CNT_REPORT_MAX;
  config.cnt_updtable_max = SDN_WISE_DFLT_CNT_UPDTABLE_MAX;
  config.addr = SDN_WISE_DFLT_ADDR;
  config.addr_h = HIGH_BYTE(SDN_WISE_DFLT_ADDR);
  config.addr_l = LOW_BYTE(SDN_WISE_DFLT_ADDR);
  
  config.net_id = SDN_WISE_DFLT_NET_ID;
  config.rssi_min = SDN_WISE_DFLT_RSSI_MIN;
  config.ttl_max = SDN_WISE_DFLT_TTL_MAX;
  
  battery = 255;
  flow_table_free_pos = 1;
  accepted_id_free_pos = 0;
  
  uint8_t i;
  for (i = 0; i< SDN_WISE_ACCEPTED_ID_MAX; i++){
    accepted_id[i] = 65535;
  }
  
#if NODE 
  semaforo = 0;
  num_hop_vs_sink = config.ttl_max + 1; // hop equal to max+1
  rssi_vs_sink = 0;
  config.cnt_sleep_max = SDN_WISE_DFLT_CNT_SLEEP_MAX;
#else
  num_hop_vs_sink = 0; // hop if sink=0
  rssi_vs_sink = 255;
#endif
  
  initNeighborTable();
  initFlowTable();
  
  packet_start_flg = 0;
  receivedBytes = 0;
  expectedBytes = 0;
  dataGestiti = 0;
  lastTransmission = 0;
  
  tx_queue.head = NULL;
  tx_queue.tail = NULL;
  tx_queue.size = 0;
  
  rx_queue.head = NULL;
  rx_queue.tail = NULL;
  rx_queue.size = 0;
}

// Random neigh. if there is no other option
uint8_t chooseNeighbor(uint8_t action_value_2_byte) {
  uint8_t i;
  for (i = 0; i < SDN_WISE_NEIGHBORS_MAX; i++) {
    if (action_value_2_byte == neighbor_table[i].addr_l)
      return (neighbor_table[i].addr_h);
  }
  // error
  return 254;
}

// adds a rule in the table
void insertRule(SdnWiseRule* rule, uint8_t pos){
  if (pos >= SDN_WISE_RLS_MAX) {
    pos = flow_table_free_pos;
    flow_table_free_pos++;
    if (flow_table_free_pos >= SDN_WISE_RLS_MAX) {
      flow_table_free_pos = 1;
    }
  }
  flow_table[pos] = *rule;
}

// check a condition in a window
uint8_t matchWindow(SdnWiseRuleWindow* window, uint8_t* packet){
  
  uint8_t size = GET_RL_WINDOW_SIZE(window->op);
  uint8_t operatore = GET_RL_WINDOW_OPERATOR(window->op);
  uint8_t matchMemory = GET_RL_WINDOW_LOCATION(window->op);
  
  uint8_t* ptr;
  
  if (matchMemory){
    ptr = packet;
  } else {
    ptr = statusRegister;  
  }
  
  switch (size) {
  case SDN_WISE_SIZE_2:
    return doOperation(operatore,
                       MERGE_BYTES(ptr[window->pos], ptr[window->pos + 1]),
                       MERGE_BYTES(window->value_h,window->value_l));
  case SDN_WISE_SIZE_1:
    return doOperation(operatore,
                       ptr[window->pos],
                       window->value_l);
  case SDN_WISE_SIZE_0:
    return 2;
  default:
    return 0;
  }
}

// match a packet
uint8_t matchRule(SdnWiseRule* rule, uint8_t* packet){
  uint8_t i;
  uint8_t sum = 0;
  for (i = 0; i<SDN_WISE_WINDOWS_MAX;i++){
    uint8_t result = matchWindow(&(rule->window[i]), packet);
    if (result)
      sum += result;
    else
      return 0;
  }
  
  return (sum == SDN_WISE_WINDOWS_MAX*2 ? 0 : 1);
}

// run the action
void runAction(SdnWiseRuleAction* action, uint8_t* packet){
  
  
  uint8_t action_type = GET_RL_ACTION_TYPE(action->act);;
  uint8_t action_location = GET_RL_ACTION_LOCATION(action->act);
  
  
  switch (action_type) {
  case SDN_WISE_FORWARD_UNICAST: 
  case SDN_WISE_FORWARD_BROADCAST:  
    {
      packet[SDN_WISE_TTL]--;
      packet[action->pos] = action->value_h;
      packet[action->pos + 1] = action->value_l;
      
#if TRACE_PATH
      uint8_t i; 
      if (packet[SDN_WISE_LEN] >= 30){
        for (i = 20; i<30; i++){  
          if (packet[SDN_WISE_TYPE]==DATA && packet[i] == 255){
            packet[i] = packet[SDN_WISE_NXHOP_L];
            break;
          }
        }
      }
#endif   
      radioTX(packet,action_type ^ SDN_WISE_FORWARD_UNICAST);    
      break;
    }
  case SDN_WISE_DROP: 
    {
      uint8_t prob = action->value_h;
      if (rand() % 101 <= prob) {
        return;
      } else {
        uint8_t first_byte_address = chooseNeighbor(action->value_l);
        if (first_byte_address == 254) {
          return;
        } else {
          packet[SDN_WISE_TTL]--;
          packet[SDN_WISE_NXHOP_H] = first_byte_address;
          packet[SDN_WISE_NXHOP_L] = action->value_h;
          radioTX(packet,SDN_WISE_MAC_SEND_UNICAST);
        }
      }
      break;
    }
  case SDN_WISE_MODIFY: 
    {
      if (action_location){
        
        uint8_t tmpAct1 = packet[action->pos];
        uint8_t tmpAct2 = packet[action->pos + 1];
        packet[action->pos] = action->value_h;
        packet[action->pos + 1] = action->value_l;
        
        queue_add_element(&rx_queue,packet,255,SDN_WISE_MAC_SEND_UNICAST);
        osal_start_timerEx(MSA_TaskId, SDN_WISE_REC_EVENT, 0);
        
        packet[action->pos] = tmpAct1;
        packet[action->pos + 1] = tmpAct2;
        
      } else {    
        
        statusRegister[action->pos] = action->value_h;
        statusRegister[action->pos + 1] = action->value_l;
        queue_add_element(&rx_queue,packet,255,SDN_WISE_MAC_SEND_UNICAST);
        osal_start_timerEx(MSA_TaskId, SDN_WISE_REC_EVENT, 0);
        
      }
      break;
    }
  case SDN_WISE_AGGREGATE: 
    {
      break;
    }
    
  case SDN_WISE_FORWARD_UP:
    {
      SDN_WISE_Callback(packet);
    }
  }
  
}

// Search in the table or send a Type3
void runFlowMatch(uint8_t* packet){
  
  uint8_t j,i, found;
  found = 0;
  
  for (j = 0; j < SDN_WISE_RLS_MAX; j++) {
    
    i = getActualFlowIndex(j);
    
    if (matchRule(&(flow_table[i]), packet) == 1) {
      found = 1;
      runAction(&(flow_table[i].action), packet);
      flow_table[i].stats.count++;
      if (!GET_RL_ACTION_IS_MULTI(flow_table[i].action.act)) {
        break;
      }
    }
  }
  
  if (!found) {
    // It's necessary to send a rule/request if we have done the lookup
    // I must modify the source address with myself,
    packet[SDN_WISE_SRC_H] = config.addr_h;
    packet[SDN_WISE_SRC_L] = config.addr_l;
    
    packet[SDN_WISE_TYPE] += 128;
    packet[SDN_WISE_TTL] = config.ttl_max;
    // The next hop at the moment will be the best hop to reach the sink
    // So, in this case forward the packet
    packet[SDN_WISE_NXHOP_H] = rule_to_sink->action.value_h;
    packet[SDN_WISE_NXHOP_L] = rule_to_sink->action.value_l;
    // Send Rule/Request packet
    
#if NODE
    radioTX(packet,SDN_WISE_MAC_SEND_UNICAST);
#else
    controllerTX(packet);
#endif   
  }
}


uint8_t doOperation(uint8_t operatore, uint16_t item1, uint16_t item2) {
  switch (operatore) {
  case SDN_WISE_EQUAL:
    return item1 == item2;
  case SDN_WISE_NOT_EQUAL:
    return item1 != item2;
  case SDN_WISE_BIGGER:
    return item1 > item2;
  case SDN_WISE_LESS:
    return item1 < item2;
  case SDN_WISE_EQUAL_OR_BIGGER:
    return item1 >= item2;
  case SDN_WISE_EQUAL_OR_LESS:
    return item1 <= item2;
  default:
    return 0;
  }
}


// search for a rule in the table
uint8_t searchRule(SdnWiseRule* rule){
  
  uint8_t i;
  uint8_t j;
  uint8_t sum;
  uint8_t target;
  
  for (i = 0; i < SDN_WISE_RLS_MAX; i++) {
    sum = 0;
    target = SDN_WISE_WINDOWS_MAX;
    
    for (j = 0; j< SDN_WISE_WINDOWS_MAX; j++){
      if (flow_table[i].window[j].op == rule->window[j].op &&
          flow_table[i].window[j].pos == rule->window[j].pos &&
            flow_table[i].window[j].value_h == rule->window[j].value_h &&
              flow_table[i].window[j].value_l == rule->window[j].value_l)
      {
        sum++;
      }      
    }
    
    if (GET_RL_ACTION_IS_MULTI(rule->action.act)){
      target++;
      
      if (flow_table[i].action.act == rule->action.act &&
          flow_table[i].action.pos == rule->action.pos &&
            flow_table[i].action.value_h == rule->action.value_h &&
              flow_table[i].action.value_l == rule->action.value_l)
      {
        sum++;
      }
    }
    
    if (sum == target)
      return i;
  }
  return SDN_WISE_RLS_MAX + 1;
}



// init the Flowtable
void initFlowTable(void) {
  int i;
#if !NODE
  writeRuleToController();
#endif  
  for (i = 1; i < SDN_WISE_RLS_MAX; i++) {
    initRule(&flow_table[i]);
  }
}

void 
writeRuleToSink(uint8_t sink_address_h, uint8_t sink_address_l, 
                uint8_t action_h, uint8_t action_l){
  uint8_t k;
                  
  rule_to_sink->window[0].op = SDN_WISE_EQUAL|SDN_WISE_SIZE_2|SDN_WISE_PACKET;
  rule_to_sink->window[0].pos = SDN_WISE_DST_H;
  rule_to_sink->window[0].value_h = sink_address_h;
  rule_to_sink->window[0].value_l = sink_address_l;
  
  rule_to_sink->window[1].op = SDN_WISE_NOT_EQUAL|SDN_WISE_SIZE_1|SDN_WISE_PACKET;
  rule_to_sink->window[1].pos = SDN_WISE_TYPE;
  rule_to_sink->window[1].value_h  = 0;
  rule_to_sink->window[1].value_l  = 0;
  
  for (k = 2; k<SDN_WISE_WINDOWS_MAX;k++){
    rule_to_sink->window[k] = empty_rule_window;
  }
   
  rule_to_sink->action.act = SDN_WISE_FORWARD_UNICAST | SDN_WISE_PACKET;
  rule_to_sink->action.pos = SDN_WISE_NXHOP_H;
  rule_to_sink->action.value_h = action_h;
  rule_to_sink->action.value_l = action_l;  
  
  rule_to_sink->stats = empty_rule_stats;
}

void 
writeRuleToController(){
  uint8_t k;
                  
  rule_to_sink->window[0].op = SDN_WISE_EQUAL|SDN_WISE_SIZE_2|SDN_WISE_PACKET;
  rule_to_sink->window[0].pos = SDN_WISE_DST_H;
  rule_to_sink->window[0].value_h = config.addr_h;
  rule_to_sink->window[0].value_l = config.addr_l;
  
  for (k = 1; k<SDN_WISE_WINDOWS_MAX;k++){
    rule_to_sink->window[k] = empty_rule_window;
  }
   
  rule_to_sink->action.act = SDN_WISE_FORWARD_UP;
  rule_to_sink->stats.ttl = SDN_WISE_PERMANENT_RULE;
  rule_to_sink->stats.count = 0;
}

// init neigh. list
void initNeighborTable(void) {
  uint8_t i;
  for (i = 0; i<SDN_WISE_NEIGHBORS_MAX; i++){
    neighbor_table[i] = empty_neighbor;
  }
  neighbors_number = 0;
}

// returns the index of a neigh. otherwise MAX+1 if full -1 if there is space
int getNeighborIndex(uint16_t addr) {
  int i;
  uint16_t tmp_addr;
  for (i = 0; i < SDN_WISE_NEIGHBORS_MAX; i++) {
    tmp_addr = MERGE_BYTES(neighbor_table[i].addr_h, neighbor_table[i].addr_l); 
    
    if ( tmp_addr == addr)
      return i;
    if (tmp_addr == 65535)
      return -1;
  }
  return SDN_WISE_NEIGHBORS_MAX + 1;
}

// enqueue a packet to be send
void radioTX(uint8_t packet[SDN_WISE_PACKET_LEN], uint8_t is_multicast) {
  
  queue_add_element(&tx_queue, packet, 255, is_multicast);
  osal_start_timerEx(MSA_TaskId, SDN_WISE_SEND_EVENT, 0);
}

// send using the serial port

void controllerTX(uint8_t* packet) {
  // WARNING: START and STOP bytes should be added in HalUARTWRite
  HalUARTWrite(HAL_UART_PORT_0, packet, packet[SDN_WISE_LEN]);
}

// init a rule
void initRule(SdnWiseRule* rule){
  uint8_t i;
  for (i = 0; i<SDN_WISE_WINDOWS_MAX;i++)
    rule->window[i] = empty_rule_window;
  rule->action = empty_rule_action;
  rule->stats = empty_rule_stats;
}

void SDN_WISE_Callback(uint8_t* packet){
#if NODE   
  //echo    
  packet[SDN_WISE_DST_H] = packet[SDN_WISE_SRC_H];
  packet[SDN_WISE_DST_L] = packet[SDN_WISE_SRC_L];
  packet[SDN_WISE_SRC_H] = config.addr_h;
  packet[SDN_WISE_SRC_L] = config.addr_l;  
  packet[SDN_WISE_TTL] = config.ttl_max;   
  runFlowMatch(packet);
#else
  controllerTX(packet);
#endif  
}

uint8_t searchAcceptedId(uint16_t addr){
  uint8_t i;
  for (i = 0; i< SDN_WISE_ACCEPTED_ID_MAX; i++){
    if (accepted_id[i] == addr){ 
      return i; 
    }
  }
  return SDN_WISE_ACCEPTED_ID_MAX + 1;
}

uint8_t isAcceptedIdAddress(uint8_t addr_h, uint8_t addr_l){
  if (((addr_h == config.addr_h && addr_l == config.addr_l) ||
       (addr_h == 255 && addr_l == 255) ||
         (searchAcceptedId(MERGE_BYTES(addr_h,addr_l)) != 
          SDN_WISE_ACCEPTED_ID_MAX + 1))) 
    return 1;
  else
    return 0; 
}

uint8_t isAcceptedIdPacket(uint8_t* packet){
  return isAcceptedIdAddress(packet[SDN_WISE_DST_H],packet[SDN_WISE_DST_L]);
}

uint8_t getActualFlowIndex(uint8_t j){
  //j = j % SDN_WISE_RLS_MAX;
  int16_t i;
  if (!j){
    i = 0;
  } else {          
    i = flow_table_free_pos - j;
    if (i == 0){
      i = SDN_WISE_RLS_MAX-1;
    } else if (i < 0){
      i = SDN_WISE_RLS_MAX -1 + i;
    }              
  }
  return i;
}


