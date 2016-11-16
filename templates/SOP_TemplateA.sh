#!/usr/bin/env bash
g++-4.8 -DUT_DSO_TAGINFO='"3262197cbf075a113186189b4228dcc67219edbdfa068a8c7ed4b6180914bfe618912890d67e2c704309e3b267da425d1fee9b5fb9c2e856530628e266fc9bbf4dbe57b0ee846394a3268d0847ba11fce917cff0cff78a61e1dc"'  -DVERSION=\"15.0.313\" -D_GNU_SOURCE -DLINUX -DAMD64 -m64 -fPIC -DSIZEOF_VOID_P=8 -DFBX_ENABLED=1 -DOPENCL_ENABLED=1 -DOPENVDB_ENABLED=1 -DSESI_LITTLE_ENDIAN -DENABLE_THREADS -DUSE_PTHREADS -D_REENTRANT -D_FILE_OFFSET_BITS=64 -c  -DGCC4 -DGCC3 -Wno-deprecated -std=c++11 -isystem /opt/houdini/toolkit/include -Wall -W -Wno-parentheses -Wno-sign-compare -Wno-reorder -Wno-uninitialized -Wunused -Wno-unused-parameter -Wno-unused-local-typedefs -O2 -fno-strict-aliasing  -DMAKING_DSO -o SOP_TemplateA.o SOP_TemplateA.C


g++-4.8 -DUT_DSO_TAGINFO='"3262197cbf075a113186189b4228dcc67219edbdfa068a8c7ed4b6180914bfe618912890d67e2c704309e3b267da425d1fee9b5fb9c2e856530628e266fc9bbf4dbe57b0ee846394a3268d0847ba11fce917cff0cff78a61e1dc"'  -DVERSION=\"15.0.313\" -D_GNU_SOURCE -DLINUX -DAMD64 -m64 -fPIC -DSIZEOF_VOID_P=8 -DFBX_ENABLED=1 -DOPENCL_ENABLED=1 -DOPENVDB_ENABLED=1 -DSESI_LITTLE_ENDIAN -DENABLE_THREADS -DUSE_PTHREADS -D_REENTRANT -D_FILE_OFFSET_BITS=64 -c  -DGCC4 -DGCC3 -Wno-deprecated -std=c++11 -isystem /opt/houdini/toolkit/include -Wall -W -Wno-parentheses -Wno-sign-compare -Wno-reorder -Wno-uninitialized -Wunused -Wno-unused-parameter -Wno-unused-local-typedefs -O2 -fno-strict-aliasing  -DMAKING_DSO -o GU_PackedSphere.o GU_PackedSphere.C


g++ -shared SOP_TemplateA.o GU_PackedSphere.o -L/usr/X11R6/lib64 -L/usr/X11R6/lib -lGLU -lGL -lX11 -lXext -lXi -ldl -o /home/user/houdini15.0/dso/SOP_TemplateA.so

