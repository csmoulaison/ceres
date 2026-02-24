// TODO: Currently, I don't understand these all that well. Continue reading the
// physics book and get a better handle on what's going on. Incidentally, these
// don't really work all that great either. The final result should feel like a
// nice flawless game of ice hockey with a bit of friction. No stickies, please!

void physics_resolve_velocities(Session* session) {
	f32 radius = 0.5f;
	for(i32 player_index = 0; player_index < session->players_len; player_index++) {
		Player* player = &session->players[player_index];

		// Resolve against level, reducing checks to a 3x3 region around the player
		for(i32 pos = 0; pos < 25; pos++) {
			f32 x = player->position.x + (pos % 5) - 1.0f;
			f32 y = player->position.y + (pos / 5) - 1.0f;

			i32 cube_index = (i32)y * session->level.side_length + (i32)x;
			if(session->level.tiles[cube_index] == 0) continue;

			v2 relative_center = v2_sub(player->position, v2_new((i32)x, (i32)y));
			v2 offset_from_corner = v2_sub(v2_abs(relative_center), v2_new(0.5f, 0.5f));
			f32 penetration = f32_min(f32_max(offset_from_corner.x, offset_from_corner.y), 0.0f)
				+ v2_magnitude(v2_max(offset_from_corner, v2_zero()))
				- radius;

			v2 contact_normal;
			if(offset_from_corner.x > offset_from_corner.y) {
				contact_normal = v2_new(-1.0f, 1.0f);
			}
			if(offset_from_corner.y > offset_from_corner.x) {
				contact_normal = v2_new(1.0f, -1.0f);
			}

			if(penetration < 0.0f) {
				// TODO: Player damage should be based on both velocity and the angle of
				// incidence instead of just velocity
				f32 mag = v2_magnitude(player->velocity);
				if(mag > 10.0f) {
					player_damage(player, mag * 0.01f);
				}

				player->position = v2_add(player->position, v2_scale(v2_normalize(player->velocity), penetration * 1.2f));
				player->velocity = v2_mult(player->velocity, contact_normal);
				player->velocity = v2_scale(player->velocity, 0.7f);
				player->position = v2_add(player->position, v2_scale(v2_normalize(player->velocity), -penetration));
				//player->velocity = v2_add(player->velocity, v2_scale(contact_normal, v2_magnitude(player->velocity)));
			}
		}

		// Resolve against other players
		// TODO: Nudge the rotational speeds on collision.
		for(i32 other_index = player_index + 1; other_index < session->players_len; other_index++) {
			Player* other = &session->players[other_index];
			if(v2_distance(player->position, other->position) < radius * 2.0f) {
				f32 mag = v2_magnitude(player->velocity) + v2_magnitude(other->velocity);
				player_damage(player, mag * 0.01f);

				// This is the case of both ships having equal mass.
				//v2 collision_normal = v2_normalize(v2_sub(player->position, other->position));
				v2 vel_diff = v2_sub(player->velocity, other->velocity);
				v2 mass_diff = v2_zero();
				v2 vel_mass_other = v2_scale(v2_mult(v2_identity(), other->velocity), 2.0f);
				v2 mass_total = v2_add(v2_identity(), v2_identity());

				player->velocity = v2_scale(v2_add(v2_mult(player->velocity, mass_diff), v2_div(vel_mass_other, mass_total)), 0.66f);
				other->velocity  = v2_scale(v2_add(vel_diff, player->velocity), 0.66f);

				// Move circles out of each other
				v2 midpoint = v2_scale(v2_add(player->position, other->position), 0.5f);
				f32 distance = v2_distance(player->position, other->position);
				{
					v2 pos_diff = v2_scale(v2_sub(player->position, other->position), radius);
					player->position = v2_add(midpoint, v2_scale(pos_diff, 1.0f / distance));
				}
				{
					v2 pos_diff = v2_scale(v2_sub(other->position, player->position), radius);
					other->position = v2_add(midpoint, v2_scale(pos_diff, 1.0f / distance));
				}
			}
		}
	}
}
