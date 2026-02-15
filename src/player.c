void player_damage(GamePlayer* player, f32 damage) {
	player->health -= damage;
	player->hit_cooldown = damage * 5.0f;
}
