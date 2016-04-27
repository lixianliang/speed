
/*
 * Copyright (C) xianliang.li
 */


/*#include <string.h>
#include <stdlib.h>

#include "lxl_rbtree.h"*/
#include <lxl_config.h>
#include <lxl_core.h>


static inline void lxl_rbtree_left_rotate(lxl_rbtree_node_t **root, lxl_rbtree_node_t *sentinel, lxl_rbtree_node_t *node);
static inline void lxl_rbtree_right_rotate(lxl_rbtree_node_t **root, lxl_rbtree_node_t *sentinel, lxl_rbtree_node_t *node);


void 
lxl_rbtree_insert(lxl_rbtree_t *tree, lxl_rbtree_node_t *node)
{
	lxl_rbtree_node_t **root, *temp, *sentinel;

	root = (lxl_rbtree_node_t **) &tree->root;
	sentinel = tree->sentinel;
	if (*root == sentinel) {
		node->parent = NULL;
		node->left = sentinel;
		node->right = sentinel;
		lxl_rbt_black(node);
		*root = node;

		return;
	}

	tree->insert(*root, node, sentinel);
	
	while (node != *root && lxl_rbt_is_red(node->parent)) {
		if (node->parent == node->parent->parent->left) {
			temp = node->parent->parent->right;
			
			if (lxl_rbt_is_red(temp)) {
				lxl_rbt_black(temp);
				lxl_rbt_black(node->parent);
				lxl_rbt_red(node->parent->parent);
				node = node->parent->parent;
			} else {
				if (node == node->parent->right) {
					node = node->parent;
					lxl_rbtree_left_rotate(root, sentinel, node);	/* node location chanage */
				}

				lxl_rbt_black(node->parent);
				lxl_rbt_red(node->parent->parent);
				lxl_rbtree_right_rotate(root, sentinel, node->parent->parent);
			}
		} else {
			temp = node->parent->parent->left;
			
			if (lxl_rbt_is_red(temp)) {
				lxl_rbt_black(temp);
				lxl_rbt_black(node->parent);
				lxl_rbt_red(node->parent->parent);
				node = node->parent->parent;
			} else {
				if (node == node->parent->left) {
					node = node->parent;
					lxl_rbtree_right_rotate(root, sentinel, node);
				}

				lxl_rbt_black(node->parent);
				lxl_rbt_red(node->parent->parent);
				lxl_rbtree_left_rotate(root, sentinel, node->parent->parent);
			}
		}
	}
	lxl_rbt_black(*root);
}

void
lxl_rbtree_insert_value(lxl_rbtree_node_t *temp, lxl_rbtree_node_t *node, lxl_rbtree_node_t *sentinel)
{
	lxl_rbtree_node_t **p;

	for (; ;) {
		p = (node->key < temp->key) ? &temp->left : &temp->right;
		if (*p == sentinel) {
			break;
		}
		temp = *p;
	}	

	*p = node;
	node->parent = temp;
	node->left = sentinel;
	node->right = sentinel;
	lxl_rbt_red(node);
}

