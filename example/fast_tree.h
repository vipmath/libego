#ifndef _FAST_TREE_
#define _FAST_TREE_

// -----------------------------------------------------------------------------

// TODO replace default std::allocator with FastPool
template <class Data, template <class N> class Allocator = std::allocator>
class Node : public Data, private Allocator<Node<Data, Allocator> > {
public:

  // TODO replace this by placement new in pool or Boost::pool
  Node () {
    children.memset(NULL);
    child_count = 0;
  }

  ~Node () {
    for (ChildrenIterator child(*this); child; ++child) {
      if ((Node*)child != NULL) {
        this->destroy    (child);
        this->deallocate (child, 1);
        child_count -= 1;
      }
    }
    assertc (tree_ac, child_count == 0);
  }

  Node* AddChild (Vertex v) {
    assertc (tree_ac, children[v] == NULL);
    Node* child = this->allocate (1);
    this->construct (child, Node());
    children[v] = child;
    child_count += 1;
    return child;
  }

  void RemoveChild (Vertex v) {
    assertc (tree_ac, children[v] != NULL);
    this->destroy    (children[v]);
    this->deallocate (children[v], 1);
    children[v] = NULL;
    child_count -= 1;
  }

  bool HaveChildren () {
    return child_count > 0;
  }

  // ------------------------------------------------------------------

  class ChildrenIterator {
  public:

  ChildrenIterator(Node& parent) : parent_(parent), act_v_(0) { 
      Sync ();
    }

    Node* operator-> () { return parent_.children[act_v_]; }
    operator Node* ()   { return parent_.children[act_v_]; }

    void operator++ () {
      act_v_.next();
      Sync ();
    }

    operator bool () { return act_v_.in_range(); }

  private:
    void Sync () {
      while (act_v_.in_range () && parent_.children[act_v_] == NULL) {
        act_v_.next();
      }
    }

  private:
    Node& parent_;
    Vertex act_v_;
  };

  // ------------------------------------------------------------------

  class Iterator {
  public:
    void SetToRoot (Node* root) {
      path.clear();
      path.push_back(root);
    }
  
    void ResetToRoot () {
      assertc (tree_ac, path.size() > 0);
      path.resize(1);
    }

    void Descend (Vertex v) {
      assertc (tree_ac, path.size() > 0);
      path.push_back(path.back()->children[v]);
      assertc (tree_ac, ActNode () != NULL);
    }
  
    void Ascend () {
      assertc (tree_ac, path.size() >= 2);
      path.pop_back ();
    }

    vector<Node*>& Path () {
      assertc (tree_ac, path.size() > 0);
      return path;
    }

    Node* operator-> () { return ActNode (); }
    operator Node* ()   { return ActNode (); }

  private:
    Node* ActNode() {
      assertc (tree_ac, path.size() > 0);
      return path.back();
    }

  private:
    vector<Node*> path;
  };

private:
  FastMap<Vertex, Node*> children;
  int child_count;
};

// -----------------------------------------------------------------------------

#endif
