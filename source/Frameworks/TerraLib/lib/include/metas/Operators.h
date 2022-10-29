
#pragma once
#include "NodeMeta.h"

namespace terra
{

struct NodeBiop : public Node
{
  Parameter opA;
  Parameter opB;

  NodeBiop(NodeMeta const& m) : Node(m) {}
};

struct NodeTriop : public Node
{
  Parameter opA;
  Parameter opB;
  Parameter opC;

  NodeTriop(NodeMeta const& m) : Node(m) {}
};

}