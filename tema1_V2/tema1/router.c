#include <queue.h>
#include "parser.h"

int main(int argc, char *argv[]) {
	packet m;
	int rc;
	queue q = queue_create();

	init(argc - 2, argv + 2);

	// initialize the route table and the arp table and sort the route table for every mask
	struct route_table** route_table = parse_route_table(argv[1]);
	for (int i = 0; i < 32; i++) 	
		qsort(route_table[i]->table, route_table[i]->current_length, sizeof(struct route_table_entry), cmpfunction);
	struct arp_table* arp_table = init_arp_table();
	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		/* Students will write code here */
		struct ether_header *eth_hdr = (struct ether_header *)m.payload;
		struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
		struct arp_header *arp_hdr = parse_arp(m.payload);
		struct icmphdr *icmp_hdr = parse_icmp(m.payload);
		if (arp_hdr) {
			if (arp_hdr->op == htons(ARPOP_REQUEST)) {
				if (arp_hdr->tpa == inet_addr(get_interface_ip(m.interface))) {
					// update ethernet header
					memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);
					get_interface_mac(m.interface, eth_hdr->ether_shost);
					// send arp reply with the mac address of the router interface
					send_arp(arp_hdr->spa, arp_hdr->tpa, eth_hdr, m.interface, htons(ARPOP_REPLY));
				}
			}
			else if (arp_hdr->op == htons(ARPOP_REPLY)) {
				// add the entry to the arp table
				add_entry_arp_table(arp_table, arp_hdr->spa, arp_hdr->sha);
				// if the queue contains packages that have to be sent extract them and send them
				if (!queue_empty(q)) {
					uint8_t mac[6];
					memcpy(mac, arp_hdr->sha, 6);
					packet packet_to_send = *(packet *)queue_deq(q);
					struct ether_header *eth_hdr_pck = (struct ether_header *)packet_to_send.payload;
					memcpy(eth_hdr_pck->ether_dhost, mac, 6);
					get_interface_mac(m.interface, eth_hdr_pck->ether_shost);
					send_packet(m.interface, &packet_to_send);
				}
			}
		}
		else {
			if (icmp_hdr) {
				// if it is icmp echo request, the router replies
				uint8_t mac[6];
				get_interface_mac(m.interface, mac);
				if (icmp_hdr->type == ICMP_ECHO && inet_addr(get_interface_ip(m.interface)) == ip_hdr->daddr) {
					send_icmp(ip_hdr->saddr, ip_hdr->daddr, mac, eth_hdr->ether_shost, 
					ICMP_ECHOREPLY, icmp_hdr->code, m.interface, icmp_hdr->un.echo.id, icmp_hdr->un.echo.sequence);
					continue;
				}
			}
			// check the checksum
			uint16_t checksum_aux = ip_hdr->check;
			ip_hdr->check = 0;
			if (ip_checksum(ip_hdr, sizeof(struct iphdr)) != checksum_aux) continue;
			ip_hdr->ttl--;
			if (ip_hdr->ttl < 1) {
				// send time exceeded icmp
				send_icmp_error(ip_hdr->saddr, ip_hdr->daddr, eth_hdr->ether_dhost, eth_hdr->ether_shost, ICMP_TIME_EXCEEDED, 0, m.interface);
				continue;
			}
			// search in the route table
			struct route_table_entry* route_entry = find_best_route(route_table, ip_hdr->daddr);
			if (route_entry == NULL) {
				// destination not found
				send_icmp_error(ip_hdr->saddr, ip_hdr->daddr, eth_hdr->ether_dhost, eth_hdr->ether_shost, ICMP_UNREACH, 0, m.interface);
				continue;
			}
			// recalculate the cheschsum
			ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));
			struct arp_table_entry* arp_entry = search_arp_table(arp_table, ip_hdr->daddr);
			// entry not found in the arp table
			if (arp_entry == NULL) {
				packet *aux = (packet *)calloc(1, sizeof(packet));
				memcpy(aux, &m, sizeof(packet));
				// save the packet in the queur that will be send when the arp reply is received
				queue_enq(q, aux);
				// send arp request as broadcast and update the ethernet header
				for (int i = 0; i < 6; i++) {
					eth_hdr->ether_dhost[i] = 0xff;
				}
				get_interface_mac(route_entry->interface, eth_hdr->ether_shost);
				eth_hdr->ether_type = htons(ETHERTYPE_ARP);
				// send the arp request
				send_arp(route_entry->next_hop, inet_addr(get_interface_ip(route_entry->interface)), eth_hdr, route_entry->interface, htons(ARPOP_REQUEST));
			}
			else {
				// entry found in the arp table and the packet is immediately sent
				memcpy(eth_hdr->ether_dhost, arp_entry->mac, 6);
				get_interface_mac(route_entry->interface, eth_hdr->ether_shost);
				send_packet(route_entry->interface, &m);
			}
		}
	}
}