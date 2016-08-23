/*
#
# Copyright (c) 2012 NVIDIA CORPORATION.  All Rights Reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#define LOG_TAG "volt_cap"
#include <errno.h>
#include <stdlib.h>
#include <cutils/log.h>
#include <fcntl.h>
#include <pthread.h>
#include "cpu_volt_cap.h"
#include <sys/socket.h>
#include <linux/netlink.h>


#define VC_DEBUG 1
#if VC_DEBUG
#define VC_TRACE(...) ALOGD("VC: "__VA_ARGS__)
#else
#define VC_TRACE(...) {};
#endif


#define LOG_ALL			-1
#define LOG_TABLE		1
#define LOG_EVENTS		2
#define LOG_FILE "/data/vc_log.txt"
#define DATA_FILE "/data/device_config.txt"
#define SAFE_USER_POINT_LIMIT 100
#define VOLT_FILE "/sys/kernel/tegra_cpu_volt_cap/volt"
#define CAPPING_ENABLE_FILE "/sys/kernel/tegra_cpu_volt_cap/capping_state"
#define VOLT_STAT_POLL_INTERVAL_SECONDS (5*60)
#define STATS_FILE "/sys/power/tegra_rail_stats"
#define PATH    "/sys/class/thermal"
#define FILETYPE "nct_ext"

/* 1 degree is represented a 1000 units, following is 10 degrees celcius step size */
#define HV_THERMAL_STEP	10000
#define THREAD_DATA_INIT(function, data)\
{ \
	.f = function,\
	.d = data, \
}
/* size of the event structure, not counting name */
#define EVENT_SIZE  (sizeof(struct inotify_event))
/* reasonable guess as to size of 10 events */
#define BUF_LEN		(10 * (EVENT_SIZE + 16))

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int debug_params;
static struct status_data stats_data;
static char thermal_filepath[100];

static void log_events(const char *fmt, ...)
{
	FILE *fp = fopen(LOG_FILE, "a");
	va_list ap;
	va_start(ap, fmt);
	if ((debug_params & LOG_EVENTS) && fp) {
		struct tm *local;
		time_t t;
		t = time(NULL);
		local = localtime(&t);
		fprintf(fp, "%s ", asctime(local));
		vfprintf(fp, fmt, ap);
	}
	if (fp)
		fclose(fp);
	va_end(ap);
}

static void log_tables(const char *fmt, ...)
{
	FILE *fp = fopen(LOG_FILE,"a");
	va_list ap;
	va_start(ap, fmt);
	if ((debug_params & LOG_TABLE) && fp)
		vfprintf(fp, fmt, ap);
	if (fp)
		fclose(fp);
	va_end(ap);
}

static void sysfs_write(const char *path, const char *s)
{
	char buf[80];
	int len;
	int fd = open(path, O_RDWR);

	if (fd < 0) {
		strerror_r(errno, buf, sizeof(buf));
		VC_TRACE("Error opening %s: %s\n", path, buf);
		return;
	}

	len = write(fd, s, strlen(s));
	if (len < 0) {
		strerror_r(errno, buf, sizeof(buf));
		VC_TRACE("Error writing to %s: %s\n", path, buf);
	}
	close(fd);
}

static void sysfs_read(const char *path, char *s, int size)
{
	int len;
	int fd = open(path, O_RDONLY);

	if (fd < 0) {
		strerror_r(errno, s, size);
		VC_TRACE("Error opening %s: %s\n", path, s);
		return;
	}

	len = read(fd, s, size);
	close(fd);

	if (len < 0) {
		strerror_r(errno, s, size);
		VC_TRACE("Error reading from %s: %s\n", path, s);
	}
}

static unsigned int get_crcb(char *cp, unsigned int size)
{
	unsigned int crc;
	unsigned int mask;
	int j;

	crc = 0xFFFFFFFF;

	while (size--) {
		crc = crc ^ *cp++;
		for (j = 7; j >= 0; j--) {
			mask = (crc & 1) ? 0xEDB88320 : 0;
			crc = (crc >> 1) ^ mask;
		}
	}
	return ~crc;
}

