
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    int valid;   
    int R;     
    int M;     
    int frameNumber;
} page_table_entry;

int get_num_frames(int page_size) {
    return 1024 / page_size;
}

int get_offset_bits(int page_size) {
    switch(page_size) {
        case 32:  return 5;
        case 64:  return 6;
        case 128: return 7;
    }
    fprintf(stderr, "Invalid page size!\n");
    exit(EXIT_FAILURE);
}

unsigned int get_vpn(unsigned int vaddr, int offset_bits) {
    return vaddr >> offset_bits; 
}


static page_table_entry *g_page_table = NULL;  
static int *g_frame_to_vpn = NULL;         
static int g_num_frames = 0;               
static int g_page_table_size = 0;       


static unsigned long long g_read_count        = 0;
static unsigned long long g_write_count       = 0;
static unsigned long long g_page_fault_count  = 0;
static unsigned long long g_total_accesses    = 0;


static unsigned long long g_access_counter    = 0;


int find_free_frame(void) {
    for (int frame = 0; frame < g_num_frames; frame++) {
        if (g_frame_to_vpn[frame] == -1) {
            return frame;
        }
    }
    return -1; 
}


int find_victim_frame_NRU(void) {
    int best_class = 4; 
    int best_frame = -1;

    for (int frame = 0; frame < g_num_frames; frame++) {
        int vpn = g_frame_to_vpn[frame];
        if (vpn < 0) {
            continue;
        }
        int R = g_page_table[vpn].R;
        int M = g_page_table[vpn].M;
        int c = R*2 + M;  

        if (c < best_class) {
            best_class = c;
            best_frame = frame;
            if (best_class == 0) break;
        }
    }
    return best_frame;
}


void handle_page_fault(unsigned int vpn, int is_write) {
    int frame = find_free_frame();
    if (frame == -1) {
        frame = find_victim_frame_NRU();
        int old_vpn = g_frame_to_vpn[frame];
        if (old_vpn >= 0) {
            g_page_table[old_vpn].valid = 0;
            g_page_table[old_vpn].R     = 0;
            g_page_table[old_vpn].M     = 0;
            g_page_table[old_vpn].frameNumber = -1;
        }
    }

    g_frame_to_vpn[frame] = vpn;

    g_page_table[vpn].valid       = 1;
    g_page_table[vpn].R           = 1;  
    g_page_table[vpn].M           = (is_write ? 1 : 0);
    g_page_table[vpn].frameNumber = frame;

    g_page_fault_count++;
}

void clear_all_R_bits(void) {
    for (int i = 0; i < g_page_table_size; i++) {
        if (g_page_table[i].valid) {
            g_page_table[i].R = 0;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <input_file> <page_size> <accesses_before_clearing_R>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    int page_size = atoi(argv[2]);
    unsigned long long clear_R_after = strtoull(argv[3], NULL, 10);
 
    g_page_table_size = (1 << 16) / page_size;
    g_page_table = (page_table_entry *)malloc(g_page_table_size * sizeof(page_table_entry));
    if (!g_page_table) {
        fprintf(stderr, "Failed to allocate page table.\n");
        return 1;
    }

    for (int i = 0; i < g_page_table_size; i++) {
        g_page_table[i].valid       = 0;
        g_page_table[i].R           = 0;
        g_page_table[i].M           = 0;
        g_page_table[i].frameNumber = -1;
    }

    g_num_frames = get_num_frames(page_size);
    g_frame_to_vpn = (int *)malloc(g_num_frames * sizeof(int));
    if (!g_frame_to_vpn) {
        fprintf(stderr, "Failed to allocate frame map.\n");
        free(g_page_table);
        return 1;
    }

    for (int f = 0; f < g_num_frames; f++) {
        g_frame_to_vpn[f] = -1; 
    }

    int offset_bits = get_offset_bits(page_size);

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Could not open input file '%s'\n", filename);
        free(g_page_table);
        free(g_frame_to_vpn);
        return 1;
    }

    char address_str[32];
    int op; 

    while (fscanf(fp, "%s %d", address_str, &op) == 2) {
        unsigned int virtual_address = 0;
        sscanf(address_str, "%x", &virtual_address);

        unsigned int vpn = get_vpn(virtual_address, offset_bits);

        if (g_page_table[vpn].valid == 1) {
            g_page_table[vpn].R = 1;
            if (op == 1) {
                g_page_table[vpn].M = 1;
            }
        } else {
            handle_page_fault(vpn, op);
        }

        if (op == 0) {
            g_read_count++;
        } else {
            g_write_count++;
        }

        g_total_accesses++;
        g_access_counter++;

        if (g_access_counter == clear_R_after) {
            clear_all_R_bits();
            g_access_counter = 0;
        }
    }

    fclose(fp);

    printf("num reads = %llu\n",   g_read_count);
    printf("num writes = %llu\n",  g_write_count);

    double pf_rate = 0.0;
    if (g_total_accesses > 0) {
        pf_rate = (double)g_page_fault_count / (double)g_total_accesses;
    }
    printf("percentage of page faults %.2f\n", pf_rate);
    for (int frame = 0; frame < g_num_frames; frame++) {
        printf("mem[%d]: ", frame);
        if (g_frame_to_vpn[frame] == -1) {
            printf("ffffffff\n");
        } else {
            printf("%x\n", g_frame_to_vpn[frame]);
        }
    }

    free(g_frame_to_vpn);
    free(g_page_table);

    return 0;
}
