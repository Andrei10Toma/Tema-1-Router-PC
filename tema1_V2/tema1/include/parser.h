#pragma once
#include "skel.h"
#include "queue.h"

#define MASK 1

/**
 * structure of a cell from the route table
 * */
struct route_table_entry {
    uint32_t prefix;    /*Prefix of the to check*/
    uint32_t next_hop;  /*The ip of next hop*/
    uint32_t mask;      /*The mask to check the destination ip*/
    int interface;      /*The interface on which the packet will go*/
} __attribute__((packed));

/**
 * structure of a cell from the arp table
 * */ 
struct arp_table_entry {
    uint32_t ip;        /*The ip address*/
    uint8_t mac[6];     /*The corresponding mac address*/
} __attribute__((packed));

/**
 * structure of the arp table
 * */
struct arp_table {
    struct arp_table_entry* table;  /*Table containing the arp table entries*/
    unsigned int current_length;    /*The current length of the arp table*/
    unsigned int max_length;        /*The maximum length of the arp table*/
} __attribute__((packed));

/**
 * structure for the routing table
 * */
struct route_table {
    struct route_table_entry* table;    /*Table containing the route table entries*/
    unsigned int current_length;        /*The current length of the route table*/
    unsigned int max_length;            /*The maximum length of the route table*/
} __attribute__((packed));

/**
 * parse the route table from a file
 * @param filename - name of the file where the route table is located
 * @return the parsed route table
 * */
struct route_table** parse_route_table(char *filename);

/**
 * find the best route for the packet
 * @param rtable - the route table where the search will be done
 * @param destination - the destination ip
 * @return the best route found
 * */
struct route_table_entry *find_best_route(struct route_table **rtable, uint32_t destination);

/**
 * function for the sort of route table
 * @param a - the first element to compare (first route table entry)
 * @param b - the second element to compare (second route table entry)
 * @return <0 to move a before b; 0 if the values are equal; >0 to move a after b
 * */
int cmpfunction (const void *a, const void *b);

/**
 * initialize the arp table
 * @return pointer to the arp_table
 * */
struct arp_table* init_arp_table();

/**
 * search in the arp table
 * @param arp_table - the arp table where the search will be done
 * @param ip_addr - the ip address that is searched
 * @return the entry found in the arp table
 * */
struct arp_table_entry* search_arp_table(struct arp_table* arp_table, uint32_t ip_addr);

/**
 * add an entry in the arp table
 * @param arp_table - the arp table where the entry will be added
 * @param ip_addr - ip address
 * @param mac - mac address
 * */
void add_entry_arp_table(struct arp_table** arp_table, uint32_t ip_addr, uint8_t mac[6]);