static int get_state(void)
{
	int ret;
	pthread_mutex_lock(&mutex);
	ret = stats_data.service_state;
	pthread_mutex_unlock(&mutex);
	return ret;
}

static int set_state(int state)
{
	pthread_mutex_lock(&mutex);
	stats_data.service_state = state;
	pthread_mutex_unlock(&mutex);
	return 0;
}

static void lock_and_wait(void)
{
	pthread_mutex_lock(&mutex);
	pthread_cond_wait(&cond, &mutex);
	pthread_mutex_unlock(&mutex);
}

static void lock_and_signal(void)
{
	pthread_mutex_lock(&mutex);
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&mutex);
}

/*
 * save_to_file :
 * Saves the available points of user
 * Creates a crc value for the structure
 * Adds the time of file updation.
 */
static void save_to_file(void)
{
	int len;
	unsigned int crc;
	int fd;
	struct saved_data sd;

	clock_gettime(CLOCK_REALTIME, &sd.saved_time);
	pthread_mutex_lock(&mutex);
	sd.balance_points = stats_data.points_balance - stats_data.points_consumed;
	pthread_mutex_unlock(&mutex);
	crc = get_crcb((void *)&sd, sizeof(sd));
	fd = open(DATA_FILE, O_RDWR);
	len = write(fd, &sd, sizeof(sd));
	len = write(fd, &crc, sizeof(unsigned int));
	close(fd);
}

/* Events supported by Thermal Netlink */
enum events {
	THERMAL_AUX0,
	THERMAL_AUX1,
	THERMAL_CRITICAL,
	THERMAL_DEV_FAULT,
};

struct thermal_genl_event {
	int orig;
	enum events event;
};

/*
 * temp_mon_thread - Thread receives netlink messages for temperature
 * changes.
 *
 * Thread execution - Thread waits for messages on the socket interface.
 * The message provides the temperature origin and event type.
 * The origin would provide the id of the thermal zone device. This is
 * not used currently.
 * The thread signals the stats update thread to recalculate parameters.
 */

static void *temp_mon_thread(void *x)
{
	char buf[100];
	int sock_fd;
	int result;
	struct sockaddr_nl src_addr, dest_addr;
	struct iovec iov;
	struct msghdr msg;

	sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
	if (sock_fd == -1) {
		ALOGE("Socket faied!\n");
		return 0;
	}

	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid();
	src_addr.nl_groups = 2;
	result = bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr));
	if (result) {
		ALOGE("Bind faied! %d.\n", result);
		return 0;
	}

	iov.iov_base = buf;
	iov.iov_len = 100;
	msg.msg_name = (void *)&dest_addr;
	msg.msg_namelen = sizeof(dest_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	while (get_state()) {
		struct thermal_genl_event *pgenl;

		result = recvmsg(sock_fd, &msg, 0);
		if (result == -1) {
			ALOGE("recvmsg failed, error is %d\n", result);
			return 0;
		}
		pgenl = NLMSG_DATA(buf);
		/* skip genl header */
		pgenl += 1;
		ALOGE("Origin=%d, Event=%d\n", pgenl->orig, pgenl->event);
		memset(buf, 0x0, sizeof(buf));
		log_events("Temperature monitor\n");
		lock_and_signal();
	}
	VC_TRACE("Thread %s exit", __func__);
	return 0;
}

/*
 * volt_mon_thread - Thread create event to recalculate burnout.
 * Thread execution - Thread sleeps for voltage polling interval
 * currently 5 minutes. When it is woken up it signals the
 * stats update thread.
 */

