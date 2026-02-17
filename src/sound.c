void sound_init(SoundChannel* channels) {
	for(i32 channel_index = 0; channel_index < SOUND_CHANNELS_COUNT; channel_index++) {
		SoundChannel* channel = &channels[channel_index];
		channel->phase = 0.0f;
		channel->frequency = 0.0f;
		channel->amplitude = 0.0f;
		channel->shelf = 0.0f;
		channel->volatility = 0.0f;
		channel->pan = 0.0f;
		channel->actual_frequency = 0.0f;
		channel->actual_amplitude = 0.0f;
	}
}

void sound_update(SoundChannel* channels, GameState* game) {
	for(i32 ch = 0; ch < SOUND_CHANNELS_COUNT; ch++) {
		SoundChannel* channel = &channels[ch];
		channel->amplitude = 0.0f;
		channel->frequency = 0.0f;
		channel->volatility = 0.0f;
		//*channel = (SoundChannel){};
	}

	f32 current_player_velocity;

#define player_channels 5
	for(i32 pl = 0; pl < 2; pl++) {
		GamePlayer* player = &game->players[pl];
		f32 v_mag = v2_magnitude(player->velocity);
		if(pl == 0) current_player_velocity = v_mag;

		SoundChannel* velocity_channel = &game->sound_channels[0 + pl * player_channels];
		velocity_channel->amplitude = 2000.0f * v_mag;
		velocity_channel->frequency = 7.144123f * v_mag;
		velocity_channel->shelf = 6000.0f;
		velocity_channel->volatility = 0.002f * v_mag;

		SoundChannel* momentum_channel = &game->sound_channels[1 + pl * player_channels];
		f32 m = player->momentum_cooldown_sound;
		momentum_channel->amplitude = 2000.0f * m;
		momentum_channel->frequency = 3.0f * m;
		momentum_channel->shelf = 2000.0f;
		momentum_channel->volatility = 0.01f * m;

		SoundChannel* rotation_channel = &game->sound_channels[2 + pl * player_channels];
		f32 rot = player->rotation_velocity;
		rotation_channel->amplitude = 2000.0f * rot;
		rotation_channel->frequency = 10.0f * abs(rot);
		rotation_channel->shelf = 6000.0f;
		rotation_channel->volatility = 0.002f * rot;

		SoundChannel* shoot_channel = &game->sound_channels[3 + pl * player_channels];
		f32 shoot = player->shoot_cooldown_sound;
		shoot_channel->amplitude = 10000.0f * shoot * shoot;
		shoot_channel->frequency = 1500.0f * shoot * shoot;
		shoot_channel->shelf = 2000.0f + 2000.0f * shoot;
		shoot_channel->volatility = 0.2f * shoot;

		SoundChannel* hit_channel = &game->sound_channels[4 + pl * player_channels];
		f32 hit = player->hit_cooldown;
		hit_channel->amplitude = 1000.0f * hit * hit * ((f32)rand() / RAND_MAX) * 2500.0f;
		hit_channel->frequency = 200.0f * hit * hit;
		hit_channel->shelf = 2000.0f + 2000.0f * hit;
		hit_channel->volatility = 2.0f * hit * ((f32)rand() / RAND_MAX);

		for(i32 ch = 0; ch < player_channels; ch++) {
			SoundChannel* channel = &game->sound_channels[ch + pl * player_channels];

			f32 distance_1 = 1.0f + v2_distance(game->players[0].position, player->position);
			f32 distance_2 = 1.0f + v2_distance(game->players[1].position, player->position);
			f32 distance = fmin(distance_1, distance_2);
			// TODO: This is for singleplayer case.
			distance = distance_1;

			channel->amplitude /= distance * 1.0f;
			channel->shelf += distance * 100.0f;
			channel->volatility -= distance * 0.01f;
			channel->pan = (game->players[0].position.y - player->position.y) * 0.2f;
			if(channel->volatility < 0.0f) channel->volatility = 0.0f;
		}
	}

	i32 mod_phase = 12;
	i32 mod_half = mod_phase / 2;

	SoundChannel* music_channel = &game->sound_channels[player_channels * 2];
	f32 frequencies[32] = { 1, 0, 2, 0, 1, 0, 1, 0, 2, 0, 2, 0, 1, 0, 2, 0, 0, 0, 1, 0, 2, 0, 2, 0, 1, 0, 2, 0, 2, 0, 1 };
	f32 freq = frequencies[(game->frame / mod_half) % 16];
	music_channel->amplitude = 4000.0f - (game->frame % mod_half) * 100.0f * freq + current_player_velocity * 2000.0f;
	music_channel->frequency = freq * 100.0f;
	music_channel->shelf = 4000.0f;
	music_channel->volatility = freq * 0.01f - (game->frame % mod_half) * freq * 0.1f;
	music_channel->volatility = 0.0f;
	music_channel->pan = 0.0f;

	SoundChannel* ch2 = &game->sound_channels[player_channels * 2 + 1];
	ch2->amplitude = 5000.0f - (game->frame % mod_half) * 100.0f * freq + current_player_velocity * 1000.0f;
	ch2->frequency = freq * 33.3f;
	ch2->shelf = 4000.0f;
	ch2->volatility = freq * 0.05f;
	ch2->volatility = 0.0f;
	ch2->pan = 0.0f;

	SoundChannel* ch4 = &game->sound_channels[player_channels * 2 + 2];
	f32 fs4[16] = { 6, 1, 0, 4, 0, 0, 3, 0, 4, 0, 2, 1, 6, 1, 2, 1 };
	f32 f4 = fs4[(game->frame / mod_phase) % 16];
	ch4->amplitude = 100.0f + current_player_velocity * 400.0f - (game->frame % mod_phase) * 4000.0f;
	ch4->frequency = f4 * ((f32)rand() / RAND_MAX) * 400.0f;
	ch4->shelf = 4000.0f + current_player_velocity * 50.0f;
	ch4->volatility = f4 * 0.05f - (game->frame % mod_phase) * 0.01f;
	ch4->volatility = 0.0f;
	ch4->pan = 0.0f;

	SoundChannel* ch3 = &game->sound_channels[player_channels * 2 + 3];
	f32 fs3[16] = { 2, 1, 4, 1, 6, 1, 4, 1, 2, 3, 4, 1, 6, 3, 2, 3 };
	f32 f3 = fs3[(game->frame / mod_phase) % 16];
	ch3->amplitude = 2000.0f * f3 - (game->frame % mod_phase) * 8000.0f;
	ch3->frequency = f3 * ((f32)rand() / RAND_MAX) * 25.0f;
	ch3->shelf = 1000.0f + current_player_velocity * 100.0f;
	ch3->volatility = f3 * (0.1f + current_player_velocity * 0.02f - (game->frame % mod_phase) * 0.2f);
	ch3->pan = 0.0f;
}