void 
lxl_rbtree_delete(lxl_rbtree_t *tree, lxl_rbtree_node_t *node)
{
	unsigned char red;	
	lxl_rbtree_node_t **root, *sentinel, *subst, *temp, *w;
	
	root = (lxl_rbtree_node_t **) &tree->root;
	sentinel = tree->sentinel;
	if (node->left == sentinel) {
		subst = node;
		temp = node->right;
	} else if (node->right == sentinel) {
		subst = node;
		temp = node->left;
	} else {
		subst = lxl_rbtree_min(node->right, sentinel);
		/*if (subst->left != sentinel) {
			temp = subst->left;
		} else {
			temp = subst->right;
		}*/
		temp = subst->right;
	}

	if (subst == *root) {
		*root = temp;
		(*root)->parent = NULL;	/* lxl add geng jia fuhe */
		lxl_rbt_black(temp);
		
		/*node->parent = NULL;
		node->left = NULL;
		node->right = NULL;
		node->key = 0;*/

		return;
	}

	red = lxl_rbt_is_red(subst);
	if (subst == subst->parent->left) {
		subst->parent->left = temp;
	} else {
		subst->parent->right = temp;
	}
	if (subst == node) {
		temp->parent = subst->parent;
	} else {
		if (subst->parent == node) {
			temp->parent = subst;
		} else {
			temp->parent = subst->parent;
		}
		
		subst->parent = node->parent;
		subst->left = node->left;
		subst->right = node->right;
		lxl_rbt_copy_color(subst, node);
		if (node == *root) {
			*root = subst;
		} else {
			if (node == node->parent->left) {
				node->parent->left = subst;
			} else {
				node->parent->right = subst;
			}
		}
		if (subst->left != sentinel) {
			subst->left->parent = subst;
		}
		if (subst->right != sentinel) {
			subst->right->parent = subst;
		}
	}
	/*
	node->parent = NULL;
	node->left = NULL;
	node->parent = NULL;
	node->key = 0;*/
	if (red) {
		return;
	}

	while (temp != *root && lxl_rbt_is_black(temp)) {
		if (temp == temp->parent->left) {
			w = temp->parent->right;

			if (lxl_rbt_is_red(w)) {
				lxl_rbt_black(w);
				lxl_rbt_red(temp->parent);
				lxl_rbtree_left_rotate(root, sentinel, temp->parent);
				w = temp->parent->right;
			}
				
			if (lxl_rbt_is_black(w->left) && lxl_rbt_is_black(w->right)) {
				lxl_rbt_red(w);
				temp = temp->parent;
			} else {
				if (lxl_rbt_is_black(w->right)) {
					lxl_rbt_black(w->left);
					lxl_rbt_red(w);
					lxl_rbtree_right_rotate(root, sentinel, w);
					w = temp->parent->right;
				}
				
				lxl_rbt_copy_color(w, temp->parent);
				lxl_rbt_black(temp->parent);
				lxl_rbt_black(w->right);
				lxl_rbtree_left_rotate(root, sentinel, temp->parent);
				temp = *root;
			}
		} else {
			w = temp->parent->left;

			if (lxl_rbt_is_red(w)) {
				lxl_rbt_black(w);
				lxl_rbt_red(temp->parent);
				lxl_rbtree_right_rotate(root, sentinel, temp->parent);
				w = temp->parent->left;
			}

			if (lxl_rbt_is_black(w->left) && lxl_rbt_is_black(w->right)) {
				lxl_rbt_red(w);
				temp = temp->parent;
			} else {
				if (lxl_rbt_is_black(w->left)) {
					lxl_rbt_black(w->right);
					lxl_rbt_red(w);
					lxl_rbtree_left_rotate(root, sentinel, w);
					w = temp->parent->left;
				}

				lxl_rbt_copy_color(w, temp->parent);
				lxl_rbt_black(temp->parent);
				lxl_rbt_black(w->left);
				lxl_rbtree_right_rotate(root, sentinel, temp->parent);
				temp = *root;
			}
		}
	}
	lxl_rbt_black(temp);
}

static inline void 
lxl_rbtree_left_rotate(lxl_rbtree_node_t **root, lxl_rbtree_node_t *sentinel, lxl_rbtree_node_t *node)
{
	lxl_rbtree_node_t *temp;

	temp = node->right;
	node->right = temp->left;
	if (temp->left != sentinel) {
		temp->left->parent = node;
	}
	
	temp->parent = node->parent;
	if (node == *root) {
		*root = temp;
	} else if (node == node->parent->left) {
		node->parent->left = temp;
	} else {
		node->parent->right = temp;
	}
	temp->left = node;
	node->parent = temp;
}

static inline void
lxl_rbtree_right_rotate(lxl_rbtree_node_t **root, lxl_rbtree_node_t *sentinel, lxl_rbtree_node_t *node)
{
	lxl_rbtree_node_t *temp;
	
	temp = node->left;
	node->left = temp->right;
	if (temp->right != sentinel) {
		temp->right->parent = node;
	}

	temp->parent = node->parent;
	if (node == *root) {
		*root = temp;
	} else if (node == node->parent->left) {
		node->parent->left = temp;
	} else {
		node->parent->right = temp;
	}
	temp->right = node;
	node->parent = temp;
}