static void *volt_mon_thread(void *x)
{
	int status;
	char a[10];

	VC_TRACE("Thread %s init", __func__);
	while (get_state()) {
		sleep(VOLT_STAT_POLL_INTERVAL_SECONDS);
		lock_and_signal();
		log_events("Voltage monitor\n");
	}
	VC_TRACE("Thread %s exit", __func__);
	return 0;
}

/*
 * stats_update_thread - Thread to gather temp and voltage statistics.
 * This thread maintains a temperature vs voltage table. The stats at
 * temperature 0 is the sum of times spend at that voltage over various
 * temperature distribution.
 *
 * Thread execution - Thread waits for following signals.
 *		a.) Temperature change
 *		b.) Voltage capture
 *		c.) Points update.
 *
 * On any of the events thread takes following action.
 * 1.) Reads CPU temperature through sysfs.
 * 2.) Reads the CPU rail voltage and time stats through sysfs.
 * 3.) Subtracts new volt vs time stats with the saved volt vs time
 * stats (Present at temperature = 0).
 * 4.) The value of 3.) is places in temperature basket of previously
 * read temperature.
 * 5.) Old temperature is updated to reflect current temperature.
 * 6.) The complete table of temperature vs volt is multiplied with
 * table provided at
 * https://wiki.nvidia.com/wmpwiki/index.php/Tegra_T35/Power/Hyper-Voltaging
 * The conversion is done for ms to hour burnout.
 * 7.) The resultant is a total_burnout for this session.
 * 8.) If points available is greater than a safe value (100 points)
 * then nothing is done.
 * 9.) If points are less than 100, a capping voltage currently 1000mv
 * is set and voltage capping is enabled.
 * 10.) The table is written to log file on thread trigger.
 *
 */

