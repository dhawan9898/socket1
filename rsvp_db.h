#include<stdio.h>
#include<stdint.h>
#include<string.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>

struct session {
    char sender[16];
    char receiver[16];
    uint8_t tunnel_id;
    time_t last_path_time;
    struct session *next;
};

/* Define path_msg structure */
typedef struct path_msg {
    uint8_t tunnel_id;
    struct in_addr src_ip;
    struct in_addr dest_ip;
    struct in_addr next_hop_ip;
    uint8_t IFH;
    uint8_t time_interval;
    uint8_t setup_priority;
    uint8_t hold_priority;
    uint8_t flags;
    char name[32];
} path_msg;

/* Define resv_msg structure */
typedef struct resv_msg {
    uint8_t tunnel_id;
    struct in_addr src_ip;
    struct in_addr dest_ip;
    struct in_addr next_hop_ip;
    uint8_t IFH;
    uint8_t time_interval;
} resv_msg;

/* PATH Node */
typedef struct path_node {
    path_msg *data;
    struct path_node *left;
    struct path_node *right;
    int height;
} path_node;

/* RESV Node */
typedef struct resv_node {
    resv_msg *data;
    struct resv_node *left;
    struct resv_node *right;
    int height;
} resv_node;

static inline int get_path_height(path_node *node) {
    return node ? node->height : 0;
}

static inline int get_resv_height(resv_node *node) {
    return node ? node->height : 0;
}

static inline int max(int a, int b) {
    return (a > b) ? a : b;
}

static inline int get_path_balance(path_node *node) {
    return node ? get_path_height(node->left) - get_path_height(node->right) : 0;
}

static inline int get_resv_balance(resv_node *node) {
    return node ? get_resv_height(node->left) - get_resv_height(node->right) : 0;
}

path_node* insert_path_node(path_node *path_tree, path_msg *data);
resv_node* insert_resv_node(resv_node *resv_tree, resv_msg *data);
path_node* delete_path_node(path_node *path_tree, int tunnel_id);
resv_node* delete_resv_node(resv_node *resv_tree, int tunnel_id);
path_node* search_path_node(path_node *path_tree, path_msg *data);
resv_node* search_resv_node(resv_node *resv_tree, resv_msg *data);
void free_path_tree(path_node *path_tree);
void free_resv_tree(resv_node *resv_tree);
void display_path_tree(path_node *path_tree);
void display_resv_tree(resv_node *resv_tree);

struct session* insert_session(struct session* , uint8_t, char[], char[]);
struct session* delete_session(struct session* , char[], char[]);
path_node* path_tree_insert(path_node*, char[]);
resv_node* resv_tree_insert(resv_node*, char[]);

