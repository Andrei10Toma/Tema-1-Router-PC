#include "parser.h"

void print_ip(uint32_t ip) {
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;   
    printf("%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
}

void print_mac(uint8_t mac[6]) {
    for (int i = 0; i < 6; i++) {
        if (i != 5) printf("%x:", mac[i]);
        else printf("%x", mac[i]);
    }
    printf("\n");
}

void print_route_table (struct route_table* rtable) {
    for (int i = 0; i < rtable->current_length; i++) {
        print_ip(rtable->table[i].prefix);
        printf(" ");
        print_ip(rtable->table[i].next_hop);
        printf(" ");
        print_ip(rtable->table[i].mask);
        printf(" ");
        printf("%d\n", rtable->table[i].interface);
    }
}

struct route_table* parse_route_table(char *filename) {
    char prefix[16];
    char next_hop[16];
    char mask[16];
    int interface;
    struct route_table* route_table = (struct route_table*) calloc(1, sizeof(struct route_table));
    if (route_table == NULL) exit(-1);
    route_table->table = (struct route_table_entry*) calloc(100, sizeof(struct route_table_entry));
    if (route_table->table == NULL) {
        free(route_table); 
        exit(-1);
    }
    route_table->max_length = 100;
    FILE *input_file = fopen(filename, "r");
    if (input_file == NULL) exit(-1);
    // citire fiecare camp din tabela de rutare
    while (!feof(input_file)) {
        fscanf(input_file, "%s", prefix);
        fscanf(input_file, "%s", next_hop);
        fscanf(input_file, "%s", mask);
        fscanf(input_file, "%d\n", &interface);
        // realocare memorire in cazul in care se umple tabela de rutare alocata (se dubleaza spatiul)
        if (route_table->max_length == route_table->current_length) {
            route_table->table = (struct route_table_entry *)realloc(route_table->table, sizeof(struct route_table_entry) * 2 * route_table->max_length);
            if (route_table->table == NULL) {
                free(route_table);
                exit(-1);
            }
            route_table->max_length *= 2;
        }
        // populare campuri din tabela de rutare
        route_table->table[route_table->current_length].prefix = inet_addr(prefix);
        route_table->table[route_table->current_length].next_hop = inet_addr(next_hop);
        route_table->table[route_table->current_length].mask = inet_addr(mask);
        route_table->table[route_table->current_length].interface = interface;
        route_table->current_length++;
    }
    fclose(input_file);
    return route_table;
}

int cmpfunction(const void *a, const void *b) {
    struct route_table_entry* first = (struct route_table_entry *) a;
    struct route_table_entry* second = (struct route_table_entry *) b;
    // 255.255.255.255 should be the first in the route table
    if (first->mask == -1) return -1;
    if (second->mask == -1) return 1;
    // descending sort on the mask
    if (second->mask != first->mask) return second->mask - first->mask;
    // ascending sort on the prefix
    else return first->prefix - second->prefix;
}

struct route_table_entry *find_best_route(struct route_table *rtable, uint32_t destination) {
    for (int i = 0; i < rtable->current_length; i++) {
        // return the first match because the route table is sorted
        if ((destination & rtable->table[i].mask) == rtable->table[i].prefix) {
            return &rtable->table[i];
        }
    }
    return NULL;
}

struct arp_table* init_arp_table() {
    struct arp_table* arp_table = (struct arp_table *)calloc(1, sizeof(struct arp_table));
    if (arp_table == NULL) exit(-1);
    arp_table->table = (struct arp_table_entry *)calloc(100, sizeof(struct arp_table_entry));
    if (arp_table->table == NULL) {
        free(arp_table);
        exit(-1);
    }
    arp_table->max_length = 100;
    return arp_table;
}

struct arp_table_entry* search_arp_table(struct arp_table* arp_table, uint32_t ip_addr) {
    for (int i = 0; i < arp_table->current_length; i++) {
        if (arp_table->table[i].ip == ip_addr) {
            return &arp_table->table[i];
        }
    }
    return NULL;
}

void add_entry_arp_table(struct arp_table* arp_table, uint32_t ip_addr, uint8_t mac[6]) {
    if (arp_table->current_length == arp_table->max_length) {
        arp_table->table = (struct arp_table_entry *)realloc(arp_table->table, arp_table->max_length * 2);
        if (arp_table == NULL) exit(-1);
        arp_table->max_length *= 2;
    }
    arp_table->table[arp_table->current_length].ip = ip_addr;
    memcpy(arp_table->table[arp_table->current_length].mac, mac, 6);
    arp_table->current_length++;
}
