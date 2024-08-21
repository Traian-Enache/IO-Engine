#ifndef IO_RBTREE_H_
#define IO_RBTREE_H_ 1

#include "rbnode.h"

typedef rb_node **rb_place;

// inline static rb_node *gpr(rb_node *n) {
//     return (rb_node*)(n->meta & ~1UL);
// }

// inline static void rb_debug(rb_node *rb, int depth) {
//     if (!rb) {
//         printf("%*snil\n", 4 * depth, "");
//         return;
//     }
//     printf("%*s%c %d-%d\n", 4 * depth, "", rb->meta & 1 ? 'R' : 'B', rb->fd, gpr(rb) ? gpr(rb)->fd : 0);
//     rb_debug(rb->left, depth + 1);
//     rb_debug(rb->right, depth + 1);
// }

rb_place rb_ref_to_this(rb_node *node, rb_node **root_ref,
                        rb_node **parent_out);

rb_place rb_probe(rb_node **root_ref, rb_node **parent_out, int key);

void rb_insert(rb_node *node, rb_place place, rb_node *parent,
               rb_node **root_ref);

rb_node *rb_extract(rb_place place, rb_node *parent, rb_node **root_ref);

#endif // IO_RBTREE_H_
