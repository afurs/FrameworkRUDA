Framework RUDA - ROOT Utilities for Data Analysis
====================

INSTALLATION
------------

Execute following commands:
```
alienv enter AliPhysics/latest-master-o2
install.sh /path/to/src /path/to/installation /path/to/local/logbook /path/to/grid/ruda /path/to/grid/logbook
```

Example:
```
source install.sh . ~/work/RUDA ~/work/logbook_alice
```

Add following lines into .bashrc
```
source /path/to/installation/bin/env_ruda.sh
```



REQUIREMENTS
------------
ROOT/AliRoot/AliPhysics should be compiled with C++17


