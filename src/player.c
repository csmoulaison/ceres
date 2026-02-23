void player_spawn(Player* player, Level* level) {
	u8 spawns_len = 0;
	u8 team_spawns[MAX_LEVEL_SPAWNS];
	for(i32 spawn_index = 0; spawn_index < level->spawns_len; spawn_index++) {
		if(level->spawns[spawn_index].team == player->team) {
			team_spawns[spawns_len] = spawn_index;
			spawns_len++;
		}
	}

	i32 spawn_index = random_i32(spawns_len);
	u8 spawn = team_spawns[spawn_index];
	player->health = 1.0f;
	player->position = v2_new(level->spawns[spawn].x, level->spawns[spawn].y);
	// TODO: This is to stop players from being in the exact same position, which
	// breaks collisions. We should make both collisions and spawning more robust,
	// obviously.
	player->position = v2_add(player->position, v2_new(random_f32() * 0.1f, random_f32() * 0.1f));
	player->velocity = v2_zero();
	player->direction = 0.0f;
	player->rotation_velocity = 0.0f;
	player->hit_cooldown = 0.0f;
}

void player_damage(Player* player, f32 damage) {
	player->health -= damage;
	player->hit_cooldown = damage * 5.0f;
}
