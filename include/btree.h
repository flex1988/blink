#ifndef __BTREE_H__
#define __BTREE_H__

#include <memory>
#include <vector>

namespace blink {

class BTreeNode;

class BTreeNode {
  public:
    BTreeNode(int degree, bool leaf);

    void Traverse();
    std::shared_ptr<BTreeNode> Search(double k);

  private:
    std::vector<double> keys_;
    std::vector<std::shared_ptr<BTreeNode>> children_;
    double degree_;
    bool leaf_;
};

class BTree {
  public:
    BTree(int degree);

  private:
    std::shared_ptr<BTreeNode> root_;
    int degree_;
};
}

#endif
