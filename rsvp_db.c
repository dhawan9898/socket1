#include "rsvp_db.h"
#include "rsvp_msg.h"
#include "timer-event.h"

struct session* sess = NULL;
struct session* head = NULL;
time_t now = 0;

struct session* insert_session(struct session* sess, uint8_t t_id, char sender[], char receiver[]) {
    now = time(NULL);
    printf("insert session\n");
    if(sess == NULL) {
        struct session *temp = (struct session*)malloc(sizeof(struct session));
        if(temp < 0)
            printf("cannot allocate dynamic memory]n");

        temp->last_path_time = now;
        strcpy(temp->sender, sender);
        strcpy(temp->receiver, receiver);
        temp->tunnel_id = t_id;
        temp->next = NULL;
        return temp;
    } else {
        struct session *local = NULL;
        while(sess != NULL) {
            if((strcmp(sess->sender, sender) == 0) &&
                    (strcmp(sess->receiver, receiver) == 0)) {
                sess->last_path_time = now;
                return NULL;
            }
            local = sess;
            sess=sess->next;
        }

        struct session *temp = (struct session*)malloc(sizeof(struct session));
        if(sess < 0)
            printf("cannot allocate dynamic memory\n");

        temp->last_path_time = now;
        strcpy(temp->sender, sender);
        strcpy(temp->receiver, receiver);
        temp->tunnel_id = t_id;
        temp->next = NULL;

        local->next = temp;
    }
}

struct session* delete_session(struct session* sess, char sender[], char receiver[]) {

    struct session *temp = NULL;
    struct session *head = sess;

    printf("delete session\n");
    while(sess != NULL) {
        if((head == sess) &&
                (strcmp(sess->sender, sender) == 0) &&
                (strcmp(sess->receiver, receiver) == 0)) {
            temp = head;
            head = head->next;
            free(temp);
            return head;
        } else {
            if((strcmp(sess->sender, sender) == 0) &&
                    (strcmp(sess->receiver, receiver) == 0)) {
                temp = sess->next;
                *sess = *sess->next;
                free(temp);
            }else{
                sess = sess->next;
            }
        }
    }
}

//AVL for Path adn Resv table
//*****************************************
/* Create a new AVL Node for path_msg */
path_node* create_path_node(path_msg *data) {
    path_node *node = (path_node*)malloc(sizeof(path_node));
    if (!node) {
        printf("Memory allocation failed!\n");
        return NULL;
    }
    node->data = data;
    node->left = node->right = NULL;
    node->height = 1;
    return node;
}

/* Create a new AVL Node for resv_msg */
resv_node* create_resv_node(resv_msg *data) {
    resv_node *node = (resv_node*)malloc(sizeof(resv_node));
    if (!node) {
        printf("Memory allocation failed!\n");
        return NULL;
    }
    node->data = data;
    node->left = node->right = NULL;
    node->height = 1;
    return node;
}

/* Right rotation */
path_node* right_rotate_path(path_node *y) {
    path_node *x = y->left;
    path_node *T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = max(get_path_height(y->left), get_path_height(y->right)) + 1;
    x->height = max(get_path_height(x->left), get_path_height(x->right)) + 1;
    return x;
}

/* Left rotation */
path_node* left_rotate_path(path_node *x) {
    path_node *y = x->right;
    path_node *T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = max(get_path_height(x->left), get_path_height(x->right)) + 1;
    y->height = max(get_path_height(y->left), get_path_height(y->right)) + 1;
    return y;
}

/* Insert a path_msg node */
path_node* insert_path_node(path_node *path_tree, path_msg *data) {
    if (!path_tree) return create_path_node(data);

    if (data->tunnel_id < path_tree->data->tunnel_id)
        path_tree->left = insert_path_node(path_tree->left, data);
    else if (data->tunnel_id > path_tree->data->tunnel_id)
        path_tree->right = insert_path_node(path_tree->right, data);
    else 
        return path_tree; // Duplicate values not allowed

    path_tree->height = 1 + max(get_path_height(path_tree->left), get_path_height(path_tree->right));
    int balance = get_path_balance(path_tree);

    // Perform rotations if unbalanced
    if (balance > 1 && data->tunnel_id < path_tree->left->data->tunnel_id)
        return right_rotate_path(path_tree);
    if (balance < -1 && data->tunnel_id > path_tree->right->data->tunnel_id)
        return left_rotate_path(path_tree);
    if (balance > 1 && data->tunnel_id > path_tree->left->data->tunnel_id) {
        path_tree->left = left_rotate_path(path_tree->left);
        return right_rotate_path(path_tree);
    }
    if (balance < -1 && data->tunnel_id < path_tree->right->data->tunnel_id) {
        path_tree->right = right_rotate_path(path_tree->right);
        return left_rotate_path(path_tree);
    }

    return path_tree;
}

