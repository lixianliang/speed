# !/bin/sh 

BIN_DIR=bin
CONF_DIR=conf

function usage()
{
	echo "speed.sh dfst|dfss|dfs start|stop|status|restart"
}

if [ $# -ne 2 ]
then 
#	echo "Usage: program args"
	usage
	exit 1
fi

case $1 in
dfst)
	case $2 in
	start)
		${BIN_DIR}/dfst -c ${CONF_DIR}/dfst.conf
		echo "dfst start..."
		ps -ef|grep dfst
		;;
	stop)
		killall dfst
		;;
	restart)
		killall dfst
		${BIN_DIR}/dfst -c ${CONF_DIR}/dfst.conf
		ps -ef|grep dfst
		;;
	status)
		ps -ef|grep dfst
		;;
	*)
		usage
		exit 1
	esac
	;;

dfss)
	case $2 in
	start)
		${BIN_DIR}/dfss -c ${CONF_DIR}/dfss.conf
		echo "dfss start..."
		ps -ef|grep dfss
		;;
	stop)
		killall dfss
		;;
	restart)
		killall dfss
		${BIN_DIR}/dfss -c ${CONF_DIR}/dfss.conf
		ps -ef|grep dfss
		;;
	status)
		ps -ef|grep dfss
		;;
	*)
		usage
		exit 1
	esac
	;;

dfs)
	case $2 in
	start)
		${BIN_DIR}/dfst -c ${CONF_DIR}/dfst.conf
		sleep 1
		${BIN_DIR}/dfss -c ${CONF_DIR}/dfss.conf
		ps -ef|grep dfs
		;;
	stop)
		killall dfst dfss
		;;
	restart)
		killall dfst dfss
		${BIN_DIR}/dfst -c ${CONF_DIR}/dfst.conf
		sleep 1
		${BIN_DIR}/dfss -c ${CONF_DIR}/dfss.conf
		ps -ef|grep dfs
		;;
	status)
		ps -ef|grep dfs
		;;
	*)
		usage
		exit 1
	esac
	;;
	
*)
	usage
	exit 1
esac

