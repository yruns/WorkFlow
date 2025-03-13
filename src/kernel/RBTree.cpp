#include "RBTree.h"
#include <iostream>

void RBTree::rotateLeft(RBNode* x) {
    if (!x || !x->right) return;
    
    RBNode* y = x->right;
    x->right = y->left;
    
    if (y->left)
        y->left->parent = x;
    
    y->parent = x->parent;
    
    if (!x->parent)
        root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    
    y->left = x;
    x->parent = y;
}

void RBTree::rotateRight(RBNode* x) {
    if (!x || !x->left) return;
    
    RBNode* y = x->left;
    x->left = y->right;
    
    if (y->right)
        y->right->parent = x;
    
    y->parent = x->parent;
    
    if (!x->parent)
        root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    
    y->right = x;
    x->parent = y;
}

void RBTree::insertFixUp(RBNode* z) {
    while (z != root && z->parent && z->parent->color == Color::RED) {
        if (z->parent == z->parent->parent->left) {
            RBNode* y = z->parent->parent->right;
            
            if (y && y->color == Color::RED) {
                // Case 1: Uncle is red
                z->parent->color = Color::BLACK;
                y->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    // Case 2: Uncle is black and z is a right child
                    z = z->parent;
                    rotateLeft(z);
                }
                // Case 3: Uncle is black and z is a left child
                z->parent->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                rotateRight(z->parent->parent);
            }
        } else {
            RBNode* y = z->parent->parent->left;
            
            if (y && y->color == Color::RED) {
                // Case 1: Uncle is red
                z->parent->color = Color::BLACK;
                y->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    // Case 2: Uncle is black and z is a left child
                    z = z->parent;
                    rotateRight(z);
                }
                // Case 3: Uncle is black and z is a right child
                z->parent->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                rotateLeft(z->parent->parent);
            }
        }
    }
    
    root->color = Color::BLACK;
}

void RBTree::insert(int key) {
    RBNode* z = new RBNode(key);
    RBNode* y = nullptr;
    RBNode* x = root;
    
    // Find position to insert new node
    while (x) {
        y = x;
        if (z->key < x->key)
            x = x->left;
        else
            x = x->right;
    }
    
    z->parent = y;
    
    if (!y)
        root = z;
    else if (z->key < y->key)
        y->left = z;
    else
        y->right = z;
    
    // Fix Red-Black tree properties
    insertFixUp(z);
}

void RBTree::transplant(RBNode* u, RBNode* v) {
    if (!u->parent)
        root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;
    
    if (v)
        v->parent = u->parent;
}

void RBTree::eraseFixUp(RBNode* x, RBNode* parent) {
    while (x != root && (!x || x->color == Color::BLACK)) {
        if (!x || (!parent && !x->parent)) break;
        
        RBNode* p = x ? x->parent : parent;

        if (x == p->left) {
            RBNode* w = p->right;
            
            if (w && w->color == Color::RED) {
                // Case 1: sibling is red
                w->color = Color::BLACK;
                p->color = Color::RED;
                rotateLeft(p);
                w = p->right;
            }
            
            if (w && (!w->left || w->left->color == Color::BLACK) && 
                (!w->right || w->right->color == Color::BLACK)) {
                // Case 2: sibling is black and both its children are black
                w->color = Color::RED;
                x = p;
            } else {
                if (w && (!w->right || w->right->color == Color::BLACK)) {
                    // Case 3: sibling is black, left child is red, right child is black
                    if (w->left) w->left->color = Color::BLACK;
                    w->color = Color::RED;
                    rotateRight(w);
                    w = p->right;
                }
                
                // Case 4: sibling is black, right child is red
                if (w) {
                    w->color = p->color;
                    if (w->right) w->right->color = Color::BLACK;
                }
                p->color = Color::BLACK;
                rotateLeft(p);
                x = root;
            }
        } else {
            RBNode* w = p->left;
            
            if (w && w->color == Color::RED) {
                // Case 1: sibling is red
                w->color = Color::BLACK;
                p->color = Color::RED;
                rotateRight(p);
                w = p->left;
            }
            
            if (w && (!w->right || w->right->color == Color::BLACK) && 
                (!w->left || w->left->color == Color::BLACK)) {
                // Case 2: sibling is black and both its children are black
                w->color = Color::RED;
                x = p;
            } else {
                if (w && (!w->left || w->left->color == Color::BLACK)) {
                    // Case 3: sibling is black, right child is red, left child is black
                    if (w->right) w->right->color = Color::BLACK;
                    w->color = Color::RED;
                    rotateLeft(w);
                    w = p->left;
                }
                
                // Case 4: sibling is black, left child is red
                if (w) {
                    w->color = p->color;
                    if (w->left) w->left->color = Color::BLACK;
                }
                p->color = Color::BLACK;
                rotateRight(p);
                x = root;
            }
        }
        
        if (x == p) parent = p;
    }
    
    if (x) x->color = Color::BLACK;
}

void RBTree::erase(int key) {
    RBNode* z = search(key);
    if (!z) return;
    
    RBNode* y = z;
    RBNode* x;
    RBNode* x_parent = nullptr;
    Color original_color = y->color;
    
    if (!z->left) {
        x = z->right;
        transplant(z, z->right);
        if (x) x_parent = x->parent;
        else x_parent = z->parent;
    } else if (!z->right) {
        x = z->left;
        transplant(z, z->left);
        x_parent = x->parent;
    } else {
        y = z->right;
        while (y->left) y = y->left;
        
        original_color = y->color;
        x = y->right;
        
        if (y->parent == z)
            x_parent = y;
        else {
            x_parent = y->parent;
            transplant(y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        
        transplant(z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }
    
    if (original_color == Color::BLACK)
        eraseFixUp(x, x_parent);
    
    delete z;
}

RBNode* RBTree::search(int key) const
{
    RBNode* current = root;
    
    while (current) {
        if (key == current->key)
            return current;
        else if (key < current->key)
            current = current->left;
        else
            current = current->right;
    }
    
    return nullptr;
}

RBNode* RBTree::getMin() const
{
    if (!root) return nullptr;
    
    RBNode* current = root;
    while (current->left)
        current = current->left;
    
    return current;
}

RBNode* RBTree::getMax() const
{
    if (!root) return nullptr;
    
    RBNode* current = root;
    while (current->right)
        current = current->right;
    
    return current;
}

RBNode* RBTree::successor(RBNode* node) {
    if (!node) return nullptr;
    
    if (node->right) {
        RBNode* current = node->right;
        while (current->left)
            current = current->left;
        return current;
    }
    
    RBNode* parent = node->parent;
    while (parent && node == parent->right) {
        node = parent;
        parent = parent->parent;
    }
    
    return parent;
}

RBNode* RBTree::predecessor(RBNode* node) {
    if (!node) return nullptr;
    
    if (node->left) {
        RBNode* current = node->left;
        while (current->right)
            current = current->right;
        return current;
    }
    
    RBNode* parent = node->parent;
    while (parent && node == parent->left) {
        node = parent;
        parent = parent->parent;
    }
    
    return parent;
}

void RBTree::clear(RBNode* node) {
    if (!node) return;
    
    clear(node->left);
    clear(node->right);
    delete node;
}

void RBTree::inOrderPrint(RBNode* node) {
    if (!node) return;
    
    inOrderPrint(node->left);
    std::cout << node->key << "(" << (node->color == Color::RED ? "R" : "B") << ") ";
    inOrderPrint(node->right);
}

void RBTree::print() {
    inOrderPrint(root);
    std::cout << std::endl;
}