/* Right rotation */
resv_node* right_rotate_resv(resv_node *y) {
    resv_node *x = y->left;
    resv_node *T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = max(get_resv_height(y->left), get_resv_height(y->right)) + 1;
    x->height = max(get_resv_height(x->left), get_resv_height(x->right)) + 1;
    return x;
}

/* Left rotation */
resv_node* left_rotate_resv(resv_node *x) {
    resv_node *y = x->right;
    resv_node *T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = max(get_resv_height(x->left), get_resv_height(x->right)) + 1;
    y->height = max(get_resv_height(y->left), get_resv_height(y->right)) + 1;
    return y;
}

/* Insert a resv_msg node */
resv_node* insert_resv_node(resv_node *resv_tree, resv_msg *data) {
    if (!resv_tree) return create_resv_node(data);

    if (data->tunnel_id < resv_tree->data->tunnel_id)
        resv_tree->left = insert_resv_node(resv_tree->left, data);
    else if (data->tunnel_id > resv_tree->data->tunnel_id)
        resv_tree->right = insert_resv_node(resv_tree->right, data);
    else 
        return resv_tree;  // Duplicate values not allowed

    resv_tree->height = 1 + max(get_resv_height(resv_tree->left), get_resv_height(resv_tree->right));
    int balance = get_resv_balance(resv_tree);

    // Perform rotations if unbalanced
    if (balance > 1 && data->tunnel_id < resv_tree->left->data->tunnel_id)
        return right_rotate_resv(resv_tree);
    if (balance < -1 && data->tunnel_id > resv_tree->right->data->tunnel_id)
        return left_rotate_resv(resv_tree);
    if (balance > 1 && data->tunnel_id > resv_tree->left->data->tunnel_id) {
        resv_tree->left = left_rotate_resv(resv_tree->left);
        return right_rotate_resv(resv_tree);
    }
    if (balance < -1 && data->tunnel_id < resv_tree->right->data->tunnel_id) {
        resv_tree->right = right_rotate_resv(resv_tree->right);
        return left_rotate_resv(resv_tree);
    }

    return resv_tree;
}

/* Utility function to get the minimum value node */
path_node* min_path_node(path_node* node) {
    path_node* current = node;
    while (current->left != NULL)
        current = current->left;
    return current;
}

resv_node* min_resv_node(resv_node* node) {
    resv_node* current = node;
    while (current->left != NULL)
        current = current->left;
    return current;
}

/* Delete a node from path_msg AVL tree */
path_node* delete_path_node(path_node* root, int tunnel_id) {
    if (root == NULL) return root;

    if (tunnel_id < root->data->tunnel_id)
        root->left = delete_path_node(root->left, tunnel_id);
    else if (tunnel_id > root->data->tunnel_id)
        root->right = delete_path_node(root->right, tunnel_id);
    else {
        // Node with only one child or no child
        if ((root->left == NULL) || (root->right == NULL)) {
            path_node* temp = root->left ? root->left : root->right;
            if (temp == NULL) {
                temp = root;
                root = NULL;
            } else
                *root = *temp; // Copy the contents
            free(temp);
        } else {
            path_node* temp = min_path_node(root->right);
            root->data = temp->data;
            root->right = delete_path_node(root->right, temp->data->tunnel_id);
        }
    }

    if (root == NULL) return root;

    root->height = 1 + max(get_path_height(root->left), get_path_height(root->right));
    int balance = get_path_balance(root);

    // Perform rotations if needed
    if (balance > 1 && get_path_balance(root->left) >= 0)
        return right_rotate_path(root);
    if (balance > 1 && get_path_balance(root->left) < 0) {
        root->left = left_rotate_path(root->left);
        return right_rotate_path(root);
    }
    if (balance < -1 && get_path_balance(root->right) <= 0)
        return left_rotate_path(root);
    if (balance < -1 && get_path_balance(root->right) > 0) {
        root->right = right_rotate_path(root->right);
        return left_rotate_path(root);
    }

    return root;
}

/* Delete a node from resv_msg AVL tree */
resv_node* delete_resv_node(resv_node* root, int tunnel_id) {
    if (root == NULL) return root;

    if (tunnel_id < root->data->tunnel_id)
        root->left = delete_resv_node(root->left, tunnel_id);
    else if (tunnel_id > root->data->tunnel_id)
        root->right = delete_resv_node(root->right, tunnel_id);
    else {
        if ((root->left == NULL) || (root->right == NULL)) {
            resv_node* temp = root->left ? root->left : root->right;
            if (temp == NULL) {
                temp = root;
                root = NULL;
            } else
                *root = *temp;
            free(temp);
        } else {
            resv_node* temp = min_resv_node(root->right);
            root->data = temp->data;
            root->right = delete_resv_node(root->right, temp->data->tunnel_id);
        }
    }

    if (root == NULL) return root;

    root->height = 1 + max(get_resv_height(root->left), get_resv_height(root->right));
    int balance = get_resv_balance(root);

    if (balance > 1 && get_resv_balance(root->left) >= 0)
        return right_rotate_resv(root);
    if (balance > 1 && get_resv_balance(root->left) < 0) {
        root->left = left_rotate_resv(root->left);
        return right_rotate_resv(root);
    }
    if (balance < -1 && get_resv_balance(root->right) <= 0)
        return left_rotate_resv(root);
    if (balance < -1 && get_resv_balance(root->right) > 0) {
        root->right = right_rotate_resv(root->right);
        return left_rotate_resv(root);
    }

    return root;
}

