#!/bin/bash
#
# valvulad        Advanced policy server
#
# chkconfig: 2345 12 88
# valvulad        Advanced policy server
### BEGIN INIT INFO
# Provides: $valvulad
# Required-Start: $local_fs
# Required-Stop: $local_fs
# Default-Start:  2 3 4 5
# Default-Stop: 0 1 6
# Short-Description: start and stop the valvulad server
# Description: valvulad advanced policy server
### END INIT INFO

# Source function library.
. /etc/init.d/functions

DESC="Valvulad server"
NAME=valvulad
DAEMON=/usr/bin/$NAME
DAEMON_ARGS=" --detach "
PIDFILE=/var/run/$NAME.pid
SCRIPTNAME=/etc/init.d/$NAME

RETVAL=0

prog=valvulad
exec=/usr/bin/$NAME
lockfile=/var/lock/subsys/$prog

# Source config
if [ -f /etc/sysconfig/$prog ] ; then
    . /etc/sysconfig/$prog
fi

start() {
	[ -x $exec ] || exit 5

	umask 077

        echo -n $"Starting ${DESC}: "
        daemon --pidfile="$PIDFILE" $exec $DAEMON_ARGS
        RETVAL=$?
        echo
        [ $RETVAL -eq 0 ] && touch $lockfile
        return $RETVAL
}
stop() {
        echo -n $"Stopping ${DESC}: "
        killproc -p "$PIDFILE" $exec
        RETVAL=$?
        echo
        [ $RETVAL -eq 0 ] && rm -f $lockfile
        return $RETVAL
}
rhstatus() {
        status -p "$PIDFILE" -l $prog $exec
}
restart() {
        stop
        start
}

case "$1" in
  start)
        start
        ;;
  stop)
        stop
        ;;
  restart)
        restart
        ;;
  reload)
        exit 3
        ;;
  force-reload)
        restart
        ;;
  status)
        rhstatus
        ;;
  condrestart|try-restart)
        rhstatus >/dev/null 2>&1 || exit 0
        restart
        ;;
  *)
        echo $"Usage: $0 {start|stop|restart|condrestart|try-restart|reload|force-reload|status}"
        exit 3
esac

exit $?
