#
# Regular cron jobs for the atomicles package
#
0 4	* * *	root	[ -x /usr/bin/atomicles_maintenance ] && /usr/bin/atomicles_maintenance
