#include "defs.h"

#define ISDEAD(i) ( (i) >=41 && (i) <=102 )

float adjustangle(float current, float ideal, float fraction)
{
	float move;

	move = ideal - current;
	if (ideal > current)
	{

		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}

	move *= fraction;

	return (current + move);
}

void Interpolate(int num, frame_t *frame, demoinfo_t *demoinfo)
{
	float	exacttime, nexttime, f;
	int		i,j;
	frame_t	*nextframe;
	qboolean good;
	player_state_t *state, *nextstate, *prevstate;

	good = false;

	if (frame->fixangle[num] == true) {
		VectorCopy(frame->playerstate[num].origin, demoinfo->origin);
		VectorCopy(frame->playerstate[num].command.angles, demoinfo->angles);
		return;
	}
	
	i = world.lastwritten;
	prevstate = nextstate = state = &frame->playerstate[num];
	nexttime = exacttime = frame->time - (float)(state->command.msec)*0.001;
	while (nexttime < frame->time && i < world.parsecount)
	{
		good = false;
		i++;

		prevstate = nextstate;
		exacttime = nexttime;
		nextframe = &world.frames[i&UPDATE_MASK];
		nextstate = &nextframe->playerstate[num];
		if (nextframe->fixangle[num]) {
			break;
		}
		if (nextstate->messagenum != nextframe->parsecount){
			break;
		}
		if (!ISDEAD(nextstate->frame) && ISDEAD(state->frame)) {
			break;
		}

		nexttime = nextframe->time - (float)(nextstate->command.msec)*0.001;
		good = true;
	}

	if (good && nexttime > frame->time)
	{
		f = (frame->time - nexttime)/(nexttime - exacttime);

		for (j=0;j<3;j++) {
			demoinfo->angles[j] = adjustangle(prevstate->command.angles[j], nextstate->command.angles[j],1.0+f);
			demoinfo->origin[j] = nextstate->origin[j] + f*(nextstate->origin[j]-prevstate->origin[j]);
		}
	} else {
		VectorCopy(state->origin, demoinfo->origin);
		VectorCopy(state->command.angles, demoinfo->angles);
	}
}

/*
==================
WritePlayers

Writes an update of players state
==================
*/

void WritePlayers (sizebuf_t *msg, frame_t *frame)
{
	int i, j, msec, dflags;
	player_info_t	*info;
	player_state_t	*state;
	demoinfo_t	demoinfo;
	frame_t		*nextframe;

	state = frame->playerstate; //from.frames[from.parsecount&UPDATE_MASK].playerstate;
	nextframe = &world.frames[(world.lastwritten+1)&UPDATE_MASK];

	for (i = 0, info = from.players; i < MAX_CLIENTS; i++, state++, info++)
	{
		if (state->messagenum != frame->parsecount)
			continue;

		dflags = 0;

		if (world.lastwritten - world.demoinfo[i].parsecount >= UPDATE_BACKUP - 1) {
			if (debug)
				Sys_Printf("wipe out:%d, %d\n", i, world.lastwritten - world.demoinfo[i].parsecount);
			memset(&world.demoinfo[i], 0, sizeof(demoinfo_t));
		}

		world.demoinfo[i].parsecount = world.lastwritten;

		Interpolate(i, frame, &demoinfo);
		msec = state->command.msec;

		demoinfo.angles[2] = 0; // no roll angle

		if (frame->fixangle[i] == true/* || (ISDEAD(state->frame) && !ISDEAD(nextframe->playerstate[i].frame))*/) {
			frame->fixangle[i] = false;
			MSG_WriteByte(msg, svc_setangle);
			MSG_WriteByte(msg, i);
			for (j=0 ; j < 3 ; j++)
				MSG_WriteAngle (msg, nextframe->playerstate[i].command.angles[j]);

			if (debug)
				Sys_Printf("send fixangle:%d\n", i);
		}

		if (state->flags & PF_DEAD)
		{	// don't show the corpse looking around...
			demoinfo.angles[0] = 0;
			demoinfo.angles[1] = state->command.angles[1];
			demoinfo.angles[2] = 0;
		}

		for (j=0; j < 3; j++)
			if (world.demoinfo[i].origin[j] != demoinfo.origin[j])
				dflags |= DF_ORIGIN << j;

		for (j=0; j < 3; j++)
			if (world.demoinfo[i].angles[j] != demoinfo.angles[j])
					dflags |= DF_ANGLES << j;

		if (state->modelindex != world.demoinfo[i].model)
			dflags |= DF_MODEL;
		if (state->effects != world.demoinfo[i].effects)
			dflags |= DF_EFFECTS;
		if (state->skinnum != world.demoinfo[i].skinnum)
			dflags |= DF_SKINNUM;
		if (state->flags & PF_DEAD)
			dflags |= DF_DEAD;
		if (state->flags & PF_GIB)
			dflags |= DF_GIB;
		if (state->weaponframe != world.demoinfo[i].weaponframe)
			dflags |= DF_WEAPONFRAME;

		MSG_WriteByte (msg, svc_playerinfo);
		MSG_WriteByte (msg, i);
		MSG_WriteShort (msg, dflags);

		//if (dflags & DF_MODEL)
		//	Sys_Printf("%dsend:%d, model:%d\n", world.lastwritten, i, state->modelindex);

		//Sys_Printf("%d:dflags:%d\n", i, dflags);

		MSG_WriteByte (msg, state->frame);
		//MSG_WriteByte (msg, msec);

		for (j=0 ; j<3 ; j++)
			if (dflags & (DF_ORIGIN << j))
				MSG_WriteCoord (msg, demoinfo.origin[j]);
		
		for (j=0 ; j<3 ; j++)
			if (dflags & (DF_ANGLES << j))
				MSG_WriteAngle16 (msg, demoinfo.angles[j]);

		if (dflags & DF_MODEL)
			MSG_WriteByte (msg, state->modelindex);

		if (dflags & DF_SKINNUM)
			MSG_WriteByte (msg, state->skinnum);

		if (dflags & DF_EFFECTS)
			MSG_WriteByte (msg, state->effects);

		if (dflags & DF_WEAPONFRAME)
			MSG_WriteByte (msg, state->weaponframe);

		VectorCopy(demoinfo.origin, world.demoinfo[i].origin);
		VectorCopy(demoinfo.angles, world.demoinfo[i].angles);
		world.demoinfo[i].skinnum = state->skinnum;
		world.demoinfo[i].effects = state->effects;
		world.demoinfo[i].weaponframe = state->weaponframe;
		world.demoinfo[i].model = state->modelindex;
	}

	for (i = 0; i < MAX_CLIENTS; i++)
		frame->fixangle[i] = false;
}

