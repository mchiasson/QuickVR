#ifndef HEADSET_H
#define HEADSET_H

#include "Node.h"

class Headset : public Node
{
    Q_OBJECT

public:
    explicit Headset(Node *parent = nullptr);
};


#endif // HEADSET_H