static void *stats_update_thread(void *data)
{
	char buf[10];
	char temperature_buffer[12];
	char *stats_buffer;
	char *pstats;
	int temperature = 0;
	int stats_buffer_size = 1024;
	int x = 0, prev_x = 0, y;
	int i = 0, j = 0;
	int *temp_vs_volt_secs;
	int temp_vs_volt_secs_size;
	int temp_numbers;
	int volt_numbers;
	struct volt_time vt[50];
	unsigned long temp_val;
	float total_burnout;
	float points_to_burn;


	temp_numbers = sizeof(temp_range)/sizeof(int);
	volt_numbers = sizeof(volt_range)/sizeof(int);
	temp_vs_volt_secs_size = temp_numbers * volt_numbers;
	temp_vs_volt_secs = (int *)malloc(temp_vs_volt_secs_size * sizeof(int));
	if (!temp_vs_volt_secs) {
		VC_TRACE("Thread %s exit due to malloc failure\n", __func__);
		return 0;
	}
	memset(temp_vs_volt_secs, 0, temp_vs_volt_secs_size);
	stats_buffer = (char *)malloc(sizeof(char) * stats_buffer_size);
	if (!stats_buffer) {
		VC_TRACE("Thread %s exit due to malloc failure\n", __func__);
		return 0;
	}
	VC_TRACE("Thread %s init", __func__);
	while (get_state()) {
		/*
		 * Read temp
		 * Read volt stats
		 * find the mapping in table. get points per sec.
		 */
		lock_and_wait();
		log_events("Burnout monitor\n");
		memset(temperature_buffer, 0, sizeof(temperature_buffer) / sizeof(temperature_buffer[0]));
		sysfs_read(thermal_filepath, temperature_buffer, sizeof(temperature_buffer));
		temperature = strtol(temperature_buffer, NULL, 10);
		x = (temperature / HV_THERMAL_STEP);
		if (!prev_x)
			prev_x = x;
		/* This block for voltage change */
		sysfs_read(STATS_FILE, stats_buffer, stats_buffer_size);
		pstats = strtok(stats_buffer, "\n");
		pstats = strtok(NULL, "\n");
		i = 0;
		while (pstats != NULL) {
			pstats = strtok(NULL, " ");
			if (pstats && !strncmp(pstats, "vdd_core", strlen("vdd_core")))
				break;
			vt[i].volt = strtol(pstats, NULL, 10);
			pstats = strtok(NULL, "\n");
			vt[i].time = strtol(pstats, NULL, 10);
			i++;
		}

		/* Stats of voltages below 1V */
		temp_val = 0;
		/* Update temp_vs_volt table */
		for (j = 0; j < i; j++) {
			if (vt[j].volt < volt_range[0]) {
				temp_val += vt[j].time;
				continue;
			}
			y = (vt[j].volt - volt_range[0]) / (volt_range[1] - volt_range[0]);
			if (y == 0)
				vt[j].time += temp_val;
			/* Find diff with last and update that index. */
			temp_vs_volt_secs[prev_x + y*temp_numbers] += vt[j].time - temp_vs_volt_secs[y*temp_numbers];
			temp_vs_volt_secs[y*temp_numbers] = vt[j].time;
			if (x != prev_x)
				prev_x = x;
		}

		total_burnout = 0.0;
		/* find total burnout score */
		for (i = 0; i < volt_numbers; i++) /* Total voltages */
			for (j = 0; j < temp_numbers - 1; j++)/* Total temperatures */
				total_burnout += (burnout_table[i*(temp_numbers - 1)+j] *
						(float) temp_vs_volt_secs[i*temp_numbers+1+j])/(float)(100*60*60);

		/* subtract 1 to untrack the first element from tracking
		 * array as it is total time spend
		 */

		log_tables("-----------------------------------------\n");
		log_tables("%20s", "volt\\temp");
		for (i = 0; i < temp_numbers; i++)
			log_tables("%20d", temp_range[i]);
		log_tables("\n");
		log_tables("-----------------------------------------\n");
		for (i = 0; i < volt_numbers; i++) /* Total voltages */{
			log_tables("%19d", volt_range[i]);
			log_tables("|");
			for (j = 0; j < temp_numbers; j++)/* Total temperatures */
				log_tables("%20d", temp_vs_volt_secs[i*temp_numbers +j]);
			log_tables("\n");
		}

		pthread_mutex_lock(&mutex);
		stats_data.points_consumed = total_burnout;
		points_to_burn = stats_data.points_balance;
		pthread_mutex_unlock(&mutex);
		VC_TRACE(" total_burnout %f points_to_burn %f\n",
			total_burnout, points_to_burn);
		if (points_to_burn - total_burnout <= SAFE_USER_POINT_LIMIT) {
			snprintf(buf, sizeof(buf),  "%u", 1200);
			sysfs_write(VOLT_FILE, buf);
			sysfs_write(CAPPING_ENABLE_FILE, "1");
		} else
			sysfs_write(CAPPING_ENABLE_FILE, "0");
	}
	VC_TRACE("Thread %s exit", __func__);
	return 0;
}

/*
 * points_update_thread - Thread to update the available points. This
 * also saves the points to a device_config.txt file for availability
 * across boots.
 *
 * Thread execution -
 * 1.) Thread sleeps for the points update interval
 * 2.) Thread saves the time before sleep and reads time after resume.
 * 3.) The timer used is the CLOCK_MONOTONIC type of clock this gives
 * the time from a predefined reference. Thus changes to system clock
 * have no effect on the calculations.
 * 4.) The sleep time is used to calculate the amount of points to be
 * credited to user.
 * 5.) The sleep time is added to a time_since_saved variable.
 * 6.) If the time_since_saved variable is greater than
 * the POINTS_SAVE_INTERVAL then the points are saved to file.
 *
 */