/* Search for a path_msg node */
path_node* search_path_node(path_node *path_tree, path_msg *data) {
    if (!path_tree || path_tree->data->tunnel_id == data->tunnel_id)
        return path_tree;

    if (data->tunnel_id < path_tree->data->tunnel_id)
        return search_path_node(path_tree->left, data);
    return search_path_node(path_tree->right, data);
}

/* Search for a resv_msg node */
resv_node* search_resv_node(resv_node *resv_tree, resv_msg *data) {
    if (!resv_tree || resv_tree->data->tunnel_id == data->tunnel_id)
        return resv_tree;

    if (data->tunnel_id < resv_tree->data->tunnel_id)
        return search_resv_node(resv_tree->left, data);
    return search_resv_node(resv_tree->right, data);
}

/* Free a path tree */
void free_path_tree(path_node *path_tree) {
    if (!path_tree) return;
    free_path_tree(path_tree->left);
    free_path_tree(path_tree->right);
    free(path_tree->data);
    free(path_tree);
}

/* Free a resv tree */
void free_resv_tree(resv_node *resv_tree) {
    if (!resv_tree) return;
    free_resv_tree(resv_tree->left);
    free_resv_tree(resv_tree->right);
    free(resv_tree->data);
    free(resv_tree);
}

/* Display path tree (inorder traversal) */
void display_path_tree(path_node *path_tree) {
    if (!path_tree) return;
    display_path_tree(path_tree->left);
    printf("Tunnel ID: %d, Name: %s\n", path_tree->data->tunnel_id, path_tree->data->name);
    display_path_tree(path_tree->right);
}

/* Display resv tree (inorder traversal) */
void display_resv_tree(resv_node *resv_tree) {
    if (!resv_tree) return;
    display_resv_tree(resv_tree->left);
    printf("Tunnel ID: %d\n", resv_tree->data->tunnel_id);
    display_resv_tree(resv_tree->right);
}



//Fetch information from receive buffer
//-------------------------------------

path_node* path_tree_insert(path_node* path_tree, char buffer[]) {
    struct session_object *session_obj = (struct session_object*)(buffer + START_SENT_SESSION_OBJ + 20);
    struct hop_object *hop_obj = (struct hop_object*)(buffer + START_SENT_HOP_OBJ + 20);
    struct time_object *time_obj = (struct time_object*)(buffer + START_SENT_TIME_OBJ + 20);
    struct session_attr_object *session_attr_obj = (struct session_attr_object*)(buffer + START_SENT_SESSION_ATTR_OBJ + 20);
    path_msg *p = malloc(sizeof(path_msg));
    p->tunnel_id = session_obj->tunnel_id;
    p->src_ip = (session_obj->src_ip);
    p->dest_ip = (session_obj->dst_ip);
    p->next_hop_ip = (hop_obj->next_hop);
    p->time_interval = time_obj->interval;
    p->setup_priority = session_attr_obj->setup_prio;
    p->hold_priority = session_attr_obj->hold_prio;
    p->flags = session_attr_obj->flags;
    strncpy(p->name, session_attr_obj->Name, sizeof(session_attr_obj->Name) - 1);
    p->name[sizeof(p->name) - 1] = '\0';
    return insert_path_node(path_tree, p);
}

resv_node* resv_tree_insert(resv_node* resv_tree, char buffer[]) {
    struct session_object *session_obj = (struct session_object*)(buffer + START_SENT_SESSION_OBJ + 20);
    struct hop_object *hop_obj = (struct hop_object*)(buffer + START_SENT_HOP_OBJ + 20);
    struct time_object *time_obj = (struct time_object*)(buffer + START_SENT_TIME_OBJ + 20);
    resv_msg *p = malloc(sizeof(path_msg));
    p->tunnel_id = session_obj->tunnel_id;
    p->src_ip = (session_obj->src_ip);
    p->dest_ip = (session_obj->dst_ip);
    p->next_hop_ip = (hop_obj->next_hop);
    p->time_interval = time_obj->interval;
    return insert_resv_node(resv_tree, p);
}

