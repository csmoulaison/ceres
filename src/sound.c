void sound_init(SoundState* sound) {
	sound->channels_len = 0;
	for(i32 channel_index = 0; channel_index < SOUND_MAX_CHANNELS; channel_index++) {
		SoundChannel* channel = &sound->channels[channel_index];
		channel->phase = 0.0f;
		channel->actual_frequency = 0.0f;
		channel->actual_amplitude = 0.0f;
	}
}

SoundChannel* sound_push_channel(SoundState* sound) {
	SoundChannel* channel = &sound->channels[sound->channels_len - 1];
	channel->frequency = 0.0f;
	channel->amplitude = 0.0f;
	channel->shelf = 0.0f;
	channel->volatility = 0.0f;
	channel->pan = 0.0f;
	sound->channels_len++;
	return channel;
}

void sound_spatialize_channel(SoundChannel* channel, v2 source_position, v2 listener_position) {
	f32 distance = v2_distance(source_position, listener_position);
	channel->amplitude /= distance * 1.0f;
	channel->shelf += distance * 100.0f;
	channel->volatility -= distance * 0.01f;
	channel->pan = (source_position.x - listener_position.x) * 0.2f;
	if(channel->volatility < 0.0f) channel->volatility = 0.0f;
}

void sound_update(SoundState* sound, GameState* game) {
	sound->channels_len = 0;
	f32 primary_player_velocity = v2_magnitude(game->players[0].velocity);

	SoundChannel* ch;
	for(i32 pl = 0; pl < 2; pl++) {
		GamePlayer* player = &game->players[pl];
		GamePlayer* other = &game->players[0];
		if(pl == 0) other = &game->players[1];
		f32 v_mag = v2_magnitude(player->velocity);

		ch = sound_push_channel(sound);
		ch->amplitude = 2000.0f * v_mag;
		ch->frequency = 7.144123f * v_mag;
		ch->shelf = 6000.0f;
		ch->volatility = 0.002f * v_mag;
		sound_spatialize_channel(ch, other->position, player->position);

		ch = sound_push_channel(sound);
		f32 m = player->momentum_cooldown_sound;
		ch->amplitude = 2000.0f * m;
		ch->frequency = 3.0f * m;
		ch->shelf = 2000.0f;
		ch->volatility = 0.01f * m;
		sound_spatialize_channel(ch, other->position, player->position);

		ch = sound_push_channel(sound);
		f32 rot = player->rotation_velocity;
		ch->amplitude = 2000.0f * rot;
		ch->frequency = 10.0f * abs(rot);
		ch->shelf = 6000.0f;
		ch->volatility = 0.002f * rot;
		sound_spatialize_channel(ch, other->position, player->position);

		ch = sound_push_channel(sound);
		f32 shoot = player->shoot_cooldown_sound;
		ch->amplitude = 10000.0f * shoot * shoot;
		ch->frequency = 1500.0f * shoot * shoot;
		ch->shelf = 2000.0f + 2000.0f * shoot;
		ch->volatility = 0.2f * shoot;
		sound_spatialize_channel(ch, other->position, player->position);

		ch = sound_push_channel(sound);
		f32 hit = player->hit_cooldown;
		ch->amplitude = 1000.0f * hit * hit * ((f32)rand() / RAND_MAX) * 2500.0f;
		ch->frequency = 200.0f * hit * hit;
		ch->shelf = 2000.0f + 2000.0f * hit;
		ch->volatility = 2.0f * hit * ((f32)rand() / RAND_MAX);
		sound_spatialize_channel(ch, other->position, player->position);

		/*
		for(i32 ch = 0; ch < player_channels; ch++) {
			SoundChannel* channel = &game->sound_channels[ch + pl * player_channels];

		}
		*/
	}

	i32 mod_phase = 12;
	i32 mod_half = mod_phase / 2;

	ch = sound_push_channel(sound);
	f32 frequencies[32] = { 1, 0, 2, 0, 1, 0, 1, 0, 2, 0, 2, 0, 1, 0, 2, 0, 0, 0, 1, 0, 2, 0, 2, 0, 1, 0, 2, 0, 2, 0, 1 };
	f32 freq = frequencies[(game->frame / mod_half) % 16];
	ch->amplitude = 4000.0f - (game->frame % mod_half) * 100.0f * freq + primary_player_velocity * 2000.0f;
	ch->frequency = freq * 100.0f;
	ch->shelf = 4000.0f;
	ch->volatility = freq * 0.01f - (game->frame % mod_half) * freq * 0.1f;
	ch->volatility = 0.0f;
	ch->pan = 0.0f;

	ch = sound_push_channel(sound);
	ch->amplitude = 5000.0f - (game->frame % mod_half) * 100.0f * freq + primary_player_velocity * 1000.0f;
	ch->frequency = freq * 33.3f;
	ch->shelf = 4000.0f;
	ch->volatility = freq * 0.05f;
	ch->volatility = 0.0f;
	ch->pan = 0.0f;

	ch = sound_push_channel(sound);
	f32 fs4[16] = { 6, 1, 0, 4, 0, 0, 3, 0, 4, 0, 2, 1, 6, 1, 2, 1 };
	f32 f4 = fs4[(game->frame / mod_phase) % 16];
	ch->amplitude = 100.0f + primary_player_velocity * 400.0f - (game->frame % mod_phase) * 4000.0f;
	ch->frequency = f4 * ((f32)rand() / RAND_MAX) * 400.0f;
	ch->shelf = 4000.0f + primary_player_velocity * 50.0f;
	ch->volatility = f4 * 0.05f - (game->frame % mod_phase) * 0.01f;
	ch->volatility = 0.0f;
	ch->pan = 0.0f;

	ch = sound_push_channel(sound);
	f32 fs3[16] = { 2, 1, 4, 1, 6, 1, 4, 1, 2, 3, 4, 1, 6, 3, 2, 3 };
	f32 f3 = fs3[(game->frame / mod_phase) % 16];
	ch->amplitude = 2000.0f * f3 - (game->frame % mod_phase) * 8000.0f;
	ch->frequency = f3 * ((f32)rand() / RAND_MAX) * 25.0f;
	ch->shelf = 1000.0f + primary_player_velocity * 100.0f;
	ch->volatility = f3 * (0.1f + primary_player_velocity * 0.02f - (game->frame % mod_phase) * 0.2f);
	ch->pan = 0.0f;
}