static void *points_update_thread(void *data)
{
#define POINTS_UPDATE_INTERVAL	(60*60)
	/* #define POINTS_UPDATE_INTERVAL	(6) test change */
#define POINTS_PER_INTERVAL	((float)100/(float)24)
#define POINTS_SAVE_INTERVAL	(60*60*6)
	/* #define POINTS_SAVE_INTERVAL	(10) test_change */
	struct timespec start, finish, diff;
	unsigned long timediff;
	unsigned int seconds_to_add = 0;
	unsigned int time_since_credit;
	unsigned int time_since_saved = 0;
	unsigned int total_intervals;

	VC_TRACE("Thread %s init", __func__);
	pthread_mutex_lock(&mutex);
	seconds_to_add = stats_data.seconds_uncounted;
	pthread_mutex_unlock(&mutex);
	while (get_state()) {
		/* Hourly point credit system */
		clock_gettime(CLOCK_MONOTONIC, &start);
		sleep(POINTS_UPDATE_INTERVAL);
		clock_gettime(CLOCK_MONOTONIC, &finish);
		log_events("Points update\n");
		if ((finish.tv_nsec - start.tv_nsec) < 0) {
			diff.tv_sec = finish.tv_sec-start.tv_sec-1;
			diff.tv_nsec = 1000000000+finish.tv_nsec-start.tv_nsec;
		} else {
			diff.tv_sec = finish.tv_sec-start.tv_sec;
			diff.tv_nsec = finish.tv_nsec-start.tv_nsec;
		}
		time_since_saved += diff.tv_sec;
		diff.tv_sec += seconds_to_add;
		seconds_to_add = 0;
		if (diff.tv_sec <= POINTS_UPDATE_INTERVAL) {
			/* Add points POINTS_PER_INTERVAL */
			total_intervals = 1;
		} else {
			/* Find number of intervals actually elapsed*/
			total_intervals = diff.tv_sec/POINTS_UPDATE_INTERVAL;
			seconds_to_add =
				diff.tv_sec -
				(POINTS_UPDATE_INTERVAL * total_intervals);
		}
		/* Add points total_intervals * Points_per_interval */
		VC_TRACE("Time pointsinsleep %lu, totl_interval %d, secs_sc %d points %f time_saved %d\n",
				diff.tv_sec, total_intervals, seconds_to_add,
				(float)total_intervals * POINTS_PER_INTERVAL,
				time_since_saved);
		pthread_mutex_lock(&mutex);
		stats_data.points_balance +=
			((float)total_intervals * POINTS_PER_INTERVAL);
		pthread_mutex_unlock(&mutex);
		/* Saving of points */
		if (time_since_saved >= POINTS_SAVE_INTERVAL) {
			/* Save to file */
			time_since_saved = 0;
			save_to_file();
			VC_TRACE("***** Points Saved *****\n");
			log_events("Points saved\n");
		}
	}
	VC_TRACE("Thread %s exit", __func__);
	return 0;
}

struct thread_data threads[] = {
	THREAD_DATA_INIT(volt_mon_thread, NULL),
	THREAD_DATA_INIT(temp_mon_thread, NULL),
	THREAD_DATA_INIT(stats_update_thread, NULL),
	THREAD_DATA_INIT(points_update_thread, NULL),
};

static void cleanup_function(int x)
{
	int sig;
	int err;
	sigset_t set;
	unsigned i;

	sigemptyset(&set);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGINT);
	sigprocmask(SIG_BLOCK, &set, NULL);
	err = sigwait(&set, &sig);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
	log_events("Program exiting\n");
	VC_TRACE("cleanup function received signal %d err %d\n", sig, err);

	set_state(0);
	save_to_file();
}

/*
 * init_function : Finds the saved context file.
 * Validates sanity by crc check.
 * If sanity check pass then adds points equal to the interval
 * difference between current time and saved time and
 * adds the points to structure.
 * If sanity check fails currently provides user a 2100 points credit,
 * need to recreate this based on aging and other factors.
 */

