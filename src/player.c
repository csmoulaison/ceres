void player_spawn(GamePlayer* player, GameLevel* level, u8 player_team) {
	u8 spawns_len = 0;
	u8 team_spawns[MAX_LEVEL_SPAWNS];
	for(i32 sp = 0; sp < level->spawns_len; sp++) {
		if(level->spawns[sp].team == player_team) {
			team_spawns[spawns_len] = sp;
			spawns_len++;
		}
	}

	u8 spawn = team_spawns[random_i32(spawns_len)];
	player->health = 1.0f;
	player->position = v2_new(level->spawns[spawn].x, level->spawns[spawn].y);
	player->velocity = v2_zero();
	player->direction = 0.0f;
	player->rotation_velocity = 0.0f;
	player->hit_cooldown = 0.0f;
}

void player_damage(GamePlayer* player, f32 damage) {
	player->health -= damage;
	player->hit_cooldown = damage * 5.0f;
}
