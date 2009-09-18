#ifndef _FAST_TREE_
#define _FAST_TREE_

// -----------------------------------------------------------------------------

// TODO extract iterator with path
// pool podawac jako parametr

template <class Data>
class TreeT {
public:

  class Node;
  class Iterator;

  TreeT (FastPool<Node>& node_pool_) : node_pool(node_pool_) {
  }

  void init () {
    Node* new_node = node_pool.malloc();
    new_node->init (); // TODO move to malloc // TODO use Pool Boost
    it.init(new_node);
  }

  Node* alloc_child (Vertex v) {
    Node* new_node;
    new_node = node_pool.malloc ();
    new_node->init ();
    it.act_node ()->add_child (v, new_node);
    return new_node;
  }

  void delete_act_node (Vertex v) {
    assertc (tree_ac, !it.act_node ()->have_children ());
    it.ascend();
    it.act_node()->remove_child (v);
  }

  // TODO free history (for sync with base board)

  Iterator it;
private:
  FastPool<Node>& node_pool;
};

// -----------------------------------------------------------------------------

template <class Data>
class TreeT<Data> :: Iterator {
public:

  void init (Node* new_node) {
    path.clear();
    path.push_back(new_node);
  }

  void history_reset () {
    path.resize(1);
  }

  Node* act_node () {
    return path.back();
  }

  void descend (Vertex v) {
    path.push_back(path.back()->child(v));
    assertc (tree_ac, act_node () != NULL);
  }
  
  void ascend () {
    assertc (tree_ac, path.size() >= 2);
    path.pop_back ();
  }

  vector<Node*>& history () {
    return path;
  }

private:
  vector<Node*> path;
};

// -----------------------------------------------------------------------------

template <class Data>
class TreeT<Data> :: Node : public Data {
public:

  class Iterator;

  Iterator children() {
    return Iterator(*this);
  }

  void init () {
    children_.memset(NULL);
    have_child = false;
  }

  void add_child (Vertex v, Node* new_child) { // TODO sorting?
    have_child = true;
    // TODO assert
    children_[v] = new_child;
  }

  void remove_child (Vertex v) { // TODO inefficient
    assertc (tree_ac, children_[v] != NULL);
    children_[v] = NULL;
  }

  bool have_children () {
    return have_child;
  }

  Node* child(Vertex v) {
    return children_[v];
  }

private:
  FastMap<Vertex, Node*> children_;
  bool have_child;
};

// -----------------------------------------------------------------------------

template <class Data>
class TreeT<Data> :: Node :: Iterator {
public:

  Iterator(Node& parent) : parent_(parent), act_v_(0) { Sync (); }

  Node& operator* ()  { return *parent_.children_[act_v_]; }
  Node* operator-> () { return parent_.children_[act_v_]; }
  operator Node* ()   { return parent_.children_[act_v_]; }

  void operator++ () { act_v_.next(); Sync (); }
  operator bool () { return act_v_.in_range(); }

private:
  void Sync () {
    while (act_v_.in_range () && parent_.children_[act_v_] == NULL) {
      act_v_.next();
    }
  }
private:
  Node& parent_;
  Vertex act_v_;
};

#endif
