//
// Created by yruns on 2025/3/12.
//

#ifndef LIST_H
#define LIST_H

#include <cstddef>

/**
 * 双向链表节点结构体
 */
struct BiListNode
{
    BiListNode* prev;  // 指向前一个节点
    BiListNode* next;  // 指向下一个节点
};

/**
 * 双向循环链表类
 */
class BiList
{
public:
    /**
     * @brief 构造函数，初始化头节点
     */
    BiList() : head(new BiListNode), size_(0)
    {
        head->next = head;
        head->prev = head;
    }

    /**
     * @brief 析构函数，释放所有节点
     */
    ~BiList()
    {
        clear();
        delete head;
    }

    /**
     * @brief 在指定位置插入节点
     * @param entry 需要插入的节点
     * @param prev 该节点的前驱节点
     */
    void __add(BiListNode *entry, BiListNode *prev)
    {
        BiListNode *next = prev->next;
        prev->next = entry;
        entry->prev = prev;
        entry->next = next;
        next->prev = entry;
        size_ += 1;
    }

    /**
     * @brief 在头部插入节点
     * @param entry 需要插入的节点
     */
    void add(BiListNode *entry)
    {
        __add(entry, head);
    }

    /**
     * @brief 在尾部插入节点
     * @param entry 需要插入的节点
     */
    void addToTail(BiListNode *entry)
    {
        __add(entry, head->prev);
    }

    /**
     * @brief 删除指定节点
     * @param prev 该节点的前驱节点
     * @param next 该节点的后继节点
     */
    void __del(BiListNode *prev, BiListNode* next)
    {
        prev->next = next;
        next->prev = prev;
        size_ -= 1;
    }

    /**
     * @brief 删除链表中的某个节点
     * @param entry 需要删除的节点
     */
    void del(const BiListNode *entry)
    {
        __del(entry->prev, entry->next);
        delete entry;
    }

    /**
     * @brief 移动节点到另一个链表
     * @param entry 需要移动的节点
     * @param biList 目标链表
     */
    void move(BiListNode* entry, BiList& biList)
    {
        __del(entry->prev, entry->next);
        biList.add(entry);
    }

    /**
     * @brief 移动节点到另一个链表的尾部
     * @param entry 需要移动的节点
     * @param biList 目标链表
     */
    void moveToTail(BiListNode* entry, BiList& biList)
    {
        __del(entry->prev, entry->next);
        biList.addToTail(entry);
    }

    /**
     * @brief 判断链表是否为空
     * @return 若为空返回 true，否则返回 false
     */
    bool empty() const {
        return size_ == 0;
    }

    /**
     * @brief 清空链表，释放所有节点
     */
    void clear()
    {
        if (size_ > 0)
        {
            BiListNode *cur = head->next;
            while (cur != head)
            {
                BiListNode *next = cur->next;
                delete cur;
                cur = next;
            }
            head->next = head;
            head->prev = head;
        }
        size_ = 0;
    }

    /**
     * @brief 合并另一个链表到当前链表
     * @param biList 需要合并的链表
     * @param clear 是否清空被合并的链表
     */
    void merge(BiList& biList, const bool clear = false)
    {
        if (!biList.empty())
        {
            BiListNode* first = biList.head;
            BiListNode* last = biList.head->prev;

            BiListNode* next = head->next;
            head->next = first;
            first->prev = head;
            last->next = next;
            next->prev = last;

            size_ += biList.size_;
            if (clear)
            {
                biList.head->next = biList.head;
                biList.head->prev = biList.head;
                biList.size_ = 0;
            }
        }
    }

    /**
     * @brief 获取链表的大小
     * @return 链表中的节点数量
     */
    size_t size() const
    {
        return size_;
    }

private:
    BiListNode* head;  // 头节点
    size_t size_;      // 链表节点数
};

#endif // LIST_H
