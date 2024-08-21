#include "rbtree.h"

#define LEFT 0
#define RIGHT 1

inline static int is_red(rb_node *node) {
    return node && (node->meta & 1UL);
}

inline static void set_red(rb_node *node) {
    node->meta |= 1UL;
}

inline static void set_black(rb_node *node) {
    node->meta &= ~1UL;
}

inline static void set_parent(rb_node *node, rb_node *parent) {
    node->meta &= 1UL;
    node->meta |= (uintptr_t)parent;
}

inline static void swap_colors(rb_node *a, rb_node *b) {
    uintptr_t diff_colors = (a->meta ^ b->meta) & 1UL;
    a->meta ^= diff_colors;
    b->meta ^= diff_colors;
}

inline static rb_node *get_parent(rb_node *node) {
    return (rb_node *)(node->meta & ~1UL);
}

inline static rb_place predecessor(rb_node *node)
{
    rb_place pred = &node->left;

    while ((*pred)->right)
        pred = &(*pred)->right;

    return pred;
}

inline static void rb_rotate(rb_node *node, int dir, rb_node **root_ref) {
    rb_node *parent = get_parent(node);
    rb_node **p_link = parent ?
        &parent->child[parent->child[RIGHT] == node] :
        root_ref;
    rb_node *child = node->child[1 - dir];
    rb_node *grandchild = child->child[dir];

    set_parent(child, parent);
    *p_link = child;

    node->child[1 - dir] = grandchild;
    if (grandchild)
        set_parent(grandchild, node);

    child->child[dir] = node;
    set_parent(node, child);
}

rb_place rb_ref_to_this(rb_node *node, rb_node **root_ref,
                        rb_node **parent_out) {
    rb_node *parent = get_parent(node);

    if (parent_out)
        *parent_out = parent;

    if (!parent)
        return root_ref;

    int node_dir = (int)(node == parent->right);
    return &parent->child[node_dir];
}

rb_place rb_probe(rb_node **root_ref, rb_node **out_parent, int fd) {
    *out_parent = NULL;

    while (*root_ref) {
        if ((*root_ref)->fd == fd)
            break;

        *out_parent = *root_ref;
        root_ref = &(*root_ref)->child[fd > (*root_ref)->fd];
    }

    return root_ref;
}

inline static void fix_insert(rb_node *node, rb_node *parent,
                              rb_node **root_ref) {
    while (is_red(parent)) {
        rb_node *grandparent = get_parent(parent);

        if (!grandparent)
            return; // Reached root. Black height increases by 1

        int parent_dir = grandparent->child[RIGHT] == parent;
        rb_node *uncle = grandparent->child[1 - parent_dir];

        if (is_red(uncle)) {
            set_black(uncle);
            set_black(parent);
            set_red(grandparent);
            node = grandparent;
            parent = get_parent(node);
        } else {
            int node_dir = parent->child[RIGHT] == node;
            if (parent_dir != node_dir) {
                // Mixed rotation
                rb_rotate(parent, parent_dir, root_ref);

                // Update ptrs for next rotation
                rb_node *temp = parent;
                parent = node;
                node = temp;
            }

            // Simple rotation
            rb_rotate(grandparent, 1 - parent_dir, root_ref);
            set_red(grandparent);
            set_black(parent);
            return;
        }
    }
}

void rb_insert(rb_node *node, rb_place place, rb_node *parent,
               rb_node **root_ref) {
    *place = node;
    set_parent(node, parent);

    if (is_red(parent))
        fix_insert(node, parent, root_ref);

    set_black(*root_ref);
}

inline static void fix_delete(rb_node *db, rb_node *parent, rb_node **root_ref) {
    while (!is_red(db)) {
        if (!parent)
            break; // Reached root. Black height decreases by 1

        int dir_to_db = parent->child[RIGHT] == db;
        rb_node *sibling = parent->child[1 - dir_to_db];

        if (is_red(sibling)) {
            set_red(parent);
            set_black(sibling);
            rb_rotate(parent, dir_to_db, root_ref);

            continue;
        }

        enum {
            NONE, NEAR, FAR
        } sibling_red_child = is_red(sibling->child[1 - dir_to_db]) ? FAR :
                              is_red(sibling->child[dir_to_db]) ? NEAR : NONE;

        if (sibling_red_child == NONE) {
            // Sibling is black with all children black
            set_red(sibling);
            db = parent;
            parent = get_parent(db);
            continue;
        }

        if (sibling_red_child == NEAR) {
            // Move near red to far
            set_black(sibling->child[dir_to_db]);
            set_red(sibling);
            rb_rotate(sibling, 1 - dir_to_db, root_ref);
            // Update sibling pointer post rotation
            sibling = parent->child[1 - dir_to_db];
        }

        set_black(sibling->child[1 - dir_to_db]);
        swap_colors(parent, sibling);

        rb_rotate(parent, dir_to_db, root_ref);
        return;
    }

    if (db)
        set_black(db);
}

rb_node *rb_extract(rb_place place, rb_node *parent, rb_node **root_ref) {
    rb_node *curr_node = *place;
    int fd = curr_node->fd;

    if (curr_node->left && curr_node->right) {
        rb_place pred_loc = predecessor(curr_node);
        rb_node *pred = *pred_loc;

        swap_colors(pred, curr_node);

        if (curr_node->left == pred) {
            *place = pred;
            set_parent(pred, parent);
            set_parent(curr_node, pred);

            rb_node *tmp = pred->left;
            pred->left = curr_node;
            curr_node->left = tmp;

            if (tmp)
                set_parent(tmp, curr_node);
        } else {
            // To parent
            set_parent(curr_node, get_parent(pred));
            set_parent(pred, parent);

            // From parent
            *place = pred;
            *pred_loc = curr_node;

            // To left
            rb_node *tmp = curr_node->left;
            curr_node->left = pred->left;
            pred->left = tmp;

            // From left
            if (curr_node->left)
                set_parent(curr_node->left, curr_node);

            set_parent(tmp, pred);
        }

        // Swap right children
        rb_node *tmp = curr_node->right;
        curr_node->right = NULL; // pred->right is always NULL
        pred->right = tmp;

        if (tmp)
            set_parent(tmp, pred);

        parent = pred;
        place = rb_probe(&pred->left, &parent, fd);
    }

    rb_node *son = curr_node->child[LEFT] ?
        curr_node->child[LEFT] : curr_node->child[RIGHT];

    *place = son;
    if (son)
        set_parent(son, parent);
    if (!is_red(curr_node))
        fix_delete(son, parent, root_ref);

    return curr_node;
}
