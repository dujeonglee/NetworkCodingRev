#ifndef _AVLTREE_H_
#define _AVLTREE_H_

/*
 * BALANCED_DELETION (Default = Enabled): Delete a node in consideration of balance factor to minimize number of rotations.
 * NONPRIMITIVE_KEY (Default = Disabled): Support a non-primitive type for key value.
 * DEBUG_FUNCTIONS (Default = Disabled): Support debugging functions
 */

#define BALANCED_DELETION
#define NONPRIMITIVE_KEY
//#define DEBUG_FUNCTIONS

#include <exception>
#include <functional>
#ifdef NONPRIMITIVE_KEY
#include <string.h>
#endif


template <class KEY, class DATA> class avltreeelement;
template <class KEY, class DATA> class avltree;

#ifdef NONPRIMITIVE_KEY
template<class KEY>
bool less(const KEY& key1, const KEY& key2){
    return memcmp(&key1, &key2, sizeof(KEY))<0;
}

template<class KEY>
bool greater(const KEY& key1, const KEY& key2){
    return memcmp(&key1, &key2, sizeof(KEY))>0;
}

template<class KEY>
bool equal(const KEY& key1, const KEY& key2){
    return memcmp(&key1, &key2, sizeof(KEY))==0;
}

template<class KEY>
void copy(KEY* const key1, const KEY* const key2){
    memcpy(key1, key2, sizeof(KEY));
}
#endif

