#include "btree.h"

namespace blink {

BTreeNode::BTreeNode(int degree, bool leaf) : degree_(degree), leaf_(leaf) {}

std::shared_ptr<BTreeNode> BTreeNode::Search(double k)
{
    int i = 0;
    while (i < keys_.size() && k > keys_[i])
        i++;

    if (i < keys_.size() && keys_[i] == k)
        return children_[i];

    if (leaf_)
        return nullptr;

    return children_.at(i)->Search(k);
}

BTree::BTree(int degree) : degree_(degree), root_(new BTreeNode(degree, true)) {}
}
