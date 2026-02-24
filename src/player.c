void player_spawn(Player* player, Player* players, u8 players_len, Level* level) {
	u8 spawns_len = 0;
	u8 eligible_spawns[MAX_LEVEL_SPAWNS];
	for(i32 spawn_index = 0; spawn_index < level->spawns_len; spawn_index++) {
		if(level->spawns[spawn_index].team == player->team) {
			bool colliding_player = false;
			for(i32 player_index = 0; player_index < players_len; player_index++) {
				v2 spawn_pos = v2_new(level->spawns[spawn_index].x, level->spawns[spawn_index].y);
				if(v2_distance(players[player_index].position, spawn_pos) < 1.0f) {
					colliding_player = true;
					break;
				}
			}
			if(!colliding_player) {
				eligible_spawns[spawns_len] = spawn_index;
				spawns_len++;
			}
		}
	}

	assert(spawns_len != 0);
	i32 spawn_index = random_i32(spawns_len);
	LevelSpawn* spawn = &level->spawns[eligible_spawns[spawn_index]];
	player->health = 1.0f;
	player->position = v2_new(spawn->x, spawn->y);
	player->velocity = v2_zero();
	player->direction = 0.0f;
	player->rotation_velocity = 0.0f;
	player->hit_cooldown = 0.0f;
}

void player_damage(Player* player, f32 damage) {
	player->health -= damage;
	player->hit_cooldown = damage * 5.0f;
}
