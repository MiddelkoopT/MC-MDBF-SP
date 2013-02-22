#!/usr/bin/python

## Bellman-Ford Multi-Core test platform
## Copyright (c) 2008 Timothy Middelkoop. All rights reserved.
## Licensed under the GPL 2.0 or later.

## Problem generator/import

import sys
import gc
import struct
import string

problem="problem-B.gr"

class Node:
    def __init__(self,_id):
        #self.id=_id
        self.input=[]
        self.output=[]
        self.cost={}
    def __repr__(self):
        return "%d:(>%s <%s)[%s]" % (self.id,self.input,self.output,self.cost)

class Arc:
    def __init__(self,src=None,dest=None,cost=None):
        self.src=src
        self.dest=dest
        self.cost=cost
    def __repr__(self):
        return "(%d->%d):%d" % (self.src,self.dest,self.cost)
    
def read_gr(problem):
    global node

    f=open(problem)

    count=0
    nodes=None
    arcs=None

    print "Disabling GC"
    gc.disable()
    
    ## write out data
    for l in f.xreadlines():
        count+=1
        if count%10000 == 0:
            print "\r%08d" % count,
            sys.stdout.flush()

        if l[0]=='a':
            pass
        elif l[0]=='p':
            sp,nodes,arcs=string.split(l[2:-1],' ')
            nodes=int(nodes)
            arcs=int(arcs)
            if(sp=='spb'):
                arcs*=2;
            node=[None]*(nodes+1)
            continue
        else:
            continue
        
        n1,n2,c=map(lambda x: int(x),string.split(l[2:-1],' '))
        assert n1<=nodes and n2<=nodes
        
        n=node[n1]=node[n1] or Node(n1)
        if n2 not in n.output:
            n.output.append(n2)
            n.cost[n2]=c

        n=node[n2]=node[n2] or Node(n2)
        if n1 not in n.input:
            n.input.append(n1)
            n.cost[n1]=c
            
        ## bi-directional
        if(sp=='spb'):
            (n1,n2)=(n2,n1)
            
            n=node[n1]=node[n1] or Node(n1)
            if n2 not in n.output:
                n.output.append(n2)
                n.cost[n2]=c
    
            n=node[n2]=node[n2] or Node(n2)
            if n1 not in n.input:
                n.input.append(n1)
                n.cost[n1]=c
        
    f.close()
    print "\rdone.       "

    return nodes,arcs,1
    
   
node=None # global due to big copy!
if __name__ == "__main__":
    print "bf-problem.py\n"

    if len(sys.argv) > 1:
        problem=sys.argv[1]

    nodes,arcs,source=read_gr(problem)

    ## write out data
    f=open(problem +".bin",'w')

    ## network
    f.write(struct.pack("iii",nodes,arcs,source))

    for j,n in enumerate(node):

        if j%1 == 0:
            print "\r%08d" % j,
            sys.stdout.flush()

        if not n:
            print "\n",j
            continue

        ## node *N,An,Bn
        f.write(struct.pack("iii",j,len(n.output),len(n.input)))

        ## output arcs (A)
        for k in n.output:
            f.write(struct.pack("ii",k,n.cost[k]))

        ## input arcs (B)
        for i in n.input:
            f.write(struct.pack("ii",i,n.cost[i]))
            
    f.close()
    print "\rdone.       "
    
