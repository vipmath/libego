//
// Copyright 2006 and onwards, Lukasz Lew
//

#ifndef NAT_H_
#define NAT_H_

#include "uint.h"

// -----------------------------------------------------------------------------
// For a use case look in player.h

template <class T>
class Nat {
 public:
  // Constructors.

  Nat ();  // TODO replace default constructor with Invalid.
  static T OfRaw (uint raw);
  static T Invalid();

  // Utils.

  bool MoveNext();
  uint GetRaw() const;
  bool IsValid() const;
  bool operator == (const Nat& other) const;
  bool operator != (const Nat& other) const;

 protected:
  explicit Nat (uint raw);

 private:
  uint raw;
  const static bool kDoTests = false;
};

#define ForEachNat(T, var) for (T var = T::Invalid(); var.MoveNext(); )

// -----------------------------------------------------------------------------
// NatMap<Nat, Elt> is an array that can be indexed by Nat

template <typename Nat, typename Elt>
class NatMap {
 public:
  NatMap();
  Elt& operator[] (Nat nat);
  const Elt& operator[] (Nat nat) const;
  void SetAll (const Elt& elt);
  void SetToZero (); // Assumes POD of Elt, uses memcpy.

 private:
  Elt tab [Nat::kBound];
  const static bool kDoTests = false;
};

// -----------------------------------------------------------------------------

#include "nat-inl.h"

#endif // NAT_H_