template <class KEY, class DATA> class avltreeelement{
private:
    DATA _data;
    KEY _key;
    avltreeelement<KEY, DATA>* _parent;
    avltreeelement<KEY, DATA>* _left;
    avltreeelement<KEY, DATA>* _right;
    char _balance_factor;
    avltree<KEY, DATA> _tree;

    avltreeelement<KEY, DATA>(avltree<KEY, DATA>& t){
        _parent = nullptr;
        _left = nullptr;
        _right = nullptr;
        _balance_factor = 0;
        _tree = t;
        _tree._size++;
    }

    ~avltreeelement<KEY, DATA>(){
        avltreeelement<KEY, DATA>* adjustment_child;
        avltreeelement<KEY, DATA>* adjustment_parent;

        if(_left == nullptr && _right == nullptr){
            adjustment_child = this;
            adjustment_parent = _parent;
            if(_parent){
                (_parent->_left == this?_parent->_left:_parent->_right) = nullptr;
            }else{
                _tree._root = nullptr;
            }
        }else{
            avltreeelement<KEY, DATA>* target;
#ifdef BALANCED_DELETION
            if(_balance_factor > 0){
#else
            if(1){
#endif
                target = (_left?_left:_right);
                while((_left?target->_right:target->_left) != nullptr){
                    target = (_left?target->_right:target->_left);
                }
            }else{
                target = (_right?_right:_left);
                while((_right?target->_left:target->_right) != nullptr){
                    target = (_right?target->_left:target->_right);
                }
            }
            target->_balance_factor = _balance_factor;

            if(target->_parent == this){
                adjustment_parent = target;
                if(adjustment_parent->_left || adjustment_parent->_right){
                    adjustment_child = (adjustment_parent->_left?adjustment_parent->_left:adjustment_parent->_right);
                }else{
                    adjustment_child = nullptr;
                    adjustment_parent->_balance_factor = _balance_factor + (adjustment_parent==_left?-1:1);
                }
                if(_parent){
                    (_parent->_left == this?_parent->_left:_parent->_right) = target;
                }
                target->_parent = _parent;

#ifdef BALANCED_DELETION
                if(_balance_factor > 0){
#else
                if(1){
#endif
                    if((_left?_right:_left)){
                        (_left?_right:_left)->_parent = target;
                    }
                    (_left?target->_right:target->_left) = (_left?_right:_left);
                }else{
                    if((_right?_left:_right)){
                        (_right?_left:_right)->_parent = target;
                    }
                    (_right?target->_left:target->_right) = (_right?_left:_right);
                }
            }else{
                adjustment_child = target;
                adjustment_parent = target->_parent;
#ifdef BALANCED_DELETION
                if(_balance_factor > 0){
#else
                if(1){
#endif
                    (_left?target->_parent->_right:target->_parent->_left) = (_left?target->_left:target->_right);
                    if((_left?target->_parent->_right:target->_parent->_left)){
                        (_left?target->_parent->_right:target->_parent->_left)->_parent = target->_parent;
                    }
                }else{
                    (_right?target->_parent->_left:target->_parent->_right) = (_right?target->_right:target->_left);
                    if((_right?target->_parent->_left:target->_parent->_right)){
                        (_right?target->_parent->_left:target->_parent->_right)->_parent = target->_parent;
                    }
                }
                target->_parent = _parent;
                if(target->_parent){
                    (target->_parent->_left == this?target->_parent->_left:target->_parent->_right) = target;
                }
                target->_left = _left;
                if(target->_left){
                    target->_left->_parent = target;
                }
                target->_right = _right;
                if(target->_right){
                    target->_right->_parent = target;
                }
            }
            if(this == _tree._root){
                _tree._root = target;
            }
        }
        while(adjustment_parent != nullptr){
            if(adjustment_child){
#ifdef NONPRIMITIVE_KEY
                adjustment_parent->_balance_factor += (less<KEY>(adjustment_child->_key, adjustment_parent->_key)?-1:1);
#else
                adjustment_parent->_balance_factor += (adjustment_child->_key < adjustment_parent->_key?-1:1);
#endif
            }
            if( adjustment_parent->_balance_factor == 1 || adjustment_parent->_balance_factor == -1 ){
                break;
            }
            if(adjustment_parent->_balance_factor == 2 || adjustment_parent->_balance_factor == -2 ){
                avltreeelement<KEY, DATA>* const grand_parent = adjustment_parent;
                if(adjustment_parent->_balance_factor == 2){
                    avltreeelement<KEY, DATA>* const parent = grand_parent->_left;
                    if(parent->_balance_factor == 1 || parent->_balance_factor == 0){
                        _tree._right_rotation(grand_parent, parent);
                        if(parent->_balance_factor == 1){
                            grand_parent->_balance_factor = 0;
                            parent->_balance_factor = 0;

                            adjustment_parent = parent->_parent;
                            adjustment_child = parent;
                        }else{ // parent->_balance_factor == 0
                            grand_parent->_balance_factor = 1;
                            parent->_balance_factor = -1;
                            break;
                        }
                    }else{ // adjustment_parent->_left && adjustment_parent->_left->_right
                        avltreeelement<KEY, DATA>* const child = parent->_right;

                        _tree._left_rotation(parent, child);
                        _tree._right_rotation(grand_parent, child);
                        if(child->_balance_factor == 0){
                            grand_parent->_balance_factor = 0;
                            parent->_balance_factor = 0;
                        }else{
                            if(child->_balance_factor == 1){
                                grand_parent->_balance_factor = -1;
                                parent->_balance_factor = 0;
                                child->_balance_factor = 0;
                            }else{ // child->_balance_factor == -1
                                grand_parent->_balance_factor = 0;
                                parent->_balance_factor = 1;
                                child->_balance_factor = 0;
                            }

                        }

                        adjustment_parent = child->_parent;
                        adjustment_child = child;
                    }
                }else{ // adjustment_parent->_balance_factor == -2
                    avltreeelement<KEY, DATA>* const parent = grand_parent->_right;
                    if(parent->_balance_factor == -1|| parent->_balance_factor == 0){
                        _tree._left_rotation(grand_parent, parent);
                        if(parent->_balance_factor == -1){
                            grand_parent->_balance_factor = 0;
                            parent->_balance_factor = 0;

                            adjustment_parent = parent->_parent;
                            adjustment_child = parent;
                        }else{ // parent->_balance_factor == 0
                            grand_parent->_balance_factor = -1;
                            parent->_balance_factor = 1;
                            break;
                        }
                    }else{ // adjustment_parent->_right && adjustment_parent->_right->_left
                        avltreeelement<KEY, DATA>* const child = parent->_left;

                        _tree._right_rotation(parent, child);
                        _tree._left_rotation(grand_parent, child);
                        if(child->_balance_factor == 0){
                            grand_parent->_balance_factor = 0;
                            parent->_balance_factor = 0;
                        }else{
                            if(child->_balance_factor == 1){
                                grand_parent->_balance_factor = 0;
                                parent->_balance_factor = -1;
                                child->_balance_factor = 0;
                            }else{ // child->_balance_factor == -1
                                grand_parent->_balance_factor = 1;
                                parent->_balance_factor = 0;
                                child->_balance_factor = 0;
                            }
                        }
                        adjustment_parent = child->_parent;
                        adjustment_child = child;
                    }
                }
            }else{
                adjustment_child = adjustment_parent;
                adjustment_parent = adjustment_parent->_parent;
            }
        }
        _tree._size--;
    }
public:
    avltreeelement<KEY, DATA> &operator = (const avltreeelement<KEY, DATA> &other) {
        if (this != &other) {
            _data = other._data;
        }
        return *this;
    }

    friend class avltree<KEY, DATA>;
};

template <class KEY, class DATA> class avltree {
private:
    avltreeelement<KEY, DATA>* _root;
    unsigned int _size;

    void _left_rotation(avltreeelement<KEY, DATA>* const parent, avltreeelement<KEY, DATA>* const child){
        if(parent->_right != child){
            return;
        }

        child->_parent = parent->_parent;
        if(child->_parent){
            (child->_parent->_left == parent?child->_parent->_left:child->_parent->_right) = child;
        }else{
            _root = child;
        }

        parent->_right = child->_left;
        if(parent->_right){
            parent->_right->_parent = parent;
        }

        child->_left = parent;
        parent->_parent = child;
    }

    void _right_rotation(avltreeelement<KEY, DATA>* parent, avltreeelement<KEY, DATA>* child){
        if(parent->_left != child){
            return;
        }

        child->_parent = parent->_parent;
        if(child->_parent){
            (child->_parent->_left == parent?child->_parent->_left:child->_parent->_right) = child;
        }else{
            _root = child;
        }

        parent->_left = child->_right;
        if(parent->_left){
            parent->_left->_parent = parent;
        }

        child->_right = parent;
        parent->_parent = child;
    }

public:
    avltree<KEY, DATA>(){
        _root = nullptr;
        _size = 0;
    }
    ~avltree<KEY, DATA>(){
        if(_size > 0){
            clear();
        }
    }

    bool insert(const KEY& key, const DATA& data){
        if(_root == nullptr){
            try{
            _root = new avltreeelement<KEY, DATA>(*this);
            }catch(const std::bad_alloc& ex){
                _root = nullptr;
                return false;
            }

#ifdef NONPRIMITIVE_KEY
            copy<KEY>(&_root->_key, &key);
#else
            _root->_key = key;
#endif
            _root->_data = data;
        }else{
            avltreeelement<KEY, DATA>* grand_parent = nullptr;
            avltreeelement<KEY, DATA>* parent = _root;
            avltreeelement<KEY, DATA>* child = nullptr;
            while(
                  (
          #ifdef NONPRIMITIVE_KEY
                      (less<KEY>(key, parent->_key) && parent->_left == nullptr)
          #else
                      (key < parent->_key && parent->_left == nullptr)
          #endif
                      ||
          #ifdef NONPRIMITIVE_KEY
                      (greater<KEY>(key, parent->_key) && parent->_right == nullptr)
          #else
                      (key > parent->_key && parent->_right == nullptr)
          #endif
                      ||
          #ifdef NONPRIMITIVE_KEY
                      (equal<KEY>(key, parent->_key))
          #else
                      key == parent->_key
          #endif
                      )
                  ==
                  false
                  ){
#ifdef NONPRIMITIVE_KEY
                parent = (less<KEY>(key, parent->_key)?parent->_left:parent->_right);
#else
                parent = (key < parent->_key?parent->_left:parent->_right);
#endif
            }
#ifdef NONPRIMITIVE_KEY
            if(equal<KEY>(key, parent->_key)){
#else
            if(key == parent->_key){
#endif
                return false;
            }else{
                try{
#ifdef NONPRIMITIVE_KEY
                child = (less<KEY>(key, parent->_key)?parent->_left:parent->_right) = new avltreeelement<KEY, DATA>(*this);
#else
                child = (key < parent->_key?parent->_left:parent->_right) = new avltreeelement<KEY, DATA>(*this);
#endif
                }catch(const std::bad_alloc& ex){
#ifdef NONPRIMITIVE_KEY
                      child = (less<KEY>(key, parent->_key)?parent->_left:parent->_right) = nullptr;
#else
                      child = (key < parent->_key?parent->_left:parent->_right) = nullptr;
#endif
                      return false;
                }
#ifdef NONPRIMITIVE_KEY
                copy<KEY>(&child->_key, &key);
#else
                child->_key = key;
#endif
                child->_data = data;
                child->_parent = parent;
                while(parent != nullptr){
                    parent->_balance_factor += (parent->_left == child?1:-1);
                    if(parent->_balance_factor == 2 || parent->_balance_factor == -2 || parent->_balance_factor == 0){
                        break;
                    }
                    child = parent;
                    parent = parent->_parent;
                }
                if(parent && parent->_balance_factor != 0){
                    grand_parent = parent;
                    parent = (grand_parent->_balance_factor == 2?grand_parent->_left:grand_parent->_right);
                    child = (parent->_balance_factor == 1?parent->_left:parent->_right);
                    if(grand_parent->_left == parent && parent->_right == child){
                        _left_rotation(parent, child);
                        _right_rotation(grand_parent, child);
                        if(child->_balance_factor == 0){
                            grand_parent->_balance_factor = 0;
                            parent->_balance_factor = 0;
                        }else{
                            if(child->_balance_factor == 1){
                                grand_parent->_balance_factor = -1;
                                parent->_balance_factor = 0;
                                child->_balance_factor = 0;
                            }else{ // child->_balance_factor == -1
                                grand_parent->_balance_factor = 0;
                                parent->_balance_factor = 1;
                                child->_balance_factor = 0;
                            }
                        }
                    }
                    if(grand_parent->_left == parent && parent->_left == child){
                        _right_rotation(grand_parent, parent);
                        grand_parent->_balance_factor = 0;
                        parent->_balance_factor = 0;
                    }
                    if(grand_parent->_right == parent && parent->_left == child){
                        _right_rotation(parent, child);
                        _left_rotation(grand_parent, child);
                        if(child->_balance_factor == 0){
                            grand_parent->_balance_factor = 0;
                            parent->_balance_factor = 0;
                        }else{
                            if(child->_balance_factor == 1){
                                grand_parent->_balance_factor = 0;
                                parent->_balance_factor = -1;
                                child->_balance_factor = 0;
                            }else{ // child->_balance_factor == -1
                                grand_parent->_balance_factor = 1;
                                parent->_balance_factor = 0;
                                child->_balance_factor = 0;
                            }
                        }
                    }
                    if(grand_parent->_right == parent && parent->_right == child){
                        _left_rotation(grand_parent, parent);
                        grand_parent->_balance_factor = 0;
                        parent->_balance_factor = 0;
                    }
                }
            }
        }
        return true;
    }

    bool remove(const KEY& key){
        if(_root == nullptr){
            return false;
        }
        avltreeelement<KEY, DATA>* target;
        target = _root;
#ifdef NONPRIMITIVE_KEY
        while(!equal<KEY>(key, target->_key)){
#else
        while(key != target->_key){
#endif
#ifdef NONPRIMITIVE_KEY
            target = (less<KEY>(key, target->_key)?target->_left:target->_right);
#else
            target = (key < target->_key?target->_left:target->_right);
#endif
            if(target == nullptr){
                return false;
            }
        }
        delete target;
        return true;
    }

    DATA* find(const KEY& key){
        if(_root == nullptr){
            return nullptr;
        }
        avltreeelement<KEY, DATA>* target;
        target = _root;
#ifdef NONPRIMITIVE_KEY
        while(!equal<KEY>(key, target->_key)){
#else
        while(key != target->_key){
#endif
#ifdef NONPRIMITIVE_KEY
            target = (less<KEY>(key, target->_key)?target->_left:target->_right);
#else
            target = (key < target->_key?target->_left:target->_right);
#endif
            if(target == nullptr){
                return nullptr;
            }
        }
        return (&target->_data);
    }

    unsigned int size(){
        return _size;
    }

    void clear(){
        while(_size){
            delete _root;
        }
    }

#ifdef DEBUG_FUNCTIONS
public:
    void check_max_depth_and_validity(unsigned int* depth, bool* valid){
        (*depth) = 0;
        (*valid) = true;
        if(_root){
            (*depth) = _calculate_max_depth_and_balance_factor(_root, valid);
        }
    }

private:
    unsigned int _calculate_max_depth_and_balance_factor(avltreeelement<KEY, DATA>* node, bool* result){
        unsigned int left = 0;
        unsigned int right = 0;
        if(node->_left){
            left = _calculate_max_depth_and_balance_factor(node->_left, result);
        }
        if(node->_right){
            right = _calculate_max_depth_and_balance_factor(node->_right, result);
        }
        (*result) = (*result) && (left>right?left-right < 2:right-left < 2);
        return 1+(left>right?left:right);
    }
#endif
private:
    void _perform_for_all_data(avltreeelement<KEY, DATA>* node, std::function <void (DATA&)> func){
        func(node->_data);
        if(node->_left){
            _perform_for_all_data(node->_left, func);
        }
        if(node->_right){
            _perform_for_all_data(node->_right, func);
        }
    }

    void _perform_for_all_key(avltreeelement<KEY, DATA>* node, std::function <void (KEY&)> func){
        func(node->_key);
        if(node->_left){
            _perform_for_all_key(node->_left, func);
        }
        if(node->_right){
            _perform_for_all_key(node->_right, func);
        }
    }
public:
    void perform_for_all_data(std::function <void (DATA&)> func){
        if(_root){
            _perform_for_all_data(_root, func);
        }
    }
    void perform_for_all_key(std::function <void (KEY&)> func){
        if(_root){
            _perform_for_all_key(_root, func);
        }
    }
    friend class avltreeelement<KEY, DATA>;
};

#endif
