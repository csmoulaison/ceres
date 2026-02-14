void player_damage(GamePlayer* player, f32 damage) {
	player->health -= damage;
	player->hit_cooldown = damage * 5.0f;

	if(player->health <= 0.0f) {
		player->health = 1.0f;
		f32 pos = 32.0f;
		player->position = v2_new(pos, pos);
		player->velocity = v2_zero();
		player->hit_cooldown = 1.2f;
	}
}
