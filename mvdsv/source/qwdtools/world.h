// player_state_t is the information needed by a player entity
// to do move prediction and to generate a drawable entity
typedef struct
{
	int			messagenum;		// all player's won't be updated each frame

	usercmd_t	command;		// last command for prediction

	vec3_t		origin;
	vec3_t		viewangles;		// only for demos, not from server
	vec3_t		velocity;
	int			weaponframe;

	int			modelindex;
	int			frame;
	int			skinnum;
	int			effects;

	int			flags;			// dead, gib, etc
} player_state_t;


#define	MAX_SCOREBOARDNAME	16
typedef struct player_info_s
{
	char	userinfo[MAX_INFO_STRING];
	char	name[MAX_SCOREBOARDNAME];
	int		stats[MAX_CL_STATS];	// health, etc
	qboolean	spectator;
} player_info_t;

typedef struct
{
	vec3_t	origin;
	vec3_t	angles;
	int		weaponframe;
	int		skinnum;
	int		model;
	int		effects;
	int		parsecount;
} demoinfo_t;

typedef struct
{
	int		modelindex;
	vec3_t	origin;
	vec3_t	angles;
} projectile_t;

#define	MAX_PROJECTILES	32

typedef struct
{
	// generated on client side
	usercmd_t	cmd;		// cmd that generated the frame
	double		senttime;	// time cmd was sent off
	int			delta_sequence;		// sequence number to delta from, -1 = full update

	// received from server
	double		receivedtime;	// time message was received, or -1
	player_state_t	playerstate[MAX_CLIENTS];	// message received that reflects performing
	qboolean	fixangle[MAX_CLIENTS];
							// the usercmd
	packet_entities_t	packet_entities;
	qboolean	invalid;		// true if the packet_entities delta was invalid
	sizebuf_t	buf;
	byte		buf_data[20*MAX_MSGLEN]; // heh?
	projectile_t projectiles[MAX_PROJECTILES];
	int			num_projectiles;
	int			parsecount;
	float		latency;
	float		time;
} frame_t;


typedef struct
{
	int		length;
	char	map[MAX_STYLESTRING];
} lightstyle_t;

#define	MAX_EFRAGS		512

#define	MAX_DEMOS		8
#define	MAX_DEMONAME	16

typedef struct
{
	qboolean	interpolate;
	vec3_t		origin;
	vec3_t		angles;
	int			oldindex;
} interpolate_t;

typedef struct
{
	int			servercount;	// server identification for prespawns
	char		serverinfo[MAX_SERVERINFO_STRING];
	int			parsecount;		// server message counter
	int			delta_sequence;
	int			validsequence;	// this is the sequence number of the last good
								// packetentity_t we got.  If this is 0, we can't
								// render a frame yet
	int			lastwritten;
	frame_t		frames[UPDATE_BACKUP];

	player_info_t	players[MAX_CLIENTS];
	interpolate_t	int_entities[MAX_PACKET_ENTITIES];
	int				int_packet;
	qboolean	signonloaded;

	demoinfo_t	demoinfo[MAX_CLIENTS];
	float		time;

} world_state_t;

extern world_state_t	world;
extern lightstyle_t		lightstyle[MAX_LIGHTSTYLES];
extern entity_state_t	baselines[MAX_EDICTS];
extern float			realtime;