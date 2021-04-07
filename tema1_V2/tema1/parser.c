#include "parser.h"

unsigned int convert_mask_to_int(uint32_t mask) {
    unsigned int counter = 0;
    // count how many times bit 1 appears
    for (int i = 0; i < 32; i++) {
        if (((MASK << i) & mask) == (MASK << i)) {
            counter++;
        }
    }
    return counter;
}

struct route_table** parse_route_table(char *filename) {
    char prefix[16];
    char next_hop[16];
    char mask[16];
    int interface;
    // memory for every kind of mask possible (32)
    struct route_table** route_table = (struct route_table**) calloc(33, sizeof(struct route_table *));
    if (route_table == NULL) exit(-1);
    for (int i = 0; i <= 32; i++) {
        route_table[i] = (struct route_table *)calloc(1, sizeof(struct route_table));
        if (route_table[i] == NULL) {
            free(route_table);
            exit(-1);
        }
    }
    FILE *input_file = fopen(filename, "r");
    if (input_file == NULL) exit(-1);
    // read every line from the file representing an entry of the route table
    while (!feof(input_file)) {
        fscanf(input_file, "%s", prefix);
        fscanf(input_file, "%s", next_hop);
        fscanf(input_file, "%s", mask);
        fscanf(input_file, "%d\n", &interface);
        // convert the mask to integer form (ex 255.255.255.0 converted to 24)
        unsigned int int_mask = convert_mask_to_int(inet_addr(mask));
        if (route_table[int_mask]->table == NULL) {
            route_table[int_mask]->table = (struct route_table_entry *)calloc(100, sizeof(struct route_table_entry));
            if (route_table[int_mask]->table == NULL) exit(-1);
            route_table[int_mask]->max_length = 100;
        }
        // realloc the memory if it becomes full for a mask
        if (route_table[int_mask]->current_length == route_table[int_mask]->max_length) {
            route_table[int_mask]->table = (struct route_table_entry *)realloc(route_table[int_mask]->table,
                sizeof(struct route_table_entry) * 2 * route_table[int_mask]->max_length);
            if (route_table[int_mask]->table == NULL) exit(-1);
            route_table[int_mask]->max_length *= 2;
        }
        // wirte in the fields of the structure
        route_table[int_mask]->table[route_table[int_mask]->current_length].mask = inet_addr(mask);
        route_table[int_mask]->table[route_table[int_mask]->current_length].interface = interface;
        route_table[int_mask]->table[route_table[int_mask]->current_length].next_hop = inet_addr(next_hop);
        route_table[int_mask]->table[route_table[int_mask]->current_length].prefix = inet_addr(prefix);
        route_table[int_mask]->current_length++;
    }
    fclose(input_file);
    return route_table;
}

int cmpfunction(const void *a, const void *b) {
    struct route_table_entry* first = (struct route_table_entry *) a;
    struct route_table_entry* second = (struct route_table_entry *) b;
    // ascending sort on the prefix
    return first->prefix - second->prefix;
}

int search(struct route_table* r_table, uint32_t destination, int left, int right) {
    int mid = 0;
    while (left <= right) {
        mid = (left + right) / 2;
        if (r_table->table[mid].prefix == (destination & r_table->table[mid].mask)) return mid;
        if (r_table->table[mid].prefix > (destination & r_table->table[mid].mask)) right = mid - 1;
        if (r_table->table[mid].prefix < (destination & r_table->table[mid].mask)) left = mid + 1;
    }
    return -1;
}

struct route_table_entry *find_best_route(struct route_table **rtable, uint32_t destination) {
    for (int i = 32; i >= 0; i--) {
        if (rtable[i]->current_length != 0 && rtable[i]->table != NULL) {
            int position = search(rtable[i], destination, 0, rtable[i]->current_length - 1);
            if (position != -1) {
                return &rtable[i]->table[position];
            }
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
