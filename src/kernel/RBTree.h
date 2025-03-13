//
// Created by yruns on 2025/3/12.
//

#ifndef RBTREE_H
#define RBTREE_H

enum class Color { RED, BLACK };

struct RBNode {
    int key;
    Color color;
    RBNode *parent, *left, *right;

    RBNode(int k) : key(k), color(Color::RED), parent(nullptr), left(nullptr), right(nullptr) {}
};

class RBTree {
public:
    RBTree() : root(nullptr) {}
    ~RBTree() { clear(root); }

    void insert(int key);
    void erase(int key);
    RBNode* search(int key) const;
    RBNode* getMin() const;
    RBNode *getMax() const;
    static RBNode *successor(RBNode *node);
    static RBNode * predecessor(RBNode* node);
    void print();

private:
    RBNode *root;

    void rotateLeft(RBNode* node);
    void rotateRight(RBNode* node);
    void insertFixUp(RBNode* node);
    void eraseFixUp(RBNode* node, RBNode* parent);
    void transplant(RBNode *u, RBNode *v);
    static void clear(RBNode *node);
    static void inOrderPrint(RBNode* node);
};

#endif // RBTREE_H