/*
==================
WriteDelta

Writes part of a packetentities message.
Can delta from either a baseline or a previous packet_entity
==================
*/
void WriteDelta (entity_state_t *from, entity_state_t *to, sizebuf_t *msg, qboolean force)
{
	int		bits;
	int		i;
	float	miss;

// send an update
	bits = 0;
	
	for (i=0 ; i<3 ; i++)
	{
		miss = to->origin[i] - from->origin[i];
		if ( miss < -0.1 || miss > 0.1 )
			bits |= U_ORIGIN1<<i;
	}

	if ( to->angles[0] != from->angles[0] )
		bits |= U_ANGLE1;
		
	if ( to->angles[1] != from->angles[1] )
		bits |= U_ANGLE2;
		
	if ( to->angles[2] != from->angles[2] )
		bits |= U_ANGLE3;
		
	if ( to->colormap != from->colormap )
		bits |= U_COLORMAP;
		
	if ( to->skinnum != from->skinnum )
		bits |= U_SKIN;
		
	if ( to->frame != from->frame )
		bits |= U_FRAME;
	
	if ( to->effects != from->effects )
		bits |= U_EFFECTS;
	
	if ( to->modelindex != from->modelindex )
		bits |= U_MODEL;

	if (bits & 511)
		bits |= U_MOREBITS;

	if (to->flags & U_SOLID)
		bits |= U_SOLID;

	//
	// write the message
	//
	if (!to->number)
		Sys_Error ("Unset entity number");
	if (to->number >= 512)
		Sys_Error ("Entity number >= 512");

	if (!bits && !force)
		return;		// nothing to send!
	i = to->number | (bits&~511);
	if (i & U_REMOVE)
		Sys_Error ("U_REMOVE");
	MSG_WriteShort (msg, i);
	
	if (bits & U_MOREBITS)
		MSG_WriteByte (msg, bits&255);
	if (bits & U_MODEL)
		MSG_WriteByte (msg,	to->modelindex);
	if (bits & U_FRAME)
		MSG_WriteByte (msg, to->frame);
	if (bits & U_COLORMAP)
		MSG_WriteByte (msg, to->colormap);
	if (bits & U_SKIN)
		MSG_WriteByte (msg, to->skinnum);
	if (bits & U_EFFECTS)
		MSG_WriteByte (msg, to->effects);
	if (bits & U_ORIGIN1)
		MSG_WriteCoord (msg, to->origin[0]);		
	if (bits & U_ANGLE1)
		MSG_WriteAngle(msg, to->angles[0]);
	if (bits & U_ORIGIN2)
		MSG_WriteCoord (msg, to->origin[1]);
	if (bits & U_ANGLE2)
		MSG_WriteAngle(msg, to->angles[1]);
	if (bits & U_ORIGIN3)
		MSG_WriteCoord (msg, to->origin[2]);
	if (bits & U_ANGLE3)
		MSG_WriteAngle(msg, to->angles[2]);
}


/*
=============
EmitPacketEntities

Writes a delta update of a packet_entities_t to the message.
=============
*/