static int init_function(void)
{
	int fd;
	int len;
	int valid_file = 0;
	struct saved_data sd;
	unsigned read_crc, calc_crc;
	struct timespec current_time, diff;
	unsigned int total_intervals;

	fd = open(DATA_FILE, O_RDWR);
	if (fd <= 0) {
		fd = open(DATA_FILE, O_CREAT | O_RDWR, 0666);
	} else {
		len = read(fd, &sd, sizeof(sd));
		if (len <= 0)
			goto exit;
		calc_crc = get_crcb((void *)&sd, sizeof(sd));
		len = read(fd, &read_crc, sizeof(unsigned int));
		if (len <= 0)
			goto exit;
		if (read_crc == calc_crc)
			valid_file++;
	}

exit:
	close(fd);
	if (valid_file) {
		VC_TRACE("File validated\n");
		clock_gettime(CLOCK_REALTIME, &current_time);
		if ((current_time.tv_nsec - sd.saved_time.tv_nsec) < 0) {
			diff.tv_sec =
				current_time.tv_sec - sd.saved_time.tv_sec - 1;
			diff.tv_nsec =
				1000000000 + current_time.tv_nsec -
				sd.saved_time.tv_nsec;
		} else {
			diff.tv_sec =
				current_time.tv_sec - sd.saved_time.tv_sec;
			diff.tv_nsec =
				current_time.tv_nsec - sd.saved_time.tv_nsec;
		}
		total_intervals = diff.tv_sec/POINTS_UPDATE_INTERVAL;
		stats_data.seconds_uncounted = diff.tv_sec -
			total_intervals * POINTS_UPDATE_INTERVAL;
		stats_data.points_balance =
			((float)total_intervals * POINTS_PER_INTERVAL);
		stats_data.points_balance += sd.balance_points;
	} else {
		/* Find the device age but provide boost points as of now */
		/* Add a 21 day boost points */
		stats_data.points_balance = 2100;
	}
	return 0;
}

static int init_thermal_path(void)
{
	int file = 0;
	FILE *fp;
	char path[100];
	char buf[100];
	int err = -1;

	while (file != 10) {
		snprintf(path, sizeof(path), "%s/thermal_zone%d/type", PATH, file);
		fp = fopen(path, "r");
		if (!fp)
			break;
		fgets(buf, 100, fp);
		if (!strncmp(buf, FILETYPE, strlen(FILETYPE))) {
			snprintf(thermal_filepath, sizeof(thermal_filepath), "%s/thermal_zone%d/temp", PATH, file);
			err = 0;
			break;
		}
		fclose(fp);
		file++;
	}
	VC_TRACE("TEMP PATH:%s\n", thermal_filepath);
	return err;
}

static void print_usage(void)
{
	VC_TRACE("usage: voltcapd [options]\n");
	VC_TRACE("-a:log all debug data\n");
	VC_TRACE("-e:log all event data\n");
	VC_TRACE("-t:log all table data\n");
}

int main(int argc, char *argv[])
{
	int err;
	unsigned int i;
	void *dev = NULL;
	int fd;

	/* Skip program name */
	argv++;
	argc--;
	while (argc != 0) {
		if (strcmp(argv[0], "-a") == 0)
			debug_params = LOG_ALL;
		else if (strcmp(argv[0], "-t") == 0)
			debug_params |= LOG_TABLE;
		else if (strcmp(argv[0], "-e") == 0)
			debug_params |= LOG_EVENTS;
		else
			print_usage();
		argc--;
		argv++;
	}
	if (debug_params) {
		fd = open(LOG_FILE, O_CREAT | O_RDWR, 0666);
		if (fd > 0)
			close(fd);
	}

	memset(&stats_data, 0, sizeof(struct status_data));
	init_function();
	if (init_thermal_path())
		return -1;
	stats_data.service_state = 1;
	for (i = 0; i < (sizeof(threads)/sizeof(struct thread_data)); i++) {
		pthread_create(&(threads[i].t), NULL,
				threads[i].f , threads[i].d);
	}

	cleanup_function(0);
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
	VC_TRACE("cpuvoltcapd exit **\n");
	exit(0);
	return 0;
}
