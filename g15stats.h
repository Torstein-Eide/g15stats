#define	TEXT_LEFT	1
#define NUM_BATS	3
#define NUM_PROBES	4

#define MAX_NET_HIST	107

#define MAX_SENSOR      4

#define MAX_MODE        1
#define MAX_SUB_MODE    1

#define MAX_INTERVAL    300

#define PAUSE           6
#define RETRY_COUNT     10

#define	VL_LEFT		42
#define	BAR_START	45
#define	BAR_END		153
#define BAR_BOTTOM      32
#define	TEXT_RIGHT      155
#define	MAX_LINES	128
#define INFO_ROW        37

#define SENSOR_ERROR    -100

#define CHANGE_DOWN     1
#define CHANGE_UP       2
#define CHANGE_MODE     3
#define CHANGE_SUBMODE  4

#define SCREEN_SUMMARY  0
#define SCREEN_CPU      1
#define SCREEN_FREQ     2
#define SCREEN_FREQ_AGG 3
#define SCREEN_MEM      4
#define SCREEN_SWAP     5
#define SCREEN_NET      6
#define SCREEN_BAT      7
#define SCREEN_TEMP     8
#define SCREEN_FAN      9
#define SCREEN_NET2     10

#define	MAX_SCREENS	9

typedef struct g15_stats_bat_info
{	
	long	max_charge;
	long	cur_charge;
	long	status;
} g15_stats_bat_info;

typedef struct g15_stats_info
{
	float	cur;
	float	max;
} g15_stats_info;