void EmitPacketEntities (sizebuf_t *msg, packet_entities_t *to)
{
	frame_t	*fromframe;
	packet_entities_t *from;
	int		oldindex, newindex;
	int		oldnum, newnum;
	int		oldmax;

	// this is the frame that we are going to delta update from
	if (world.delta_sequence != -1)
	{
		fromframe = &world.frames[world.delta_sequence & UPDATE_MASK];
		from = &fromframe->packet_entities;
		oldmax = from->num_entities;

		MSG_WriteByte (msg, svc_deltapacketentities);
		MSG_WriteByte (msg, world.delta_sequence);
	}
	else
	{
		oldmax = 0;	// no delta update
		from = NULL;

		MSG_WriteByte (msg, svc_packetentities);
	}

	newindex = 0;
	oldindex = 0;
//Con_Printf ("---%i to %i ----\n", client->delta_sequence & UPDATE_MASK
//			, client->netchan.outgoing_sequence & UPDATE_MASK);
	while (newindex < to->num_entities || oldindex < oldmax)
	{
		newnum = newindex >= to->num_entities ? 9999 : to->entities[newindex].number;
		oldnum = oldindex >= oldmax ? 9999 : from->entities[oldindex].number;

		if (newnum == oldnum)
		{	// delta update from old position
//Con_Printf ("delta %i\n", newnum);
			WriteDelta (&from->entities[oldindex], &to->entities[newindex], msg, false);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{	// this is a new entity, send it from the baseline
//Con_Printf ("baseline %i\n", newnum);
			WriteDelta (&baselines[newnum], &to->entities[newindex], msg, true);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{	// the old entity isn't present in the new message
//Con_Printf ("remove %i\n", oldnum);
			MSG_WriteShort (msg, oldnum | U_REMOVE);
			oldindex++;
			continue;
		}
	}

	MSG_WriteShort (msg, 0);	// end of packetentities
}


void EmitNailUpdate (sizebuf_t *msg, frame_t *frame)
{
	byte	bits[6];	// [48 bits] xyzpy 12 12 12 4 8 
	int		n, i;
	int		x, y, z, p, yaw;
	projectile_t	*pr;

	if (!frame->num_projectiles)
		return;

	MSG_WriteByte (msg, svc_nails);
	MSG_WriteByte (msg, frame->num_projectiles);

	for (n=0 ; n<frame->num_projectiles ; n++)
	{
		pr = &frame->projectiles[n];
		x = (int)(pr->origin[0]+4096)>>1;
		y = (int)(pr->origin[1]+4096)>>1;
		z = (int)(pr->origin[2]+4096)>>1;
		p = (int)(16*pr->angles[0]/360)&15;
		yaw = (int)(256*pr->angles[1]/360)&255;

		bits[0] = x;
		bits[1] = (x>>8) | (y<<4);
		bits[2] = (y>>4);
		bits[3] = z;
		bits[4] = (z>>8) | (p<<4);
		bits[5] = yaw;

		for (i=0 ; i<6 ; i++)
			MSG_WriteByte (msg, bits[i]);
	}
}

/*
void WriteEntitiesToClient (sizebuf_t *msg, frame_t *frame)
{
	packet_entities_t *efrom, *eto;

	eto = &world.frames[world.parsecount&UPDATE_MASK].packet_entities;
	efrom = &from.frames[from.validsequence&UPDATE_MASK].packet_entities;

	memcpy(eto, efrom, sizeof(packet_entities_t));

	//Sys_Printf("valid:%d, num:%d\n", from.validsequence, efrom->num_entities);

	WritePlayers (msg);
	EmitPacketEntities (eto, msg);
	EmitNailUpdate(msg);

	//Sys_Printf("this:%d(%d), last:%d(%d)\n", world.parsecount, eto->num_entities, world.delta_sequence, world.frames[world.delta_sequence & UPDATE_MASK].packet_entities.num_entities);
}
*/

void WritePackets(int num)
{
	frame_t *frame;
	sizebuf_t	msg;
	byte		buf[MAX_DATAGRAM];

	msg.data = buf;
	msg.maxsize = sizeof(buf);
	msg.allowoverflow = true;
	msg.overflowed = false;

	while (num)
	{
		msg.cursize = 0;
		frame = &world.frames[world.lastwritten&UPDATE_MASK];
		msgbuf =  &frame->buf;

		if (debug)
			Sys_Printf("real:%f, demo:%f\n", realtime, demo.time);
		// Add packet entities, nails, and player
		if (!frame->invalid) 
		{
			if (!world.lastwritten)
				world.delta_sequence = -1;

			WritePlayers (&msg, frame);
			EmitPacketEntities (&msg, &frame->packet_entities);
			EmitNailUpdate(&msg, frame);

			DemoWrite_Begin(dem_all, 0, msg.cursize);
			SZ_Write (msgbuf, msg.data, msg.cursize);
		}
		
		DemoWriteToDisk(demo.lasttype,demo.lastto, frame->time); // this goes first to reduce demo size a bit
		DemoWriteToDisk(0,0, frame->time); // now goes the rest
		num--;
		world.delta_sequence = world.lastwritten&255;
		world.lastwritten++;
	}
}