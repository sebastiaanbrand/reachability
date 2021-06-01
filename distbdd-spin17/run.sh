export GASNET_BACKTRACE=1

upcrun -n $1 -pthreads=$1 -shared-heap 750MB main $2 --tablesize=23 --cachesize=23 --quadratic-probing --chunksize=8